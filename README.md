# Partition Channel Library
### partition channel library contains:
1. A communication channel (a memory model for thread-architecture) that supports:
   opensmt signaling to channel it has new clauses to share, so push thread will push it to CC
   pull thread signaling to channel that it has pulled clauses
   SMTS thread signaling partition , stop, incremental queries to channel
1. An async printer to output coloured text which can be used in multi-threading context.
use case:
A shared cross-thread object of opensmt::synced_stream, synced_stream, can be used as follow:
synced_stream(std::clog);
synced_stream.println(Color::FG_BrightCyan, "status:", status, ...);
to have a thread-safe coloured stream.

2. A serialization and deserialization interface
   
2. A manual time checker which can start, stop, accumulate and reset the time
use case:
opensmt::StoppableWatch timer;
timer.start();
timer.elapsed_time_milliseconds()

3. An async time checker/printer which triggers on contructor/destructor, automatically measures the time
use case:
opensmt::PrintStopWatch timer("wait time ", synced_stream, Color::Code:: FG_BrightCyan );
   

   
