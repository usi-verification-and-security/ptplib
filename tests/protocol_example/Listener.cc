#include "Listener.h"
#include "SMTSolver.h"

#include <PTPLib/common/Memory.hpp>
#include "PTPLib/net/Lemma.hpp"
#include <PTPLib/common/Exception.hpp>

#include <iostream>
#include <string>
#include <cmath>
#include <cassert>


void Listener::memory_checker()
{
    memory_thread_id = std::this_thread::get_id();
    size_t limit = 4000;//atoll(max_memory.c_str());
    if (limit == 0)
        return;

    while (true) {
        size_t memory_size_b = PTPLib::common::current_memory();
        if (memory_size_b > limit * 1024 * 1024) {
            stream.println(color_enabled ? PTPLib::common::Color::FG_Yellow : PTPLib::common::Color::FG_DEFAULT, "[t max memory checker ] -> max memory reached: ", std::to_string(memory_size_b));
                exit(-1);
        }
        stream.println(color_enabled ? PTPLib::common::Color::FG_Yellow : PTPLib::common::Color::FG_DEFAULT, "[t max memory checker ] -> ", std::to_string(memory_size_b));
        std::unique_lock<std::mutex> lk(channel.getMutex());
        if (channel.wait_for_reset(lk, std::chrono::seconds (10)))
            break;
        assert([&]() {
            if (memory_thread_id != std::this_thread::get_id())
                throw PTPLib::common::Exception(__FILE__, __LINE__, "memory_checker has inconsistent thread id");

            if (not lk.owns_lock()) {
                throw PTPLib::common::Exception(__FILE__, __LINE__, "memory_checker can't take the lock");
            }
            return true;
        }());
    }
}

void Listener::worker(PTPLib::common::TASK wname, double seed, double td_min, double td_max) {

    switch (wname) {

        case PTPLib::common::TASK::COMMUNICATION:
            communicator.communicate_worker();
            break;

        case PTPLib::common::TASK::CLAUSEPUSH:
            push_clause_worker(seed, td_min, td_max);
            break;

        case PTPLib::common::TASK::CLAUSEPULL:
            pull_clause_worker(seed, td_min, td_max);
            break;

        case PTPLib::common::TASK::MEMORYCHECK:
            memory_checker();
            break;
        case PTPLib::common::TASK::CLAUSELEARN:
            periodic_clauseLearning_worker();
            break;
        default:
            break;
    }

}

void Listener::push_to_pool(PTPLib::common::TASK t_name, double seed, double td_min, double td_max )
{
    th_pool.push_task([this, t_name, seed, td_min, td_max]
    {
            worker(t_name, seed, td_min, td_max);
    }, ::get_task_name(t_name));
}

void Listener::notify_reset()
{
    std::unique_lock<std::mutex> _lk(getChannel().getMutex());
    assert([&]() {
        if (not _lk.owns_lock()) {
            throw PTPLib::common::Exception(__FILE__, __LINE__, "listener can't take the lock");
        }
        return true;
    }());
    channel.setReset();
    channel.notify_all();
    _lk.unlock();
}

template<class T>
bool Listener::queue_event(T && event)
{
    bool reset = false;
    if (event.header[PTPLib::common::Param.COMMAND] == PTPLib::common::Command.STOP) {
        reset = true;
        getChannel().push_front_event(std::forward<T>(event));
    }
    else
        getChannel().push_back_event(std::forward<T>(event));

    getChannel().notify_all();
    return reset;
}

PTPLib::net::SMTS_Event Listener::generate_event(int counter, int time_passed)
{
    std::string payload;
    int sleep_duration = 0;
    if (counter > 1) {
        if (waiting_duration)
            sleep_duration = waiting_duration*(1000);
        else
            sleep_duration = SMTSolver::generate_rand(0, 5000);
    }

    std::this_thread::sleep_for(std::chrono::milliseconds (sleep_duration));

    PTPLib::net::Header header;
    header.emplace(PTPLib::common::Param.NAME, "instance"+ std::to_string(instanceNum) +".smt2");
    header.emplace(PTPLib::common::Param.NODE, "[" + std::to_string(counter) + "]");
    header.emplace(PTPLib::common::Param.QUERY, "(check-sat)");
    if (time_passed > nCommands * 5) {
        header.emplace(PTPLib::common::Param.COMMAND, PTPLib::common::Command.STOP);
        return PTPLib::net::SMTS_Event(std::move(header), std::move(payload));
    }
    else if (counter == nCommands) {
        if (not waiting_duration)
            std::this_thread::sleep_for(std::chrono::seconds ((nCommands * 10) - time_passed));
        header.emplace(PTPLib::common::Param.COMMAND, PTPLib::common::Command.STOP);
        return PTPLib::net::SMTS_Event(std::move(header), std::move(payload));
    }
    else if (counter == 1) {
        header.emplace(PTPLib::common::Param.COMMAND, PTPLib::common::Command.SOLVE);
        payload = "solve( " + header.at(PTPLib::common::Param.NAME) + " )";
    }

    else if (counter % 2 == 0) {
        header.emplace(PTPLib::common::Param.COMMAND, PTPLib::common::Command.INCREMENTAL);
        header.emplace(PTPLib::common::Param.NODE_, "[" + std::to_string(counter + 1) + "]");
        payload = "move ( " + header.at(PTPLib::common::Param.NAME) + " )";
    }

    else {
        header.emplace(PTPLib::common::Param.COMMAND, PTPLib::common::Command.PARTITION);
        header.emplace(PTPLib::common::Param.PARTITIONS, "2");
    }

    return PTPLib::net::SMTS_Event(std::move(header), std::move(payload));
}

void Listener::push_clause_worker(double seed, double min, double max)
{
    push_thread_id = std::this_thread::get_id();
    int push_duration = (waiting_duration ? waiting_duration*(100) :  min + ( std::fmod(seed, ( max - min + 1 ))));
    stream.println(color_enabled ? PTPLib::common::Color::FG_Blue : PTPLib::common::Color::FG_DEFAULT,
                   "[t PUSH -> timout : ", push_duration," ms");
    PTPLib::net::time_duration wakeupAt = std::chrono::milliseconds (push_duration);
    while (true) {
        PTPLib::common::PrintStopWatch psw("[t PUSH ] -> measured wait and write duration: ", stream,
                                   color_enabled ? PTPLib::common::Color::FG_Blue : PTPLib::common::Color::FG_DEFAULT);
        std::unique_lock<std::mutex> lk(getChannel().getMutex());
        bool reset = getChannel().wait_for_reset(lk, wakeupAt);
        assert([&]() {
            if (push_thread_id != std::this_thread::get_id())
                throw PTPLib::common::Exception(__FILE__, __LINE__, "push_clause_worker has inconsistent thread id");

            if (not lk.owns_lock()) {
                throw PTPLib::common::Exception(__FILE__, __LINE__, "push_clause_worker can't take the lock");
            }
            return true;
        }());
        if (not reset)
        {
            if (not getChannel().empty_learned_clauses())
            {
                auto m_clauses = getChannel().swap_learned_clauses();
                getChannel().clear_learned_clauses();
                auto header = getChannel().get_current_header();
                lk.unlock();
                assert([&]() {
                    if (lk.owns_lock()) {
                        throw PTPLib::common::Exception(__FILE__, __LINE__, "push_clause_worker should not hold the lock");
                    }
                    return true;
                }());
                if (not header.empty()) {
                    write_lemma(m_clauses, header);
                    m_clauses->clear();
                }
            }
            else stream.println(color_enabled ? PTPLib::common::Color::FG_Blue : PTPLib::common::Color::FG_DEFAULT,
                                "[t PUSH ] -> Channel empty!");
        }
        else if (getChannel().shouldReset())
            break;

        else {
            stream.println(color_enabled ? PTPLib::common::Color::FG_Blue : PTPLib::common::Color::FG_DEFAULT,
                           "[t PUSH ] -> ", "spurious wake up!");
            assert(false);
        }
    }
}

void Listener::pull_clause_worker(double seed, double min, double max)
{
    pull_thread_id = std::this_thread::get_id();
    int pull_duration = (waiting_duration ? waiting_duration*(200) : min + ( std::fmod(seed, ( max - min + 1 ))));
    stream.println(color_enabled ? PTPLib::common::Color::FG_Magenta : PTPLib::common::Color::FG_DEFAULT,
                   "[t PULL -> timout : ", pull_duration," ms");
    PTPLib::net::time_duration wakeupAt = std::chrono::milliseconds (pull_duration);
    while (true) {

        PTPLib::common::PrintStopWatch psw("[t PULL ] -> measured wait and read duration: ", stream,
                                   color_enabled ? PTPLib::common::Color::FG_Magenta : PTPLib::common::Color::FG_DEFAULT);
        std::unique_lock<std::mutex> lk(getChannel().getMutex());
        bool reset = getChannel().wait_for_reset(lk, wakeupAt);
        assert([&]() {
            if (pull_thread_id != std::this_thread::get_id())
                throw PTPLib::common::Exception(__FILE__, __LINE__, "pull_clause_worker has inconsistent thread id");

            if (not lk.owns_lock()) {
                throw PTPLib::common::Exception(__FILE__, __LINE__, "pull_clause_worker can't take the lock");
            }
            return true;
        }());
        if (not reset)
        {
            auto header = channel.get_current_header();
            lk.unlock();
            assert([&]() {
                if (lk.owns_lock()) {
                    throw PTPLib::common::Exception(__FILE__, __LINE__, "pull_clause_worker should not hold the lock");
                }

                return true;
            }());
            if (not header.empty()) {
                std::vector<PTPLib::net::Lemma> lemmas;
                if (this->read_lemma(lemmas, header)) {
                    stream.println(color_enabled ? PTPLib::common::Color::FG_Magenta : PTPLib::common::Color::FG_DEFAULT,
                                   "[t PULL ] -> pulled learned clauses copied to channel buffer, Size: ",
                                   lemmas.size());
                    {
                        std::unique_lock<std::mutex> _lk(getChannel().getMutex());
                        assert([&]() {
                            if (not _lk.owns_lock()) {
                                throw PTPLib::common::Exception(__FILE__, __LINE__, "pull_clause_worker can't take the lock");
                            }
                            return true;
                        }());

                        if (getChannel().shouldReset())
                            break;
                        channel.insert_pulled_clause(std::move(lemmas));
                        header.erase(PTPLib::common::Param.COMMAND);
                        header[PTPLib::common::Param.COMMAND] = PTPLib::common::Command.CLAUSEINJECTION;
                        queue_event(PTPLib::net::SMTS_Event(std::move(header), ""));
                        _lk.unlock();
                    }
                    stream.println(color_enabled ? PTPLib::common::Color::FG_Magenta : PTPLib::common::Color::FG_DEFAULT,
                                   "[t PULL ] -> ", PTPLib::common::Command.CLAUSEINJECTION, " is queued and notified");
                }
            }
        } else if (getChannel().shouldReset())
            break;

        else {
                stream.println(color_enabled ? PTPLib::common::Color::FG_Magenta : PTPLib::common::Color::FG_DEFAULT,
                               "[t PULL ] -> ", "spurious wake up!");
                assert(false);
            }
    }
}

bool Listener::read_lemma(std::vector<PTPLib::net::Lemma>  & lemmas, PTPLib::net::Header & header) {

    assert((not header.at(PTPLib::common::Param.NODE).empty()) and (not header.at(PTPLib::common::Param.NAME).empty()));

    int rand_number = waiting_duration ? waiting_duration*(200) : SMTSolver::generate_rand(100, 2000);
    std::this_thread::sleep_for(std::chrono::milliseconds (rand_number));
    if (rand_number % 3 == 0)
        return false;
    for (int i = 0; i < rand_number ; ++i) {
        std::string str = (i%2 == 0 ? "true" : "false");
        lemmas.emplace_back(PTPLib::net::Lemma("assert(" + str + ")", i % 3));
    }
    header.at(PTPLib::common::Param.NODE);
    return not lemmas.empty();
}

bool Listener::write_lemma(std::unique_ptr<std::map<std::string, std::vector<PTPLib::net::Lemma>>> const & m_clauses,
                           PTPLib::net::Header & header)
{
    assert((not header.at(PTPLib::common::Param.NODE).empty()) and (not header.at(PTPLib::common::Param.NAME).empty()));
    for (const auto &node_clauses : *m_clauses)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds (int(waiting_duration ? (waiting_duration*100) :  m_clauses->size())));
        stream.println(color_enabled ? PTPLib::common::Color::FG_Blue : PTPLib::common::Color::FG_DEFAULT,
                       "[t PUSH ] -> push learned clauses to Cloud Clause Size: ", node_clauses.second.size());
    }
    header.at(PTPLib::common::Param.NODE);
    return true;
}

void Listener::periodic_clauseLearning_worker()
{
    int random_n = waiting_duration ? waiting_duration * (100) : SMTSolver::generate_rand(1000, 2000);
    if (getChannel().isClauseShareMode()) {
        th_pool.push_task
        (
            [this, random_n]
            {
                while (true)
                {
                    std::unique_lock<std::mutex> lk(channel.getMutex());
                    if (getChannel().wait_for_reset(lk, std::chrono::milliseconds(static_cast<int>(random_n*2))))
                        break;
                    assert([&]() {
                        if (not lk.owns_lock()) {
                            throw PTPLib::common::Exception(__FILE__, __LINE__, "periodic_clauseLearning_worker can't take the lock");
                        }
                        return true;
                    }());
                    getChannel().setShouldLearnClauses();
                    lk.unlock();
                }
                stream.println(color_enabled ? PTPLib::common::Color::FG_BrightBlue : PTPLib::common::Color::FG_DEFAULT,
                           "[t CLAUSELEARN ] -> clause learn timout: ", random_n);
            },
            ::get_task_name(PTPLib::common::TASK::CLAUSELEARN)
       );
    }

}