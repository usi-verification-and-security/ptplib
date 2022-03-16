# Partition Tree Protocol Library

Partition tree is a way of organising the solving of monotonic logics by
combining divide-and-conquer parallelism, algorithm portfolio and lemma
sharing.  The nodes of a partition tree are problem instances in the
underlying logic such that a node is satisfiable if and only if at
least one of its children is satisfiable.

The library offers primitives for implementing a partition-tree-based
parallelization approach.  The goals of the protocol are:
 - The idle time of a solver minimised:
    - If there are solvers running on an instance that is known to be
      unsatisfiable, the solvers are moved to instances whose
      satisfiability is unknown
    - If there are fewer unknown nodes than solvers, (some of the)
      solvers will work as a portfolio
 - The state of the solver is used maximally.  The idea is that when a
   solver needs to start solving a new instance, it should move to a
   "nearby" instance.  The lemmas that the solver has learned can be
   used maximally in solving the new instance.
 - The tree is constructed dynamically:
    - A node shown unsatisfiable is never expanded
    - A node is expanded only if there are redundant solvers
    - The tree is expanded in a way that aims at having equal lengths
      for all paths from the root to the children.
 - The task of constructing the children of a node is delegated to a
   solver in the node, if one exists.
 - Learned clauses are associated with a node in which it is known to be
   valid.  This allows their sharing between different nodes.

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
   

   
