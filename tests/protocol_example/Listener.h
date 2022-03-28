#pragma once
#include "Communicator.cc"

#include <PTPLib/Channel.hpp>
#include <PTPLib/Header.hpp>
#include <PTPLib/PartitionConstant.hpp>
#include <PTPLib/ThreadPool.hpp>

class Listener {
    Channel channel;
    Communicator communicator;
    PTPLib::synced_stream & stream;
    PTPLib::StoppableWatch timer;
    bool color_enabled;
    int nCommands;
    int instanceNum;


public:
    PTPLib::ThreadPool listener_pool;
    Listener(PTPLib::synced_stream & ss, const bool & ce)
        :
        communicator(channel, ss, ce),
        stream(ss),
        color_enabled(ce),
        listener_pool(std::thread::hardware_concurrency() - 2) {}


    void set_numberOf_Command(int inc, int nc) { instanceNum = inc; nCommands = nc; }

    void notify_reset();

    void worker(PTPLib::WORKER tname, int seed, int td_min, int td_max);

    void push_to_pool(PTPLib::WORKER tname, int seed = 0, int td_min = 0, int td_max = 0);

    std::pair<PTPLib::net::Header, std::string> read_event(int counter, int solve_time);

    void queue_event(std::pair<PTPLib::net::Header, std::string> && header_payload);

    void pull_clause_worker(int seed, int n_min, int n_max);

    void push_clause_worker(int seed, int n_min, int n_max);

    bool read_lemma(std::vector<std::pair<std::string, int>> & lemmas, PTPLib::net::Header & header);

    void write_lemma(std::map<std::string, std::vector<std::pair<std::string, int>>> const & lemmas, PTPLib::net::Header & header);

    void mem_check(const std::string & max_memory);

    Channel & getChannel() { return channel;};

};
