#include "Communicator.h"

#include <PTPLib/common/Exception.hpp>

#include <iostream>
#include <string>
#include <cassert>
#include <climits>

void Communicator::communicate_worker()
{
    thread_id = std::this_thread::get_id();
    while (true)
    {
        std::unique_lock<std::mutex> lk(channel.getMutex());
        getChannel().wait_event_solver_reset(lk);
        assert([&]() {
            if (thread_id != std::this_thread::get_id())
                throw PTPLib::common::Exception(__FILE__, __LINE__, "communicate_worker has inconsistent thread id");

            if (not lk.owns_lock()) {
                throw PTPLib::common::Exception(__FILE__, __LINE__, "communicate_worker can't take the lock");
            }
            return true;
        }());

        if (channel.shallStop()) {
            channel.clearShallStop();
            lk.unlock();

            if (future.valid()) {
                solver.setResult(future.get());
                assert(solver.getResult() != SMTSolver::Result::UNKNOWN);
                // Report Solver Result
            }

        } else if (not getChannel().isEmpty_event()) {
            auto event = getChannel().pop_front_event();
            assert(not event.header[PTPLib::common::Param.COMMAND].empty());
            stream.println(color_enabled ? PTPLib::common::Color::FG_Cyan : PTPLib::common::Color::FG_DEFAULT,
                            "[t COMMUNICATOR ] -> ", "updating the channel with ",
                           event.header.at(PTPLib::common::Param.COMMAND), " and waiting");
            lk.unlock();

            if (setStop(event)) {
                if (future.valid())
                    future.wait();
            }

            bool should_resume;
            bool shouldUpdateSolverAddress = false;
            {
                std::scoped_lock<std::mutex> slk(channel.getMutex());
                should_resume = execute_event(event, shouldUpdateSolverAddress);
                if (shouldUpdateSolverAddress) {
                    channel.clear_current_header();
                    channel.set_current_header(event.header);
                }
            }

            if (should_resume) {
                getChannel().clearShouldStop();
                channel.clearShallStop();
                future = th_pool.submit([this, event] {
                    assert(not event.header.at(PTPLib::common::Param.QUERY).empty());
                    return solver.search((char *) (event.body + event.header.at(PTPLib::common::Param.QUERY)).c_str());
                }, ::get_task_name(PTPLib::common::TASK::SOLVER));
            } else
                break;
        }
        else if (channel.shouldReset())
            break;

        else {
            stream.println(color_enabled ? PTPLib::common::Color::FG_Cyan : PTPLib::common::Color::FG_DEFAULT,
                           "[t COMMUNICATOR ] -> ", "spurious wake up!");
            assert(false);
        }
    }
}


bool Communicator::execute_event(const PTPLib::net::SMTS_Event & event, bool & shouldUpdateSolverAddress)
{
    assert(not event.header.at(PTPLib::common::Param.COMMAND).empty());
    if (event.header.at(PTPLib::common::Param.COMMAND) == PTPLib::common::Command.STOP)
        return false;

    else if (event.header.at(PTPLib::common::Param.COMMAND) == PTPLib::common::Command.SOLVE) {
        solver.initialise_logic();
        shouldUpdateSolverAddress = true;
    }

    else if (event.header.at(PTPLib::common::Param.COMMAND) == PTPLib::common::Command.PARTITION)
        solver.do_partition(event.header.at(PTPLib::common::Param.NODE), event.header.at(PTPLib::common::Param.PARTITIONS));

    else if (event.header.at(PTPLib::common::Param.COMMAND) == PTPLib::common::Command.CLAUSEINJECTION) {
        auto pulled_clauses = channel.swap_pulled_clauses();
        solver.inject_clauses(*pulled_clauses);

    } else if (event.header.at(PTPLib::common::Param.COMMAND) == PTPLib::common::Command.INCREMENTAL)
        shouldUpdateSolverAddress = true;

    if (not channel.isEmpty_event() and channel.front_event() == PTPLib::common::Command.STOP)
        return false;

    return true;
}


bool Communicator::setStop(PTPLib::net::SMTS_Event & event)
{
    assert(not event.header.at(PTPLib::common::Param.COMMAND).empty());
    if (event.header.at(PTPLib::common::Param.COMMAND) != PTPLib::common::Command.SOLVE) {
        channel.setShouldStop();
        return true;
    }
    else
        return false;
}
