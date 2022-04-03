#include "Communicator.h"

#include <iostream>
#include <string>


void Communicator::communicate_worker()
{
    while (true)
    {
        std::unique_lock<std::mutex> lk(channel.getMutex());
        getChannel().wait(lk);

        if (channel.shallStop())
        {
            channel.getMutex().unlock();

            if (future.valid()) {
                future.wait();
                solver.setResult(future.get());
                assert(solver.getResult()!= SMTSolver::Result::UNKNOWN);
            }

            {
                std::scoped_lock<std::mutex> slk(channel.getMutex());
                channel.clearShallStop();
            }
        }

        else if (not getChannel().isEmpty_query())
        {
            auto event = getChannel().pop_front_query();
            stream.println(color_enabled ? PTPLib::Color::FG_Cyan : PTPLib::Color::FG_DEFAULT,
               "[t COMMUNICATOR ] -> ", "updating the channel with ", event.first.at(PTPLib::Param.COMMAND), " and waiting");
            channel.getMutex().unlock();

            if (setStop(event)) {
                if (future.valid())
                {
                    future.wait();
                    future.get();
                }
            }

            bool should_resume;
            {
                std::scoped_lock<std::mutex> slk(channel.getMutex());
                should_resume = execute_event(event);
                if (should_resume) {
                    channel.clear_current_header();
                    channel.set_current_header(event.first);
                }
            }

            if (should_resume) {
                getChannel().clearShouldStop();

                future = th_pool.submit([this, event]
                {
                    return solver.search((char *)(event.second + event.first.at(PTPLib::Param.QUERY)).c_str());
                });

            } else
                break;
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
    if (not channel.isEmpty_query() and channel.front_queries().at(PTPLib::Param.COMMAND) == PTPLib::Command.STOP)
        return false;
    return true;
}


bool Communicator::setStop(std::pair<PTPLib::net::Header, std::string> & header_payload)
{
    if (header_payload.first.at(PTPLib::Param.COMMAND) != PTPLib::Command.SOLVE) {
        channel.setShouldStop();
        return true;
    }
    else
        return false;
}
