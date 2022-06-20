/*
 * Copyright (c) 2022, Antti Hyvarinen <antti.hyvarinen@gmail.com>
 * Copyright (c) 2022, Seyedmasoud Asadzadeh <seyedmasoud.asadzadeh@usi.ch>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef PTPLIB_NET_CHANNEL_HPP
#define PTPLIB_NET_CHANNEL_HPP

#include "Header.hpp"
#include "Lemma.hpp"
#include "SMTSEvent.hpp"

#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <map>
#include <algorithm>
#include <chrono>
#include <deque>
#include <cassert>

namespace PTPLib::net {

    using map_solverBranch_lemmas = std::map<std::string, std::vector<PTPLib::net::Lemma>>;
    using time_duration = std::chrono::duration<double>;

    template <class EVENT, class LEMMA>
    class Channel {

        std::mutex mutex;
        std::condition_variable cv;

        using queue_event = std::deque<EVENT>;
        queue_event events;
        std::unique_ptr<map_solverBranch_lemmas> solverBranchToPublishLemmas;
        std::unique_ptr<map_solverBranch_lemmas> solverBranchToPulledLemmas;

        PTPLib::net::Header current_header;

        std::atomic_bool requestStop;
        bool reset;
        std::atomic_bool isStopping;

        bool clauseShareMode;
        std::atomic_bool shouldLearnClause;

        bool parallel_mode;
        bool color_mode;

    public:
        Channel()
        : requestStop(false)
        , reset(false)
        , isStopping(false)
        , clauseShareMode(false)
        , shouldLearnClause(true)
        , parallel_mode(false)
        , color_mode(false)
        {
            solverBranchToPublishLemmas = std::make_unique<map_solverBranch_lemmas>();
            solverBranchToPulledLemmas = std::make_unique<map_solverBranch_lemmas>();
        }

        std::mutex & getMutex() { return mutex; }

        void insert_learned_clause(std::vector<LEMMA> && toPublish_clauses) {
            assert(not get_current_header().empty());
            (*solverBranchToPublishLemmas)[get_current_header().at(PTPLib::common::Param.NODE)].insert
                    (
                            std::end((*solverBranchToPublishLemmas)[get_current_header().at(PTPLib::common::Param.NODE)]),
                            std::begin(toPublish_clauses), std::end(toPublish_clauses)
                    );
        }

        void insert_pulled_clause(std::vector<LEMMA> && toInject_clauses) {
            assert(not get_current_header().empty());
            (*solverBranchToPulledLemmas)[get_current_header().at(PTPLib::common::Param.NODE)].insert
                    (
                            std::end((*solverBranchToPulledLemmas)[get_current_header().at(PTPLib::common::Param.NODE)]),
                            std::begin(toInject_clauses), std::end(toInject_clauses)
                    );
        }

        std::unique_ptr<map_solverBranch_lemmas> swap_learned_clauses() {
            auto out = std::make_unique<map_solverBranch_lemmas>();
            std::swap(out, solverBranchToPublishLemmas);
            return out;
        };

        std::unique_ptr<map_solverBranch_lemmas> swap_pulled_clauses() {
            auto out = std::make_unique<map_solverBranch_lemmas>();
            std::swap(out, solverBranchToPulledLemmas);
            return out;
        };

        void clear_queries() {
            queue_event empty_q;
            std::swap(events, empty_q);
        }

        size_t size_event() const { return events.size(); }

        bool isEmpty_event() const { return events.empty(); }

        queue_event get_events() const & { return events; }

        EVENT pop_front_event() {
            EVENT tmp_p(std::move(events.front()));
            events.pop_front();
            return tmp_p;
        }

        std::string & front_event() { return events.front().header.at(PTPLib::common::Param.COMMAND); }

        template <typename Arg>
        void push_back_event(Arg && event) {
            assert((not event.header.at(PTPLib::common::Param.NODE).empty()) and (not event.header.at(PTPLib::common::Param.NAME).empty()));
            events.push_back(std::forward<Arg>(event));
        }

        template <typename Arg>
        void push_front_event(Arg && event) {
            assert((not event.header.at(PTPLib::common::Param.NODE).empty()) and (not event.header.at(PTPLib::common::Param.NAME).empty()));
            events.push_front(std::forward<Arg>(event));
        }

        void set_current_header(PTPLib::net::Header & hd) {
            assert((not hd.at(PTPLib::common::Param.NODE).empty()) and (not hd.at(PTPLib::common::Param.NAME).empty()));
            current_header = hd.copy(hd.keys());
        }

        void set_current_header(PTPLib::net::Header & hd, const std::vector<std::string> & keys) {
            assert((hd.count(PTPLib::common::Param.NODE) == 1) and
            ((hd.count(PTPLib::common::Param.NAME) == 1)) and
            ((hd.count(PTPLib::common::Param.QUERY) == 1)));
            current_header = hd.copy(keys);
        }

        PTPLib::net::Header get_current_header(const std::vector<std::string> & keys) {
            return current_header.copy(keys);
        }

        PTPLib::net::Header get_current_header() { return current_header; }

        void clear_current_header() { current_header.clear(); }

        size_t size() const { return solverBranchToPublishLemmas->size(); }

        auto begin() const { return solverBranchToPublishLemmas->begin(); }

        auto end() const { return solverBranchToPublishLemmas->end(); }

        void notify_one() { cv.notify_one(); }

        void notify_all() { cv.notify_all(); }

        void clear_learned_clauses() { solverBranchToPublishLemmas->clear(); }

        void clear_pulled_clauses() { solverBranchToPulledLemmas->clear(); }

        bool empty_learned_clauses() const { return (solverBranchToPublishLemmas->empty()); }

        bool shouldReset() const { return reset; }

        void setReset() { reset = true; }

        void clearReset() { reset = false; }

        bool shouldStop() const { return requestStop; }

        void setShouldStop() { requestStop = true; }

        void clearShouldStop() { requestStop = false; }

        bool shallStop() const { return isStopping; }

        void setShallStop() { isStopping = true; }

        void clearShallStop() { isStopping = false; }

        bool isClauseShareMode() const { return clauseShareMode; }

        void setClauseShareMode() { clauseShareMode = true; }

        void clearClauseShareMode() { clauseShareMode = false; }

        bool shouldLearnClauses() const { return shouldLearnClause; }

        void setShouldLearnClauses() { shouldLearnClause = true; }

        void clearShouldLearnClauses() { shouldLearnClause = false; }

        bool isSolverInParallelMode() const { return parallel_mode; }

        void setParallelMode() { parallel_mode = true; }

        void clearParallelMode() { parallel_mode = false; }

        bool isColorMode() const { return color_mode; }

        void setColorMode() { color_mode = true; }

        void clearColorMode() { color_mode = false; }

        bool wait_for_reset(std::unique_lock<std::mutex> & lock, const time_duration & td) {
            return cv.wait_for(lock, td, [&] {
                return shouldReset();
            });
        }

        void wait_event_solver_reset(std::unique_lock<std::mutex> & lock) {
            cv.wait(lock, [&] {
                return (shouldReset() or shallStop() or not isEmpty_event());
            });
        }

        void resetChannel() {
            clear_pulled_clauses();
            clear_learned_clauses();
            clear_current_header();
            if (not isEmpty_event())
                clear_queries();
            clearShouldStop();
            clearShallStop();
            clearReset();
        }

    };
}
#endif // PTPLIB_NET_CHANNEL_HPP