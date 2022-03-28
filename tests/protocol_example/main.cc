#include "Listener.cc"

#include <iostream>
#include <string>
#include <PTPLib/PartitionConstant.hpp>
#include <PTPLib/lib.hpp>

int main(int argc, char** argv) {

    bool color_enabled;
    if (argc < 3) {
        std::cout << "Usage: " << argv[0] << " <max number of instances> <max number of events> <color enabled=default>" << std::endl;
        return 1;
    } else color_enabled = argv[3] == NULL ? true : ::to_bool(argv[3]);

    PTPLib::synced_stream stream (std::clog);
    PTPLib::StoppableWatch solving_watch;


    int number_instances = SMTSolver::generate_rand(1, atoi(argv[1]));
    stream.println(color_enabled ? PTPLib::Color::FG_Red : PTPLib::Color::FG_DEFAULT,
                   "-------------------------------- Total Number of Instances: ", number_instances," ---------------------------------- ");

    Listener listener(stream, color_enabled);
    int instanceNum = 0;
    while (number_instances != 0)
    {
        solving_watch.start();
        {
            listener.push_to_pool(PTPLib::WORKER::MEMORYCHECK);
            listener.push_to_pool(PTPLib::WORKER::COMMUNICATION);

            std::srand(static_cast<std::uint_fast8_t>(solving_watch.elapsed_time_microseconds()));

            listener.push_to_pool(PTPLib::WORKER::CLAUSEPUSH, std::rand(), 1000, 5000);
            listener.push_to_pool(PTPLib::WORKER::CLAUSEPULL, std::rand(), 1000, 10000);
        }

        int nCommands = SMTSolver::generate_rand(2,  atoi(argv[2]));
        listener.set_numberOf_Command(++instanceNum, nCommands);

        stream.println( color_enabled ? PTPLib::Color::FG_Red : PTPLib::Color::FG_DEFAULT,
                        "************************* Instance: ", instanceNum," ( Number of Commands: ", nCommands, ") **********************");
        int command_counter = 1;
        bool reset = false;

        while (true)
        {
            auto header_payload = listener.read_event(command_counter, solving_watch.elapsed_time_second());
            assert(not header_payload.first[PTPLib::Param.COMMAND].empty());

            stream.println(color_enabled ? PTPLib::Color::FG_Red : PTPLib::Color::FG_DEFAULT,
                           "[t LISTENER ] -> ", header_payload.first.at(PTPLib::Param.COMMAND)," is received and notified" );

            std::unique_lock<std::mutex> _lk(listener.getChannel().getMutex());
            if (header_payload.first[PTPLib::Param.COMMAND] == PTPLib::Command.STOP)
            {
                reset = true;
                listener.getChannel().clear_queries();
            }
            listener.queue_event(std::move(header_payload));
            _lk.unlock();
            if (reset)
                break;
            command_counter++;
        }
        listener.notify_reset();
        listener.listener_pool.wait_for_tasks();
        listener.getChannel().resetChannel();
        solving_watch.reset();
        number_instances--;
    }
    return 0;
}


