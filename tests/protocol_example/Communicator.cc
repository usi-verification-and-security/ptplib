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
            lk.unlock();

            if (future.valid()) {
                solver.setResult(future.get());
                // Report Solver Result
                assert(solver.getResult() != SMTSolver::Result::UNKNOWN);
            }

            {
                std::scoped_lock<std::mutex> slk(channel.getMutex());
                channel.clearShallStop();
            }
        } else if (not getChannel().isEmpty_query()) {
            auto event = getChannel().pop_front_query();
            assert(not event.first[PTPLib::common::Param.COMMAND].empty());
            stream.println(color_enabled ? PTPLib::common::Color::FG_Cyan : PTPLib::common::Color::FG_DEFAULT,
                            "[t COMMUNICATOR ] -> ", "updating the channel with ",
                           event.first.at(PTPLib::common::Param.COMMAND), " and waiting");
            lk.unlock();

            if (setStop(event)) {
                if (future.valid()) {
                    future.get();
                }
            }

            bool should_resume;
            bool shouldUpdateSolverAddress = false;
            {
                std::scoped_lock<std::mutex> slk(channel.getMutex());
                should_resume = execute_event(event, shouldUpdateSolverAddress);
                if (shouldUpdateSolverAddress) {
                    channel.clear_current_header();
                    channel.set_current_header(event.first);
                }
            }

            if (should_resume) {
                getChannel().clearShouldStop();
                future = th_pool.submit([this, event] {
                    assert(not event.first.at(PTPLib::common::Param.QUERY).empty());
                    return solver.search((char *) (event.second + event.first.at(PTPLib::common::Param.QUERY)).c_str());
                }, ::get_task_name(PTPLib::common::TASK::SOLVER));
            }
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


bool Communicator::execute_event(const PTPLib::net::smts_event & event, bool & shouldUpdateSolverAddress)
{
    assert(not event.first.at(PTPLib::common::Param.COMMAND).empty());
    if (event.first.at(PTPLib::common::Param.COMMAND) == PTPLib::common::Command.STOP)
        return false;

    else if (event.first.at(PTPLib::common::Param.COMMAND) == PTPLib::common::Command.SOLVE) {
        solver.initialise_logic();
        shouldUpdateSolverAddress = true;
    }

    else if (event.first.at(PTPLib::common::Param.COMMAND) == PTPLib::common::Command.PARTITION)
        solver.do_partition(event.first.at(PTPLib::common::Param.NODE), event.first.at(PTPLib::common::Param.PARTITIONS));

    else if (event.first.at(PTPLib::common::Param.COMMAND) == PTPLib::common::Command.CLAUSEINJECTION) {
        auto pulled_clauses = channel.swap_pulled_clauses();
        solver.inject_clauses(*pulled_clauses);

    } else if (event.first.at(PTPLib::common::Param.COMMAND) == PTPLib::common::Command.INCREMENTAL)
        shouldUpdateSolverAddress = true;

    if (not channel.isEmpty_query() and channel.front_queries().at(PTPLib::common::Param.COMMAND) == PTPLib::common::Command.STOP)
        return false;

    return true;
}


bool Communicator::setStop(PTPLib::net::smts_event & event)
{
    assert(not event.first.at(PTPLib::common::Param.COMMAND).empty());
    if (event.first.at(PTPLib::common::Param.COMMAND) != PTPLib::common::Command.SOLVE) {
        channel.setShouldStop();
        return true;
    }
    else
        return false;
}
