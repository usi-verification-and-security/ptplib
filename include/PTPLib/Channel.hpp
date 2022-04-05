/*
 * Copyright (c) 2022, Antti Hyvarinen <antti.hyvarinen@gmail.com>
 * Copyright (c) 2022, Seyedmasoud Asadzadeh <seyedmasoud.asadzadeh@usi.ch>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef PTPLIB_CHANNEL_HPP
#define PTPLIB_CHANNEL_HPP

#include "PTPLib/Header.hpp"
#include "PTPLib/Net/Lemma.hpp"

#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <map>
#include <algorithm>
#include <chrono>
#include <queue>
#include <cassert>

namespace PTPLib {

    using smts_event = std::pair<PTPLib::net::Header, std::string>;
    using queue_event = std::queue<smts_event>;
    using map_solver_clause = std::map<std::string, std::vector<PTPLib::Net::Lemma>>;
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
        bool isStopping;
        bool clauseShareMode;
        bool isFirstTime;
        int clauseLearnDuration;
        bool apiMode;

    public:
        Channel()
                :
                requestStop(false),
                reset(false),
                isStopping(false),
                clauseShareMode(false),
                isFirstTime(false),
                clauseLearnDuration(4000),
                apiMode(false) {
            m_learned_clauses = std::make_unique<map_solver_clause>();
            m_pulled_clauses = std::make_unique<map_solver_clause>();
        }

        std::mutex & getMutex() { return mutex; }

        void insert_learned_clause(std::vector<PTPLib::Net::Lemma> && toPublish_clauses) {
            assert(not get_current_header().at(PTPLib::Param.NODE).empty());
            (*m_learned_clauses)[get_current_header().at(PTPLib::Param.NODE)].insert
                    (
                            std::end((*m_learned_clauses)[get_current_header().at(PTPLib::Param.NODE)]),
                            std::begin(toPublish_clauses), std::end(toPublish_clauses)
                    );
        }

        void insert_pulled_clause(std::vector<PTPLib::Net::Lemma> && toInject_clauses) {
            assert(not get_current_header().at(PTPLib::Param.NODE).empty());
            (*m_pulled_clauses)[get_current_header().at(PTPLib::Param.NODE)].insert
                    (
                            std::end((*m_pulled_clauses)[get_current_header().at(PTPLib::Param.NODE)]),
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

        smts_event pop_front_query() {
            smts_event tmp_p(std::move(queries.front()));
            queries.pop();
            return tmp_p;
        }

        PTPLib::net::Header & front_queries() { return queries.front().first; }

        void push_back_query(smts_event && hd) {
            assert((not hd.first.at(PTPLib::Param.NODE).empty()) and (not hd.first.at(PTPLib::Param.NAME).empty()));
            queries.push(std::move(hd));
        }

        void set_current_header(PTPLib::net::Header & hd) {
            assert((not hd.at(PTPLib::Param.NODE).empty()) and (not hd.at(PTPLib::Param.NAME).empty()));
            current_header = hd.copy(hd.keys());
        }

        PTPLib::net::Header & get_current_header() { return current_header; }

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

        void setClauseShareMode() { clauseShareMode = isFirstTime = true; }

        void clearClauseShareMode() { clauseShareMode = false; }

        bool getFirstTimeLearnClause() const { return isFirstTime; }

        void clearFirstTimeLearnClause() { isFirstTime = false; }

        void setClauseLearnDuration(int cld) { clauseLearnDuration = cld; }

        int getClauseLearnDuration() const { return clauseLearnDuration; }

        bool isApiMode() const { return apiMode; }

        void setApiMode() { apiMode = true; }

        void clearApiMode() { apiMode = false; }

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
            clearClauseShareMode();
            setApiMode();
        }

    };
}
#endif // PTPLIB_CHANNEL_HPP