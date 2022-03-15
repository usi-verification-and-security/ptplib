# Partition Tree Protocol Library
### PTPLib contains:
#### 1. A communication channel
 
   A memory model for thread-architecture that supports:
   opensmt signaling to channel it has new clauses to share, so push thread will push it to CC
   pull thread signaling to channel that it has pulled clauses
   SMTS thread signaling partition , stop, incremental queries to the channel.
#### 2. An async printer
   To output coloured text which can be used in multi-threading context. 

A shared cross-thread object of PTPLib::synced_stream, can be used as follow:
   ```
   PTPLib::synced_stream synced_stream
   synced_stream(std::clog);
   synced_stream.println(Color::FG_BrightCyan, "status:", status, ...);
   ```
to have a thread-safe coloured stream.

#### 3. A serialization and deserialization interface

#### 4. A manual time checker 
Which can start, stop, accumulate and reset the time.
   ```
   PTPLib::StoppableWatch timer;
   timer.start();
   timer.elapsed_time_milliseconds()
   ```

####  5. An async time checker/printer 
Which is triggered on contructor/destructor, automatically measures the time.
   ```
   PTPLib::PrintStopWatch timer("Stop Watch", synced_stream, Color::Code:: FG_BrightCyan );
   ```
   

   
