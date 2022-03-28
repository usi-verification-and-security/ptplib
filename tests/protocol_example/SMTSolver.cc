#include <iostream>
#include <string>
#include "SMTSolver.h"
#include <stdlib.h>
#include <random>
#include <mutex>
#include <thread>


bool SMTSolver::learnSomeClauses(std::vector<std::pair<std::string ,int>> & learned_clauses) {
    int rand_number = SMTSolver::generate_rand(0, 1000);
    if (rand_number % 5 == 0)
        return false;
    for (int i = 0; i < rand_number ; ++i) {
        learned_clauses.emplace_back(std::make_pair("assert("+to_string(i)+")",i % 10));
    }
    return not learned_clauses.empty();
}

SMTSolver::Result SMTSolver::do_solve() {

    int random_n = SMTSolver::generate_rand(1000, 2000);
    std::this_thread::sleep_for(std::chrono::milliseconds (random_n));
    std::vector<std::pair<std::string ,int>>  toPublishClauses;
    if (learnSomeClauses(toPublishClauses))
    {
        std::scoped_lock<std::mutex> lk(channel.getMutex());
        stream.println(color_enabled ? PTPLib::Color::FG_Green : PTPLib::Color::FG_DEFAULT,
                       "[t SEARCH ] -> add learned clauses to channel buffer, Size : ",
                       toPublishClauses.size());
        channel.insert_learned_clause(toPublishClauses);
        return random_n % 10 == 0 ? Result::UNSAT : Result::UNKNOWN;
    }

    return random_n % 15 == 0 ? Result::SAT : Result::UNKNOWN;
}

void SMTSolver::search(const std::string & smt_lib) {
    assert(not smt_lib.empty());
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

void SMTSolver::inject_clauses()
{
    stream.println(color_enabled ? PTPLib::Color::FG_Cyan : PTPLib::Color::FG_DEFAULT,
                   "[t COMMUNICATOR ] -> inject pulled clause ");
    for ( auto &clauses : channel.extract_pulled_clauses() ) {
        std::this_thread::sleep_for(std::chrono::milliseconds (clauses.second.size()));
    }
}
void SMTSolver::initialise_logic()
{
    stream.println(color_enabled ? PTPLib::Color::FG_Cyan : PTPLib::Color::FG_DEFAULT,
                   "[t COMMUNICATOR ] -> initialising the logic ");
}

void SMTSolver::do_partition(const std::string & node, const std::string & pn)
{
    stream.println(color_enabled ? PTPLib::Color::FG_Cyan : PTPLib::Color::FG_DEFAULT,
                   "[t COMMUNICATOR ] -> doing patition at ", node, " --- partitions: ",pn);
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