#include <elevator/concurrentqueue.h>
#include <elevator/state.h>
#include <elevator/command.h>
#include <elevator/heartbeat.h>
#include <thread>
#include <atomic>

#ifndef ELEVATOR_SCHEDULER_H
#define ELEVATOR_SCHEDULER_H

namespace elevator {

struct Scheduler {
    Scheduler( int, HeartBeat &, ConcurrentQueue< StateChange > &,
            ConcurrentQueue< Command > &, ConcurrentQueue< Command > & );
    ~Scheduler();

    void run();

  private:
    int _localElevId;
    HeartBeat &_heartbeat;
    ConcurrentQueue< StateChange > &_stateUpdateQueue;
    ConcurrentQueue< Command > &_localCommands;
    ConcurrentQueue< Command > &_remoteCommands;
    GlobalState _globalState;
    std::thread _thr;
    std::atomic< bool > _terminate;

    void _runLocal();
};

}

#endif // ELEVATOR_SCHEDULER_H
