#pragma once
#include <PTPLib/net/Channel.hpp>
#include <PTPLib/net/Header.hpp>
#include <PTPLib/common/PartitionConstant.hpp>
#include <PTPLib/threads/ThreadPool.hpp>

#include <unistd.h>
#include <chrono>

#include "SMTSolver.cc"

class Communicator {
    PTPLib::net::Channel<PTPLib::net::SMTS_Event, PTPLib::net::Lemma> & channel;
    SMTSolver solver;
    PTPLib::common::synced_stream & stream;
    PTPLib::common::StoppableWatch timer;
    bool color_enabled;
    PTPLib::threads::ThreadPool & th_pool;
    std::future<SMTSolver::Result> future;
    std::atomic<std::thread::id> thread_id;

public:

    Communicator(PTPLib::net::Channel<PTPLib::net::SMTS_Event, PTPLib::net::Lemma> & ch, PTPLib::common::synced_stream & ss, const bool & ce, double seed, PTPLib::threads::ThreadPool & th)
        :
        channel(ch),
        solver(ch, ss, timer, ce, seed),
        stream(ss),
        color_enabled(ce),
        th_pool(th)
     {}

    bool execute_event(const PTPLib::net::SMTS_Event & event, bool & shouldUpdateSolverAddress);

    bool setStop(PTPLib::net::SMTS_Event & event);

    void communicate_worker();

    PTPLib::net::Channel<PTPLib::net::SMTS_Event, PTPLib::net::Lemma> & getChannel() { return channel;};

};
