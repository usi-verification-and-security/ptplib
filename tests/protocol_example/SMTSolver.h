#pragma once
#include <PTPLib/net/Channel.hpp>
#include <PTPLib/common/Printer.hpp>
#include <PTPLib/net/Header.hpp>

class SMTSolver {
public:
    enum class Result { SAT, UNSAT, UNKNOWN };

private:
    PTPLib::net::Channel<PTPLib::net::SMTS_Event, PTPLib::net::Lemma> & channel;
    bool learnSomeClauses(std::vector<PTPLib::net::Lemma> & learned_clauses);
    Result do_solve();
    Result result;
    PTPLib::common::synced_stream & stream;
    PTPLib::common::StoppableWatch & timer;
    bool color_enabled;
    double waiting_duration;
    std::atomic<std::thread::id> thread_id;

public:
    SMTSolver(PTPLib::net::Channel<PTPLib::net::SMTS_Event, PTPLib::net::Lemma> & ch, PTPLib::common::synced_stream & st, PTPLib::common::StoppableWatch & tm, const bool & ce, double wd)
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

    PTPLib::net::Channel<PTPLib::net::SMTS_Event, PTPLib::net::Lemma> & getChannel() const    { return channel; }

    static int generate_rand(int min, int max);

    SMTSolver::Result search(char * smt_lib);

    void inject_clauses(PTPLib::net::map_solverBranch_lemmas & pulled_clauses);

    void initialise_logic();

    void do_partition(const std::string & node, const std::string & pn);

    static std::string resultToString(SMTSolver::Result res);

};
