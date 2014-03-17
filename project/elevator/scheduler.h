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
    Scheduler( int, HeartBeat &, BasicDriverInfo info,
            ConcurrentQueue< StateChange > &,
            ConcurrentQueue< StateChange > &,
            ConcurrentQueue< Command > &,
            ConcurrentQueue< Command > & );
    ~Scheduler();

    void run();

  private:
    int _localElevId;
    HeartBeat &_heartbeat;
    BasicDriverInfo _bounds;
    ConcurrentQueue< StateChange > &_stateUpdateIn;
    ConcurrentQueue< StateChange > &_stateUpdateOut;
    ConcurrentQueue< Command > &_commandsToRemote;
    ConcurrentQueue< Command > &_commandsToLocal;
    GlobalState _globalState;
    std::thread _thr;
    std::atomic< bool > _terminate;

    void _runLocal();

    void _handleButtonPress( ButtonType, int );
    void _forwardToTargets( Command );
};

}

#endif // ELEVATOR_SCHEDULER_H
