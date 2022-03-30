#include "Listener.cc"

#include <iostream>
#include <string>
#include <PTPLib/PartitionConstant.hpp>
#include <PTPLib/lib.hpp>

int main(int argc, char** argv) {

    bool color_enabled = true;
    PTPLib::synced_stream stream_errr(std::cerr);
    if (argc < 3) {
        stream_errr.println(PTPLib::Color::FG_BrightBlue, "Usage: " , argv[0]
         , "<max number of instances> <max number of events> <waiting_duration=optional> <seed=optional>");
        return 1;
    }

    PTPLib::synced_stream stream (std::clog);
    PTPLib::StoppableWatch solving_watch;
    solving_watch.start();
    std::srand((argv[3] == NULL) ? static_cast<std::uint_fast8_t>(solving_watch.elapsed_time_microseconds()) : atoi(argv[3]));
    solving_watch.stop();

    int number_instances = atoi(argv[1]) ;//SMTSolver::generate_rand(1, atoi(argv[1]));
    stream.println(color_enabled ? PTPLib::Color::FG_Red : PTPLib::Color::FG_DEFAULT,
                   "<<---------------------->> Total Number of Instances: ", number_instances," <<------------------------>> ");

    double waiting_duration = (argv[3] == NULL) ? 0 : std::stod(argv[3]);
    Listener listener(stream, color_enabled, waiting_duration);
    int instanceNum = 0;
    while (number_instances != 0)
    {
        solving_watch.start();
        {
            listener.push_to_pool(PTPLib::WORKER::MEMORYCHECK);
            listener.push_to_pool(PTPLib::WORKER::COMMUNICATION);

            listener.push_to_pool(PTPLib::WORKER::CLAUSEPUSH, (waiting_duration ? waiting_duration : std::rand()), 1000, 2000);
            listener.push_to_pool(PTPLib::WORKER::CLAUSEPULL, (waiting_duration ? waiting_duration : std::rand()), 2000, 4000);
        }

        int nCommands = atoi(argv[2]);//SMTSolver::generate_rand(2,  atoi(argv[2]));


        listener.set_eventGen_stat(++instanceNum, nCommands);

        stream.println( color_enabled ? PTPLib::Color::FG_Red : PTPLib::Color::FG_DEFAULT,
                        "<********************** Instance: ", instanceNum,
                        " ( Number of Commands: ", nCommands, ") **********************>");
        int command_counter = 1;
        bool reset = false;

        while (true)
        {
            auto header_payload = listener.read_event(command_counter, solving_watch.elapsed_time_second());
            assert(not header_payload.first[PTPLib::Param.COMMAND].empty());

            stream.println(color_enabled ? PTPLib::Color::FG_Red : PTPLib::Color::FG_DEFAULT,
                           "[t LISTENER ] -> ", header_payload.first.at(PTPLib::Param.COMMAND)," is received and notified" );

            {
                std::scoped_lock<std::mutex> _lk(listener.getChannel().getMutex());
                if (header_payload.first[PTPLib::Param.COMMAND] == PTPLib::Command.STOP) {
                    reset = true;
                    listener.getChannel().clear_queries();
                }
                listener.queue_event(std::move(header_payload));
            }
            if (reset)
                break;
            command_counter++;
        }
        listener.notify_reset();
        listener.getPool().wait_for_tasks();
        {
            std::scoped_lock<std::mutex> _lk(listener.getChannel().getMutex());
            listener.getChannel().resetChannel();
        }
        solving_watch.reset();
        number_instances--;
    }
    return 0;
}


