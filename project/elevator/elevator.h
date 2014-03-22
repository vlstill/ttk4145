// C++11    (c) 2014 Vladimír Štill <xstill@fi.muni.cz>

/* High level elevator API/event loop */

#include <elevator/driver.h>
#include <elevator/heartbeat.h>
#include <elevator/concurrentqueue.h>
#include <elevator/command.h>
#include <elevator/state.h>
#include <elevator/floorset.h>

#include <atomic>
#include <thread>
#include <vector>


#ifndef SRC_ELEVATOR_H
#define SRC_ELEVATOR_H

namespace elevator {

struct Elevator {
    Elevator( int, ConcurrentQueue< Command > &, ConcurrentQueue< StateChange > & );
    ~Elevator();

    /* spawn control loop thread and run elevator (non blocking) */
    void run( HeartBeat & );
    void terminate();

    void assertConsistency();

    BasicDriverInfo info() const {
        return BasicDriverInfo( _driver );
    }

    void recover( ElevatorState state );

    // maximum time between state update packets
    static constexpr MillisecondTime keepAlive = 500;

  private:
    void _loop( HeartBeat * );

    std::atomic< bool > _terminate;
    ConcurrentQueue< Command > &_inCommands;
    ConcurrentQueue< StateChange > &_outState;
    Driver _driver;
    std::thread _thread;

    ElevatorState _elevState;
    Direction _previousDirection;
    MillisecondTime _lastStateUpdate;

    const std::vector< Button > _floorButtons;

    void _addTargetFloor( int floor );
    void _removeTargetFloor( int floor );
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
    void _initializeElevator();

    static constexpr MillisecondTime _speed = 300;
    // how long to wait before closing doors
    static constexpr MillisecondTime _waitThreshold = 5000;
};

}

#endif // SRC_ELEVATOR_H
