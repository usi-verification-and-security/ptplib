#include "Listener.h"
#include "SMTSolver.h"

#include <iostream>
#include <string>


void Listener::mem_check(const std::string & max_memory)
{
    size_t limit = atoll(max_memory.c_str());
    if (limit == 0)
        return;

    while (true) {
//        size_t cmem = current_memory();
        size_t cmem = 1;
        if (cmem > limit * 1024 * 1024) {
            stream.println(color_enabled ? PTPLib::Color::FG_Yellow : PTPLib::Color::FG_DEFAULT, "[t max memory checker ] -> max memory reached: ", std::to_string(cmem));
                exit(-1);
        }
        stream.println(color_enabled ? PTPLib::Color::FG_Yellow : PTPLib::Color::FG_DEFAULT, "[t max memory checker ] -> ", std::to_string(cmem));
        std::unique_lock<std::mutex> lk(channel.getMutex());
        if (channel.wait_for(lk, std::chrono::seconds (10)))
            break;
    }
}

void Listener::worker(PTPLib::WORKER wname, int seed, int td_min, int td_max) {

    switch (wname) {

        case PTPLib::WORKER::COMMUNICATION:
            communicator.communicate_worker();
            break;

        case PTPLib::WORKER::CLAUSEPUSH:
            push_clause_worker(seed, td_min, td_max);
            break;

        case PTPLib::WORKER::CLAUSEPULL:
            pull_clause_worker(seed, td_min, td_max);
            break;

        case PTPLib::WORKER::MEMORYCHECK:
            mem_check("4000");
            break;

        default:
            break;
    }

}

void Listener::push_to_pool(PTPLib::WORKER wname, int seed, int td_min, int td_max )
{
    listener_pool.push_task([this, wname, seed, td_min, td_max]
    {
            worker(wname, seed, td_min, td_max);
    });
}

void Listener::notify_reset()
{
    channel.setReset();
    channel.notify_all();
}

void Listener::queue_event(std::pair<PTPLib::net::Header, std::string> && header_payload)
{
    getChannel().push_back_query(std::move(header_payload));
    getChannel().notify_all();
}

std::pair<PTPLib::net::Header, std::string> Listener::read_event(int counter, int time_passed)
{
    std::string payload;
    int rDuration = 0;
    if (counter > 1)
        rDuration = SMTSolver::generate_rand(0,5000);

    std::this_thread::sleep_for(std::chrono::milliseconds (rDuration));

    PTPLib::net::Header header;
    header.emplace(PTPLib::Param.NAME, "instance"+ to_string(instanceNum) +".smt2");
    header.emplace(PTPLib::Param.NODE, "[" + to_string(counter) + "]");
    header.emplace(PTPLib::Param.QUERY, "(check-sat)");
    if (time_passed > nCommands * 10) {
        header.emplace(PTPLib::Param.COMMAND, PTPLib::Command.STOP);
        return std::make_pair(header, payload);
    }
    else if (counter == nCommands) {
        std::this_thread::sleep_for(std::chrono::seconds ((nCommands * 5) - time_passed));
        header.emplace(PTPLib::Param.COMMAND, PTPLib::Command.STOP);
        return std::make_pair(header, payload);
    }
    else if (counter == 1) {
        header.emplace(PTPLib::Param.COMMAND, PTPLib::Command.SOLVE);
        payload = "solve( " + header.at(PTPLib::Param.NAME) + " )";
    }

    else if (counter % 2 == 0) {
        header.emplace(PTPLib::Param.COMMAND, PTPLib::Command.INCREMENTAL);
        header.emplace(PTPLib::Param.NODE_, "[" + to_string(counter + 1) + "]");
        payload = "move ( " + header.at(PTPLib::Param.NAME) + " )";
    }

    else {
        header.emplace(PTPLib::Param.COMMAND, PTPLib::Command.PARTITION);
        header.emplace(PTPLib::Param.PARTITIONS, "2");
    }

    return std::make_pair(header, payload);
}

void Listener::push_clause_worker(int seed, int min, int max)
{
    std::this_thread::sleep_for(std::chrono::seconds (1));
    int pushDuration = min + ( seed % ( max - min + 1 ) );
    stream.println(color_enabled ? PTPLib::Color::FG_Blue : PTPLib::Color::FG_DEFAULT,
                   "[t PUSH -> timout : ", pushDuration," ms");
    std::chrono::duration<double> wakeupAt = std::chrono::milliseconds (pushDuration);
    std::unique_ptr<std::map<std::string, std::vector<std::pair<std::string, int>>>> m_clauses;
    while (not getChannel().shouldReset()) {
        PTPLib::PrintStopWatch psw("[t PUSH ] -> wait and write duration: ", stream, PTPLib::Color::Code::FG_Blue);
        std::unique_lock<std::mutex> lk(getChannel().getMutex());
        if (not getChannel().wait_for(lk, wakeupAt))
        {
            if (getChannel().shouldReset())
                break;

            else if (not getChannel().empty())
            {
                m_clauses = getChannel().swap_learned_clauses();
                auto header = getChannel().get_current_header();
                lk.unlock();

                write_lemma(m_clauses, header);
                m_clauses->clear();
            }
            else stream.println(color_enabled ? PTPLib::Color::FG_Blue : PTPLib::Color::FG_DEFAULT,
                                "[t PUSH ] -> Channel empty!");
        }
        else
            break;
    }
}

void Listener::pull_clause_worker(int seed, int min, int max)
{
    std::this_thread::sleep_for(std::chrono::seconds (1));
    int pullDuration = min + ( seed % ( max - min + 1 ) );
    stream.println(color_enabled ? PTPLib::Color::FG_Black : PTPLib::Color::FG_DEFAULT,
                   "[t PULL -> timout : ", pullDuration," ms");
    std::chrono::duration<double> wakeupAt = std::chrono::milliseconds (pullDuration);
    while (true) {
        if (getChannel().shouldReset()) break;
        std::unique_lock<std::mutex> lk(getChannel().getMutex());
        PTPLib::PrintStopWatch psw("[t PULL ] -> wait and read duration: ", stream, PTPLib::Color::Code::FG_Black);
        if (not getChannel().wait_for(lk, wakeupAt))
        {
            auto header = channel.get_current_header();
            lk.unlock();

            std::vector<std::pair<std::string, int>> lemmas;
            if (this->read_lemma(lemmas, header))
            {
                if (getChannel().shouldReset())
                    break;
                stream.println(color_enabled ? PTPLib::Color::FG_Black : PTPLib::Color::FG_DEFAULT,
                               "[t PULL ] -> pulled learned clauses copied to channel buffer, Size: ",
                               lemmas.size());
                {
                    std::scoped_lock<std::mutex> _lk(getChannel().getMutex());
                    channel.insert_pulled_clause(lemmas);

                    header[PTPLib::Param.COMMAND] = PTPLib::Command.CLAUSEINJECTION;
                    queue_event(std::make_pair(header, ""));
                }
                stream.println(color_enabled ? PTPLib::Color::FG_Black : PTPLib::Color::FG_DEFAULT,
                               "[t PULL ] -> ", PTPLib::Command.CLAUSEINJECTION, " is queued and notified");
            }
        }
        else
            break;
    }
}

bool Listener::read_lemma(std::vector<std::pair<std::string, int>>  & lemmas, PTPLib::net::Header & header) {

    assert(not (header.at(PTPLib::Param.NODE).empty() and header.at(PTPLib::Param.NAME).empty()));
    int rand_number = SMTSolver::generate_rand(100, 2000);
    std::this_thread::sleep_for(std::chrono::milliseconds (rand_number));
    if (rand_number % 3 == 0)
        return false;
    for (int i = 0; i < rand_number ; ++i) {
        std::string str = (i%2 == 0 ? "true" : "false");
        lemmas.emplace_back(std::make_pair("assert(" + str + ")", i % 3));
    }
    header.at(PTPLib::Param.COMMAND);
    return not lemmas.empty();
}

void Listener::write_lemma(std::unique_ptr<std::map<std::string, std::vector<std::pair<std::string, int>>>> const & m_clauses,
                           PTPLib::net::Header & header)
{
    assert(not (header.at(PTPLib::Param.NODE).empty() and header.at(PTPLib::Param.NAME).empty()));
    for (const auto &toPushClause : *m_clauses)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds (m_clauses->size()));
        stream.println(color_enabled ? PTPLib::Color::FG_Blue : PTPLib::Color::FG_DEFAULT,
                       "[t PUSH ] -> push learned clauses to Cloud Clause Size: ", toPushClause.second.size());
    }
    header.at(PTPLib::Param.COMMAND);
}