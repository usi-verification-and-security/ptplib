#pragma once
#include "Communicator.cc"

#include <PTPLib/Channel.hpp>
#include <PTPLib/Header.hpp>
#include <PTPLib/PartitionConstant.hpp>
#include <PTPLib/ThreadPool.hpp>

class Listener {
    Channel channel;
    PTPLib::ThreadPool listener_pool;
    Communicator communicator;
    PTPLib::synced_stream & stream;
    PTPLib::StoppableWatch timer;
    bool color_enabled;
    int nCommands;
    int instanceNum;
    double waiting_duration;

public:

    Listener(PTPLib::synced_stream & ss, const bool & ce, double wd)
        :
        listener_pool("listener_pool", std::thread::hardware_concurrency()-1),
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

    std::pair<PTPLib::net::Header, std::string> read_event(int counter, int solve_time);

    void queue_event(std::pair<PTPLib::net::Header, std::string> && header_payload);

    void pull_clause_worker(double seed, double n_min, double n_max);

    void push_clause_worker(double seed, double n_min, double n_max);

    bool read_lemma(std::vector<std::pair<std::string, int>> & lemmas, PTPLib::net::Header & header);

    bool write_lemma(std::unique_ptr<std::map<std::string, std::vector<std::pair<std::string, int>>>> const & lemmas, PTPLib::net::Header & header);

    void memory_checker();

    Channel & getChannel() { return channel;};

    PTPLib::ThreadPool & getPool() { return listener_pool; }
};
