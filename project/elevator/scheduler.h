#include <elevator/concurrentqueue.h>
#include <elevator/globalstate.h>
#include <elevator/command.h>
#include <elevator/heartbeat.h>
#include <thread>
#include <atomic>

#ifndef ELEVATOR_SCHEDULER_H
#define ELEVATOR_SCHEDULER_H

namespace elevator {

struct Scheduler {
    Scheduler( int, BasicDriverInfo info, GlobalState &,
            ConcurrentQueue< StateChange > &,
            ConcurrentQueue< StateChange > &,
            ConcurrentQueue< Command > &,
            ConcurrentQueue< Command > & );
    ~Scheduler();

    void run( HeartBeat &, HeartBeat & );

  private:
    int _localElevId;
    BasicDriverInfo _bounds;
    GlobalState &_globalState;
    ConcurrentQueue< StateChange > &_stateUpdateIn;
    ConcurrentQueue< StateChange > &_stateUpdateOut;
    ConcurrentQueue< Command > &_commandsToRemote;
    ConcurrentQueue< Command > &_commandsToLocal;
    std::thread _thrSched;
    std::thread _thrReq;
    std::atomic< bool > _terminate;

    void _schedLoop( HeartBeat * );
    void _reqCheckLoop( HeartBeat * );

    void _handleButtonPress( int, ButtonType, int );
    void _resendRequest( Request );
    void _addAndForwardRequest( Command );
    void _forwardToTargets( Command );
    int _optimalElevator( ButtonType, int );
};

}

#endif // ELEVATOR_SCHEDULER_H
