#pragma once
#include "Communicator.cc"

#include <PTPLib/Channel.hpp>
#include <PTPLib/Header.hpp>
#include <PTPLib/PartitionConstant.hpp>
#include <PTPLib/ThreadPool.hpp>

class Listener {
    PTPLib::Channel channel;
    PTPLib::ThreadPool listener_pool;
    Communicator communicator;
    PTPLib::synced_stream & stream;
    PTPLib::StoppableWatch timer;
    bool color_enabled;
    int nCommands;
    int instanceNum;
    double waiting_duration;
    std::atomic<std::thread::id> memory_thread_id;
    std::atomic<std::thread::id> push_thread_id;
    std::atomic<std::thread::id> pull_thread_id;
public:

    Listener(PTPLib::synced_stream & ss, const bool & ce, double wd)
        :
        listener_pool(ss, "listener_pool", std::thread::hardware_concurrency() - 1),
        communicator(channel, ss, ce, wd, listener_pool),
        stream(ss),
        color_enabled(ce),
        waiting_duration(wd)
    {}

    void set_eventGen_stat(int inc, int nc) {
        instanceNum = inc;
        nCommands = nc;
    }

    void notify_reset();

    void worker(PTPLib::TASK tname, double seed, double td_min, double td_max);

    void push_to_pool(PTPLib::TASK tname, double seed = 0, double td_min = 0, double td_max = 0);

    PTPLib::smts_event read_event(int counter, int solve_time);

    void queue_event(PTPLib::smts_event && header_payload);

    void pull_clause_worker(double seed, double n_min, double n_max);

    void push_clause_worker(double seed, double n_min, double n_max);

    bool read_lemma(std::vector<PTPLib::Net::Lemma> & lemmas, PTPLib::net::Header & header);

    bool write_lemma(std::unique_ptr<PTPLib::map_solver_clause> const & lemmas, PTPLib::net::Header & header);

    void memory_checker();

    PTPLib::Channel & getChannel() { return channel;};

    PTPLib::ThreadPool & getPool() { return listener_pool; }
};
