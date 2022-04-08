#pragma once
#include "Communicator.cc"

#include <PTPLib/net/Channel.hpp>
#include <PTPLib/net/Header.hpp>
#include <PTPLib/common/PartitionConstant.hpp>
#include <PTPLib/threads/ThreadPool.hpp>

class Listener {
    PTPLib::net::Channel channel;
    PTPLib::threads::ThreadPool th_pool;
    Communicator communicator;
    PTPLib::common::synced_stream & stream;
    PTPLib::common::StoppableWatch timer;
    bool color_enabled;
    int nCommands;
    int instanceNum;
    double waiting_duration;
    std::atomic<std::thread::id> memory_thread_id;
    std::atomic<std::thread::id> push_thread_id;
    std::atomic<std::thread::id> pull_thread_id;
public:

    Listener(PTPLib::common::synced_stream & ss, const bool & ce, double wd)
    :
        th_pool(ss, "thread_pool", std::thread::hardware_concurrency() - 1),
        communicator(channel, ss, ce, wd, th_pool),
        stream(ss),
        color_enabled(ce),
        waiting_duration(wd)
    {}

    void set_eventGen_stat(int inc, int nc) {
        instanceNum = inc;
        nCommands = nc;
    }

    void notify_reset();

    void worker(PTPLib::common::TASK tname, double seed, double td_min, double td_max);

    void push_to_pool(PTPLib::common::TASK tname, double seed = 0, double td_min = 0, double td_max = 0);

    PTPLib::net::SMTS_Event generate_event(int counter, int solve_time);

    void queue_event(PTPLib::net::SMTS_Event && header_payload);

    void pull_clause_worker(double seed, double n_min, double n_max);

    void push_clause_worker(double seed, double n_min, double n_max);

    bool read_lemma(std::vector<PTPLib::net::Lemma> & lemmas, PTPLib::net::Header & header);

    bool write_lemma(std::unique_ptr<PTPLib::net::map_solver_clause> const & lemmas, PTPLib::net::Header & header);

    void memory_checker();

    PTPLib::net::Channel & getChannel() { return channel;};

    PTPLib::threads::ThreadPool & getPool() { return th_pool; }

    void periodic_clauseLearning_worker();
};
