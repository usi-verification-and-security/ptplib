#include "Communicator.h"

#include <iostream>
#include <string>


void Communicator::solver_worker(const PTPLib::net::Header & header, const std::string & smt_lib) {
    channel.set_current_header(header);
    solver.search(smt_lib);
}

void Communicator::communicate_worker()
{
    while (true)
    {
        std::unique_lock<std::mutex> lk(channel.getMutex());
        getChannel().wait(lk);


        if (channel.shouldReset())
            break;

        else if (channel.shallStop())
        {
            th_pool.wait_for_tasks();
            channel.clearShallStop();
        }

        else if (not getChannel().isEmpty_query())
        {
            auto event = getChannel().pop_front_query();
            stream.println(color_enabled ? PTPLib::Color::FG_Cyan : PTPLib::Color::FG_DEFAULT,
                           "[t COMMUNICATOR ] -> ", "updating the channel with ", event.first.at(PTPLib::Param.COMMAND));
            channel.getMutex().unlock();
            notify_event(event );


            channel.getMutex().lock();
            PTPLib::Task task = execute_event(event);
            getChannel().clearShouldStop();
            channel.getMutex().unlock();
            th_pool.push_task([this, event, task]
            {
                solver_worker(event.first, task.SMTLIB + event.first.at(PTPLib::Param.QUERY));
            });
        }
        else
            stream.println(color_enabled ? PTPLib::Color::FG_Cyan : PTPLib::Color::FG_DEFAULT,
                           "[t COMMUNICATOR ] -> ", "spurious wake up!");
    }
}


PTPLib::Task Communicator::execute_event(const std::pair<PTPLib::net::Header, std::string> & event)
{
    if (event.first.at(PTPLib::Param.COMMAND) == PTPLib::Command.SOLVE) {
        solver.initialise_logic();
        return PTPLib::Task{
                .command = PTPLib::Task::SOLVE,
                .SMTLIB = event.second
        };
    }
    if (event.first.at(PTPLib::Param.COMMAND) == PTPLib::Command.INCREMENTAL)
        return PTPLib::Task {
                .command = PTPLib::Task::RESUME,
                .SMTLIB = event.second
        };

    else if (event.first.at(PTPLib::Param.COMMAND) == PTPLib::Command.PARTITION)
        solver.do_partition(event.first.at(PTPLib::Param.NODE), event.first.at(PTPLib::Param.PARTITIONS));

    else if (event.first.at(PTPLib::Param.COMMAND) == PTPLib::Command.CLAUSEINJECTION)
        solver.inject_clauses();

    else if (event.first.at(PTPLib::Param.COMMAND) == PTPLib::Command.STOP)
        return PTPLib::Task {
                .command = PTPLib::Task::STOP,
                .SMTLIB = event.second
        };

    return PTPLib::Task {
            .command = PTPLib::Task::RESUME,
            .SMTLIB = event.second
    };
}


void Communicator::notify_event(std::pair<PTPLib::net::Header, std::string> & header_payload)
{
    if (header_payload.first.at(PTPLib::Param.COMMAND) != PTPLib::Command.SOLVE) {
            channel.setShouldStop();
            th_pool.wait_for_tasks();
    }
}
