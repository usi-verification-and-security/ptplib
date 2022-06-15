#include "Listener.cc"

#include <PTPLib/common/Printer.hpp>
#include <PTPLib/common/Lib.hpp>

#include <iostream>
#include <string>

int main(int argc, char** argv) {

    bool color_enabled = true;
    PTPLib::common::synced_stream stream_errr(std::cerr);
    if (argc < 3) {
        stream_errr.println(PTPLib::common::Color::FG_BrightBlue, "Usage: " , argv[0]
         , "<max number of instances> <max number of events> <waiting_duration=optional> <seed=optional>");
        return 1;
    }

    PTPLib::common::synced_stream stream(std::clog);
    PTPLib::common::StoppableWatch solving_watch;
    solving_watch.start();
    std::srand((argc < 4) ? static_cast<std::uint_fast8_t>(solving_watch.elapsed_time_microseconds()) : atoi(argv[4]));
    solving_watch.stop();

    int number_instances = atoi(argv[1]) ;
    stream.println(color_enabled ? PTPLib::common::Color::FG_Red : PTPLib::common::Color::FG_DEFAULT,
                   "<<---------------------->> Total Number of Instances: ", number_instances," <<------------------------>> ");

    double waiting_duration = (argc < 4) ? 0 : std::stod(argv[3]);
    Listener listener(stream, color_enabled, waiting_duration);

    int instanceNum = 0;
    while (number_instances != 0)
    {
        solving_watch.start();
        {
            listener.push_to_pool(PTPLib::common::TASK::MEMORYCHECK);
            listener.push_to_pool(PTPLib::common::TASK::COMMUNICATION);

            listener.push_to_pool(PTPLib::common::TASK::CLAUSEPUSH, (waiting_duration ? waiting_duration : std::rand()), 1000, 2000);
            listener.push_to_pool(PTPLib::common::TASK::CLAUSEPULL, (waiting_duration ? waiting_duration : std::rand()), 2000, 4000);
            listener.push_to_pool(PTPLib::common::TASK::CLAUSELEARN);
        }

        int nCommands = atoi(argv[2]);


        listener.set_eventGen_stat(++instanceNum, nCommands);

        stream.println( color_enabled ? PTPLib::common::Color::FG_Red : PTPLib::common::Color::FG_DEFAULT,
                        "<********************** Instance: ", instanceNum,
                        " ( Number of Commands: ", nCommands, ") **********************>");
        int command_counter = 1;
        bool reset = false;

        while (true)
        {
            auto event = listener.generate_event(command_counter, solving_watch.elapsed_time_second());
            assert(not event.header[PTPLib::common::Param.COMMAND].empty());

            stream.println(color_enabled ? PTPLib::common::Color::FG_Red : PTPLib::common::Color::FG_DEFAULT,
                           "[t LISTENER ] -> ", event.header.at(PTPLib::common::Param.COMMAND), " is received and notified" );
            {
                std::unique_lock<std::mutex> _lk(listener.getChannel().getMutex());
                assert([&]() {
                    if (not _lk.owns_lock()) {
                        throw PTPLib::common::Exception(__FILE__, __LINE__, "listener can't take the lock");
                    }
                    return true;
                }());
                reset = listener.queue_event(std::move(event));
            }
            if (reset)
                break;
            command_counter++;
        }
        listener.notify_reset();
        listener.getPool().wait_for_tasks();
        assert(not listener.getPool().get_tasks_total());
        {
            std::scoped_lock<std::mutex> _lk(listener.getChannel().getMutex());
            listener.getChannel().resetChannel();
        }
        solving_watch.reset();
        number_instances--;
    }
    return 0;
}


