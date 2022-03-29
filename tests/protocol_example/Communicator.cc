#include "Communicator.h"

#include <iostream>
#include <string>


void Communicator::solver_worker(const PTPLib::net::Header & header, std::string smt_lib) {
    channel.set_current_header(header);
    solver.search(smt_lib);
}

void Communicator::communicate_worker()
{
    while (true)
    {
        std::unique_lock<std::mutex> lk(channel.getMutex());
        getChannel().wait(lk);

        if (channel.shallStop())
        {
            channel.getMutex().unlock();
            th_pool.wait_for_tasks();

            {
                std::scoped_lock<std::mutex> slk(channel.getMutex());
                channel.clearShallStop();
            }
        }

        else if (not getChannel().isEmpty_query())
        {
            auto event = getChannel().pop_front_query();
            stream.println(color_enabled ? PTPLib::Color::FG_Cyan : PTPLib::Color::FG_DEFAULT,
                           "[t COMMUNICATOR ] -> ", "updating the channel with ", event.first.at(PTPLib::Param.COMMAND), " and waiting...");
            channel.getMutex().unlock();

            notify_and_wait(event );

            bool resume = true;
            {
                std::scoped_lock<std::mutex> slk(channel.getMutex());
                resume = execute_event(event);
                getChannel().clearShouldStop();
            }
            if (resume) {
                th_pool.push_task([this, event]
                {
                    solver_worker(event.first, event.second + event.first.at(PTPLib::Param.QUERY));
                });
            }
        }

        else if (channel.shouldReset())
            break;

        else {
            stream.println(color_enabled ? PTPLib::Color::FG_Cyan : PTPLib::Color::FG_DEFAULT,
                           "[t COMMUNICATOR ] -> ", "spurious wake up!");
            assert(false);
        }


    }
}


bool Communicator::execute_event(const std::pair<PTPLib::net::Header, std::string> & event)
{
    if (event.first.at(PTPLib::Param.COMMAND) == PTPLib::Command.STOP)
        return false;

    else if (event.first.at(PTPLib::Param.COMMAND) == PTPLib::Command.SOLVE)
        solver.initialise_logic();

    else if (event.first.at(PTPLib::Param.COMMAND) == PTPLib::Command.PARTITION)
        solver.do_partition(event.first.at(PTPLib::Param.NODE), event.first.at(PTPLib::Param.PARTITIONS));

    else if (event.first.at(PTPLib::Param.COMMAND) == PTPLib::Command.CLAUSEINJECTION) {
        auto pulled_clauses = channel.swap_pulled_clauses();
        solver.inject_clauses(*pulled_clauses);
    }
    return true;
}


void Communicator::notify_and_wait(std::pair<PTPLib::net::Header, std::string> & header_payload)
{
    if (header_payload.first.at(PTPLib::Param.COMMAND) != PTPLib::Command.SOLVE) {
            channel.setShouldStop();
            th_pool.wait_for_tasks();
    }
}
