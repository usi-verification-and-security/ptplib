/*
 * Copyright (c) 2022, Antti Hyvarinen <antti.hyvarinen@gmail.com>
 * Copyright (c) 2022, Seyedmasoud Asadzadeh <seyedmasoud.asadzadeh@usi.ch>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef PTPLIB_CHANNEL_HPP
#define PTPLIB_CHANNEL_HPP

#include "PTPLib/Header.hpp"

#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <map>
#include <algorithm>
#include <chrono>
#include <queue>


class Channel {

    using td_t = const std::chrono::duration<double>;
    mutable std::mutex mutex;
    mutable std::condition_variable cv;

    typedef std::queue<std::pair<PTPLib::net::Header, std::string>> q_pair_header_str;
    q_pair_header_str queries;
    PTPLib::net::Header current_header;

    typedef std::map<std::string, std::vector<std::pair<std::string, int>>> m_str_vec_t;
    std::unique_ptr<m_str_vec_t> m_learned_clauses;
    std::unique_ptr<m_str_vec_t> m_pulled_clauses;

    std::atomic_bool requestStop;
    std::atomic_bool reset;
    bool isStopping;
    bool clauseShareMode;
    bool isFirstTime;
    int clauseLearnDuration;
    bool apiMode;

public:
    Channel()
    :
        requestStop         (false),
        reset               (false),
        isStopping          (false),
        clauseShareMode     (false),
        isFirstTime         (false),
        clauseLearnDuration (4000),
        apiMode             (false)
    {
        m_learned_clauses = std::make_unique<m_str_vec_t>();
        m_pulled_clauses = std::make_unique<m_str_vec_t>();
    }

    std::mutex & getMutex() { return mutex; }

    void insert_learned_clause(std::vector<std::pair<std::string, int>> & toPublish_clauses) {
        (*m_learned_clauses)[get_current_header().at(PTPLib::Param.NODE)].insert
        (
                std::end((*m_learned_clauses)[get_current_header().at(PTPLib::Param.NODE)]),
                std::begin(toPublish_clauses), std::end(toPublish_clauses)
        );
    }

    void insert_pulled_clause(std::vector<std::pair<std::string, int>> & toInject_clauses) {
        (*m_pulled_clauses)[get_current_header().at(PTPLib::Param.NODE)].insert
        (
                std::end((*m_pulled_clauses)[get_current_header().at(PTPLib::Param.NODE)]),
                std::begin(toInject_clauses), std::end(toInject_clauses)
        );
    }

    std::unique_ptr<m_str_vec_t> swap_learned_clauses() {
        auto out = std::make_unique<m_str_vec_t>();
        std::swap(out, m_learned_clauses);
        return out;
    };

    std::unique_ptr<m_str_vec_t> swap_pulled_clauses() {
        auto out = std::make_unique<m_str_vec_t>();
        std::swap(out, m_pulled_clauses);
        return out;
    };

    void clear_queries() {
        q_pair_header_str empty_q;
        std::swap(get_queris(), empty_q);
    }

    size_t size_query() const { return queries.size(); }

    bool isEmpty_query() const { return queries.empty(); }

    q_pair_header_str & get_queris() { return queries; }

    std::pair<PTPLib::net::Header, std::string> pop_front_query() {
        std::pair<PTPLib::net::Header, std::string> tmp_p(std::move(queries.front()));
        queries.pop();
        return tmp_p;
    }

    void push_back_query(std::pair<PTPLib::net::Header, std::string> && hd) { queries.push(std::move(hd)); }

    void set_current_header(PTPLib::net::Header hd) { current_header = hd.copy(hd.keys()); }

    PTPLib::net::Header & get_current_header()  { return current_header; }

    size_t size() const { return m_learned_clauses->size(); }

    auto cbegin() const { return m_learned_clauses->cbegin(); }

    auto cend() const { return m_learned_clauses->cend(); }

    auto begin() const { return m_learned_clauses->begin(); }

    auto end() const { return m_learned_clauses->end(); }

    void notify_one() { cv.notify_one(); }

    void notify_all() { cv.notify_all(); }

    void clear_learned_clauses() { m_learned_clauses->clear(); }

    void clear_pulled_clauses() { m_pulled_clauses->clear(); }

    bool empty() const { return (m_learned_clauses ?  m_learned_clauses->empty() : false); }

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

    bool wait_for(std::unique_lock<std::mutex> & lock, const td_t & timeout_duration) {
        return cv.wait_for(lock, timeout_duration, [&] { return shouldReset(); });
    }

    void wait_for_queue_or_solver(std::unique_lock<std::mutex> & lock, const td_t & timeout_duration) {
        cv.wait_for(lock, timeout_duration, [&] { return (shouldReset() or not isEmpty_query() or shallStop()); });
    }

    void wait(std::unique_lock<std::mutex> & lock) {
        cv.wait(lock, [&] { return (shouldReset() or not isEmpty_query() or shallStop()); });
    }

    void resetChannel() {
//        clear_pulled_clauses();
//        clear_learned_clauses();
        clearShouldStop();
        clearReset();
        clearClauseShareMode();
        setApiMode();
    }

};

#endif // PTPLIB_CHANNEL_HPP