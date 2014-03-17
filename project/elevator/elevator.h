// C++11    (c) 2014 Vladimír Štill <xstill@fi.muni.cz>

/* High level elevator API/event loop */

#include <elevator/driver.h>
#include <elevator/spinlock.h>
#include <elevator/heartbeat.h>
#include <elevator/concurrentqueue.h>
#include <elevator/command.h>
#include <elevator/state.h>
#include <elevator/floorset.h>

#include <atomic>
#include <functional>
#include <thread>
#include <vector>
#include <type_traits>

#ifndef SRC_ELEVATOR_H
#define SRC_ELEVATOR_H

namespace elevator {

struct Elevator {
    Elevator( int, HeartBeat &, ConcurrentQueue< Command > &, ConcurrentQueue< StateChange > & );
    ~Elevator();

    /* spawn control loop thread and run elevator (non blocking) */
    void run();
    void terminate();

    void addTargetFloor( int floor );
    void removeTargetFloor( int floor );
    void setMoveDirection( Direction );

    void assertConsistency();

    BasicDriverInfo info() const {
        return BasicDriverInfo( _driver );
    }

    /* directly execute command on lower-level driver API
     * - command is executed in calling thread (that is concurrently with control loop)
     * - locking is used to ensure safety
     * - the control loop is interruptible by dirrectCommand only at start of loop
     * - do not run blocking or long-lasting commnads in dirrectCommand, it can cause
     *   missing button signals or even heart-beat signal */
    template< typename Function >
    auto dirrectCommand( Function fn ) -> decltype( fn( std::declval< Driver& >() ) )
    {
        SpinLock guard( _lock );
        return fn( _driver );
    }

  private:
    void _loop();

    std::atomic< bool > _terminate;
    std::atomic_flag _lock;
    ConcurrentQueue< Command > &_inCommands;
    ConcurrentQueue< StateChange > &_outState;
    Driver _driver;
    HeartBeat &_heartbeat;
    std::thread _thread;

    int _updateAndGetFloor();
    void _stopElevator();
    void _startElevator();
    void _startElevator( Direction );
    void _setButtonLampAndFlag( Button, bool );
    Direction _optimalDirection() const;
    bool _priorityFloorsInDirection( Direction ) const;
    void _emitStateChange( ChangeType, int );
    FloorSet _allButtons() const;
    bool _shouldStop( int ) const;
    void _clearDirectionButtonLamp();

    ElevatorState _elevState;
    Direction _lastDirection;
    MillisecondTime _lastStateUpdate;

    std::vector< Button > _floorButtons;

    static constexpr MillisecondTime _speed = 300;
    static constexpr MillisecondTime _keepAlive = 500;
    // how long to wait before closing doors
    static constexpr MillisecondTime _waitThreshold = 5000;
};

}

#endif // SRC_ELEVATOR_H
