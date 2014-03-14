// C++11    (c) 2014 Vladimír Štill <xstill@fi.muni.cz>

/* High level elevator API/event loop */

#include <elevator/driver.h>
#include <elevator/spinlock.h>
#include <elevator/heartbeat.h>
#include <elevator/concurrentqueue.h>
#include <elevator/command.h>
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
    Elevator( int, HeartBeat &, ConcurrentQueue< Command > * );
    ~Elevator();

    /* spawn control loop thread and run elevator (non blocking) */
    void run();
    void terminate();

    void addTargetFloor( int floor );
    void removeTargetFloor( int floor );
    void setMoveDirection( Direction );

    void assertConsistency();

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

    int _id;
    std::atomic< bool > _terminate;
    AtomicFloorSet _floorsToServe;
    std::vector< Button > _floorButtons;
    std::atomic_flag _lock;
    ConcurrentQueue< Command > *_inCommands;
    Driver _driver;
    HeartBeat &_heartbeat;
    std::thread _thread;

    int _updateAndGetFloor();
    void _stopElevator();
    void _startElevator();
    void _startElevator( Direction );
    Direction _optimalDirection() const;
    bool _floorsInDirection( Direction ) const;

    Direction _direction;
    Direction _lastDirection;
    int _lastFloor;

    static constexpr int _speed = 300;
};

}

#endif // SRC_ELEVATOR_H
