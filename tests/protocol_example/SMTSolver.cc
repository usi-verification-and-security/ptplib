#include "SMTSolver.h"

#include <PTPLib/common/Exception.hpp>

#include <iostream>
#include <string>
#include <cassert>
#include <random>
#include <mutex>

bool SMTSolver::learnSomeClauses(std::vector<PTPLib::net::Lemma> & learned_clauses) {
    int rand_number = waiting_duration ? waiting_duration * (100) : SMTSolver::generate_rand(0, 2000);
    if (rand() % 10 == 0)
        return false;

    for (int i = 0; i < rand_number ; ++i) {
        learned_clauses.emplace_back(PTPLib::net::Lemma("assert(" + std::to_string(i) + ")", i % 10));
    }
    return not learned_clauses.empty();
}

SMTSolver::Result SMTSolver::do_solve() {

    int random_n = waiting_duration ? waiting_duration * (100) : SMTSolver::generate_rand(1000, 2000);
    std::this_thread::sleep_for(std::chrono::milliseconds (random_n));
    std::vector<PTPLib::net::Lemma>  toPublishClauses;
    if (learnSomeClauses(toPublishClauses))
    {
        std::unique_lock<std::mutex> lk(channel.getMutex());
        assert([&]() {
            if (thread_id != std::this_thread::get_id())
                throw PTPLib::common::Exception(__FILE__, __LINE__, "search has inconsistent thread id");

            if (not lk.owns_lock()) {
                throw PTPLib::common::Exception(__FILE__, __LINE__, "search can't take the lock");
            }
            return true;
        }());
        stream.println(color_enabled ? PTPLib::common::Color::FG_Green : PTPLib::common::Color::FG_DEFAULT,
                       "[t SEARCH ] -> add learned clauses to channel buffer, Size : ",
                       toPublishClauses.size());
        channel.insert_learned_clause(std::move(toPublishClauses));
        lk.unlock();
        return Result::UNKNOWN;
    }
    else
        return std::rand() < RAND_MAX / 2 ? Result::SAT : Result::UNSAT;
}

SMTSolver::Result SMTSolver::search(char * smt_lib) {
    thread_id = std::this_thread::get_id();
    assert (smt_lib);
    stream.println(color_enabled ? PTPLib::common::Color::FG_Green : PTPLib::common::Color::FG_DEFAULT,
                   "[t SEARCH ] -> instance: ", smt_lib);
    Result solver_result = Result::UNKNOWN;
    while (not channel.shouldStop())
    {
        solver_result = do_solve();
        if (solver_result != Result::UNKNOWN) {
            std::unique_lock<std::mutex> lk(channel.getMutex());
            assert([&]() {
                if (thread_id != std::this_thread::get_id())
                    throw PTPLib::common::Exception(__FILE__, __LINE__, "search has inconsistent thread id");

                if (not lk.owns_lock()) {
                    throw PTPLib::common::Exception(__FILE__, __LINE__, "search can't take the lock");
                }
                return true;
            }());
            channel.setShallStop();
            channel.notify_all();
            lk.unlock();

            stream.println(color_enabled ? PTPLib::common::Color::FG_Green : PTPLib::common::Color::FG_DEFAULT,
                           "[t SEARCH ] -> set shall stop");
            break;
        }
    }
    stream.println(color_enabled ? PTPLib::common::Color::FG_Green : PTPLib::common::Color::FG_DEFAULT,
                   "[t SEARCH ] -> solver exited with ", SMTSolver::resultToString(solver_result));
    return solver_result;
}

void SMTSolver::inject_clauses(PTPLib::net::map_solver_clause & pulled_clauses)
{
    stream.println(color_enabled ? PTPLib::common::Color::FG_Cyan : PTPLib::common::Color::FG_DEFAULT,
                   "[t COMMUNICATOR ] -> inject pulled clause ");
    for (auto &clauses : pulled_clauses) {
        std::this_thread::sleep_for(std::chrono::milliseconds (int(waiting_duration ? waiting_duration * (100) : clauses.second.size())));
    }
}
void SMTSolver::initialise_logic()
{
    timer.start();
    stream.println(color_enabled ? PTPLib::common::Color::FG_Cyan : PTPLib::common::Color::FG_DEFAULT,
                   "[t COMMUNICATOR ] -> initialising the logic, time: ", timer.elapsed_time_milliseconds());
    timer.reset();
}

void SMTSolver::do_partition(const std::string & node, const std::string & pn)
{
    PTPLib::common::PrintStopWatch psw("[t COMMUNICATOR ] -> doing partition at "+ node + " --- partitions: "+ pn, stream,
                               color_enabled ? PTPLib::common::Color::FG_Cyan : PTPLib::common::Color::FG_DEFAULT);
}

int SMTSolver::generate_rand(int min, int max)
{
    std::uniform_int_distribution<int> un_dist(min, max);
    std::random_device rd;
    return un_dist(rd);
}

std::string SMTSolver::resultToString(SMTSolver::Result res) {
    if (res == SMTSolver::Result::UNKNOWN)
        return "unknown";
    else if (res == SMTSolver::Result::SAT)
        return "sat";
    else if (res == SMTSolver::Result::UNSAT)
        return "unsat";
    else return "undefined";
}