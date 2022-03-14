/*
 * Copyright (c) 2022, Antti Hyvarinen <antti.hyvarinen@gmail.com>
 * Copyright (c) 2022, Seyedmasoud Asadzadeh <seyedmasoud.asadzadeh@usi.ch>
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef PTPLIB_CHANNEL_HPP
#define PTPLIB_CHANNEL_HPP

#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <deque>
#include <map>
#include <algorithm>

class Channel {

    using td_t = const std::chrono::duration<double>;
    mutable std::mutex mutex;
    mutable std::condition_variable cv;

    std::deque<std::string> queries;
    std::atomic_bool requestStop;
    std::atomic_bool stop;

    bool clauseShareMode;
    bool isFirstTime;
    bool injectClause;
    int clauseLearnDuration;
    std::string currentSolverAddress;
    bool apiMode;

    std::map<std::string, std::vector<std::pair<std::string, int>>> clauses;

public:
    Channel() : requestStop(false), stop(false), clauseShareMode(false), isFirstTime(false), injectClause(false),
                clauseLearnDuration(4000), apiMode(false)
    {}

    void assign(std::vector<std::pair<std::string, int>> && toPublishTerms)
    {
        clauses[currentSolverAddress].insert(std::end(clauses[currentSolverAddress]),
                                             std::begin(toPublishTerms), std::end(toPublishTerms));
    }

    std::map<std::string, std::vector<std::pair<std::string, int>>> getClauses() { return std::move(clauses); };

    std::mutex & getMutex() { return mutex; }

    size_t size() const { return clauses.size(); }

    auto cbegin() const { return clauses.cbegin(); }

    auto cend() const { return clauses.cend(); }

    auto begin() const { return clauses.begin(); }

    auto end() const { return clauses.end(); }

    void notify_one() { cv.notify_one(); }

    void notify_all() { cv.notify_all(); }

    void clear() { clauses.clear(); }

    bool empty() const { return clauses.empty(); }

    bool shouldTerminate() const { return stop; }

    void setTerminate() { stop = true; }

    void clearTerminate() { stop = false; }

    bool shouldStop() const { return requestStop; }

    void setShouldStop() { requestStop = true; }

    void clearShouldStop() { requestStop = false; }

    bool isClauseShareMode() const { return clauseShareMode; }

    void setClauseShareMode() { clauseShareMode = isFirstTime = true; }

    void clearClauseShareMode() { clauseShareMode = false; }

    void pop_front_query() { queries.pop_front(); }

    size_t size_query() const { return queries.size(); }

    bool isEmpty_query() const { return queries.empty(); }

    void clear_query() { queries.clear(); }

    auto cbegin_query() const { return queries.cbegin(); }

    auto cend_query() const { return queries.cend(); }

    auto get_queris() const { return queries; }

    bool shouldInjectClause() const { return injectClause; }

    void setInjectClause() { injectClause = true; }

    void clearInjectClause() { injectClause = false; }

    bool getFirstTimeLearnClause() const { return isFirstTime; }

    void clearFirstTimeLearnClause() { isFirstTime = false; }

    void setClauseLearnDuration(int cld) { clauseLearnDuration = cld; }

    int getClauseLearnDuration() const { return clauseLearnDuration; }

    void push_back_query(std::string str) { queries.push_back(str); }

    bool isApiMode() const { return apiMode; }

    void setApiMode() { apiMode = true; }

    void clearApiMode() { apiMode = false; }

    void setCurrentSolverAddress(std::string csa) { currentSolverAddress = csa; }

    std::string getCurrentSolverAddress() const { return currentSolverAddress; }

    bool waitFor(std::unique_lock<std::mutex> & lock, const td_t & timeout_duration)
    {
        return cv.wait_for(lock, timeout_duration, [&] { return shouldTerminate(); });
    }

    void waitQueryOrTermination(std::unique_lock<std::mutex> & lock)
    {
        cv.wait(lock, [&] { return (shouldTerminate() or not isEmpty_query()); });
    }

    void resetChannel() {
        clear();
        clear_query();
        clearShouldStop();
        clearTerminate();
        clearClauseShareMode();
        clearInjectClause();
        setApiMode();
    }

};

#endif PTPLIB_CHANNEL_HPP