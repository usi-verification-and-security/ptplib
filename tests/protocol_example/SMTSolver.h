#pragma once
#include <PTPLib/Channel.hpp>
#include <PTPLib/Printer.hpp>
#include <PTPLib/Header.hpp>

class SMTSolver {
public:
    enum class Result { SAT, UNSAT, UNKNOWN };

private:
    PTPLib::Channel & channel;
    bool learnSomeClauses(std::vector<PTPLib::Net::Lemma> & learned_clauses);
    Result do_solve();
    Result result;
    PTPLib::synced_stream & stream;
    PTPLib::StoppableWatch & timer;
    bool color_enabled;
    double waiting_duration;
    std::atomic<std::thread::id> thread_id;

public:
    SMTSolver(PTPLib::Channel & ch, PTPLib::synced_stream & st, PTPLib::StoppableWatch & tm, const bool & ce, double wd )
    :
            channel (ch),
            result (Result::UNKNOWN),
            stream  (st),
            timer   (tm),
            color_enabled(ce),
            waiting_duration(wd)
    {}

    void setResult(Result res)     { result = res; }

    Result   getResult()  const    { return result; }

    PTPLib::Channel& getChannel() const    { return channel; }

    static int generate_rand(int min, int max);

    SMTSolver::Result search(char * smt_lib);

    void inject_clauses(PTPLib::map_solver_clause & pulled_clauses);

    void initialise_logic();

    void do_partition(const std::string & node, const std::string & pn);

    static std::string resultToString(SMTSolver::Result res);

};
