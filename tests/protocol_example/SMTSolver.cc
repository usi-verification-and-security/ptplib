#include "SMTSolver.h"

#include <iostream>
#include <string>
#include <cassert>
#include <random>
#include <mutex>

bool SMTSolver::learnSomeClauses(std::vector<std::pair<std::string ,int>> & learned_clauses) {
    int rand_number = waiting_duration ? waiting_duration * (100) : SMTSolver::generate_rand(0, 2000);
    if (rand_number % 5 == 0)
        return false;
    for (int i = 0; i < rand_number ; ++i) {
        learned_clauses.emplace_back(std::make_pair("assert("+to_string(i)+")",i % 10));
    }
    return not learned_clauses.empty();
}

SMTSolver::Result SMTSolver::do_solve() {

    int random_n = waiting_duration ? waiting_duration * (100) : SMTSolver::generate_rand(1000, 2000);
    std::this_thread::sleep_for(std::chrono::milliseconds (random_n));
    std::vector<std::pair<std::string ,int>>  toPublishClauses;
    if (learnSomeClauses(toPublishClauses))
    {
        std::scoped_lock<std::mutex> lk(channel.getMutex());
        stream.println(color_enabled ? PTPLib::Color::FG_Green : PTPLib::Color::FG_DEFAULT,
                       "[t SEARCH ] -> add learned clauses to channel buffer, Size : ",
                       toPublishClauses.size());
        channel.insert_learned_clause(toPublishClauses);
        return random_n % 25 == 0 ? Result::UNSAT : Result::UNKNOWN;
    }

    return random_n % 30 == 0 ? Result::SAT : Result::UNKNOWN;
}

void SMTSolver::search(std::string & smt_lib) {
    assert(not smt_lib.empty());
    smt_lib.clear();
    result = Result::UNKNOWN;
    while (result == Result::UNKNOWN and not channel.shouldStop())
    {
        result = do_solve();
        if (result != Result::UNKNOWN) {
            std::scoped_lock<std::mutex> lk(channel.getMutex());
            channel.setShallStop();
            stream.println(color_enabled ? PTPLib::Color::FG_Green : PTPLib::Color::FG_DEFAULT,
                           "[t SEARCH ] -> set shall stop");
            channel.notify_all();
        }
    }
    stream.println(color_enabled ? PTPLib::Color::FG_Green : PTPLib::Color::FG_DEFAULT,
                   "[t SEARCH ] -> solver exited with ", SMTSolver::resultToString(result));
}

void SMTSolver::inject_clauses(std::map<std::string, std::vector<std::pair<std::string, int>>> & pulled_clauses)
{
    stream.println(color_enabled ? PTPLib::Color::FG_Cyan : PTPLib::Color::FG_DEFAULT,
                   "[t COMMUNICATOR ] -> inject pulled clause ");
    for (auto &clauses : pulled_clauses) {
        std::this_thread::sleep_for(std::chrono::milliseconds (int(waiting_duration ? waiting_duration * (100) : clauses.second.size())));
    }
}
void SMTSolver::initialise_logic()
{
    timer.start();
    stream.println(color_enabled ? PTPLib::Color::FG_Cyan : PTPLib::Color::FG_DEFAULT,
                   "[t COMMUNICATOR ] -> initialising the logic, time: ", timer.elapsed_time_milliseconds());
    timer.reset();
}

void SMTSolver::do_partition(const std::string & node, const std::string & pn)
{
    std::string str = "[t COMMUNICATOR ] -> doing partition at "+ node + " --- partitions: "+ pn;
    PTPLib::PrintStopWatch psw(str, stream,
                               color_enabled ? PTPLib::Color::FG_Cyan : PTPLib::Color::FG_DEFAULT);
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