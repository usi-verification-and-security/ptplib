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

    using queue_event = std::deque<PTPLib::net::SMTS_Event>;
    using map_solver_clause = std::map<std::string, std::vector<PTPLib::net::Lemma>>;
    using time_duration = std::chrono::duration<double>;

    class Channel {

        std::mutex mutex;
        std::condition_variable cv;

        queue_event queries;
        std::unique_ptr<map_solver_clause> m_learned_clauses;
        std::unique_ptr<map_solver_clause> m_pulled_clauses;

        PTPLib::net::Header current_header;

        std::atomic_bool requestStop;
        bool reset;
        std::atomic_bool isStopping;

        bool clauseShareMode;
        std::atomic_bool shouldLearnClause;
        int clauseLearnDuration;

        bool cube_and_conquer;
        bool color_mode;

    public:
        Channel()
        : requestStop(false)
        , reset(false)
        , isStopping(false)
        , clauseShareMode(false)
        , shouldLearnClause(true)
        , clauseLearnDuration(1000)
        , cube_and_conquer(false)
        , color_mode(false)
        {
            m_learned_clauses = std::make_unique<map_solver_clause>();
            m_pulled_clauses = std::make_unique<map_solver_clause>();
        }

        std::mutex & getMutex() { return mutex; }

        void insert_learned_clause(std::vector<PTPLib::net::Lemma> && toPublish_clauses) {
            assert(not get_current_header().empty());
            (*m_learned_clauses)[get_current_header().at(PTPLib::common::Param.NODE)].insert
                    (
                            std::end((*m_learned_clauses)[get_current_header().at(PTPLib::common::Param.NODE)]),
                            std::begin(toPublish_clauses), std::end(toPublish_clauses)
                    );
        }

        void insert_pulled_clause(std::vector<PTPLib::net::Lemma> && toInject_clauses) {
            assert(not get_current_header().empty());
            (*m_pulled_clauses)[get_current_header().at(PTPLib::common::Param.NODE)].insert
                    (
                            std::end((*m_pulled_clauses)[get_current_header().at(PTPLib::common::Param.NODE)]),
                            std::begin(toInject_clauses), std::end(toInject_clauses)
                    );
        }

        std::unique_ptr<map_solver_clause> swap_learned_clauses() {
            auto out = std::make_unique<map_solver_clause>();
            std::swap(out, m_learned_clauses);
            return out;
        };

        std::unique_ptr<map_solver_clause> swap_pulled_clauses() {
            auto out = std::make_unique<map_solver_clause>();
            std::swap(out, m_pulled_clauses);
            return out;
        };

        void clear_queries() {
            queue_event empty_q;
            std::swap(queries, empty_q);
        }

        size_t size_query() const { return queries.size(); }

        bool isEmpty_query() const { return queries.empty(); }

        queue_event get_events() const & { return queries; }

        SMTS_Event pop_front_query() {
            SMTS_Event tmp_p(std::move(queries.front()));
            queries.pop_front();
            return tmp_p;
        }

        std::string & front_query() { return queries.front().header.at(PTPLib::common::Param.COMMAND); }

        template<class T>
        void push_back_event(T && event) {
            assert((not event.header.at(PTPLib::common::Param.NODE).empty()) and (not event.header.at(PTPLib::common::Param.NAME).empty()));
            queries.push_back(std::forward<T>(event));
        }

        template<class T>
        void push_front_event(T && event) {
            assert((not event.header.at(PTPLib::common::Param.NODE).empty()) and (not event.header.at(PTPLib::common::Param.NAME).empty()));
            queries.push_front(std::forward<T>(event));
        }

        void set_current_header(PTPLib::net::Header & hd) {
            assert((not hd.at(PTPLib::common::Param.NODE).empty()) and (not hd.at(PTPLib::common::Param.NAME).empty()));
            current_header = hd.copy(hd.keys());
        }

        PTPLib::net::Header  get_current_header(const std::vector <std::string> & keys) {
            return current_header.copy(keys);
        }

        PTPLib::net::Header  get_current_header() { return current_header; }

        void clear_current_header() { current_header.clear(); }

        size_t size() const { return m_learned_clauses->size(); }

        auto begin() const { return m_learned_clauses->begin(); }

        auto end() const { return m_learned_clauses->end(); }

        void notify_one() { cv.notify_one(); }

        void notify_all() { cv.notify_all(); }

        void clear_learned_clauses() { m_learned_clauses->clear(); }

        void clear_pulled_clauses() { m_pulled_clauses->clear(); }

        bool empty_learned_clauses() const { return (m_learned_clauses->empty()); }

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

        void setClauseLearnDuration(int cld) { clauseLearnDuration = cld; }

        int getClauseLearnDuration() const { return clauseLearnDuration; }

        bool shouldLearnClauses() const { return shouldLearnClause; }

        void setShouldLearnClauses() { shouldLearnClause = true; }

        void clearShouldLearnClauses() { shouldLearnClause = false; }

        bool isSolverInParallelMode() const { return cube_and_conquer; }

        void setParallelMode() { cube_and_conquer = true; }

        void clearParallelMode() { cube_and_conquer = false; }

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
                return (shouldReset() or shallStop() or not isEmpty_query());
            });
        }

        void resetChannel() {
            clear_pulled_clauses();
            clear_learned_clauses();
            clear_current_header();
            if (not isEmpty_query())
                clear_queries();
            clearShouldStop();
            clearShallStop();
            clearReset();
        }

    };
}
#endif // PTPLIB_NET_CHANNEL_HPP