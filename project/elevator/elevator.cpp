#include <elevator/elevator.h>
#include <elevator/restartwrapper.h>
#include <elevator/test.h>
#include <wibble/raii.h>
#include <thread>

namespace elevator {

Elevator::Elevator( int id, HeartBeat &heartbeat, ConcurrentQueue< Command > *inCommands ) :
    _id( id ), _terminate( false ), _floorsToServe( 0 ), _inCommands( inCommands ),
    _heartbeat( heartbeat ), _direction( Direction::None ), _lastDirection( Direction::None ),
    _lastFloor( _driver.minFloor() )
{
    _lock.clear();
    for ( int i = 0; i < 4; ++i ) {
        _floorButtons.push_back( Button( ButtonType::TargetFloor, i + 1 ) );
        if ( i < 3 )
            _floorButtons.push_back( Button( ButtonType::CallUp, i + 1 ) );
        if ( i != 0 )
            _floorButtons.push_back( Button( ButtonType::CallDown, i + 1 ) );
    }
}
Elevator::~Elevator() {
    if ( _thread.joinable() ) {
        _terminate = true;
        _thread.join();
    }
}

void Elevator::terminate() {
    assert( _thread.joinable(), "control loop not running" );
    _terminate = true;
    _thread.join();
}

void Elevator::run() {
    _thread = std::thread( restartWrapper( &Elevator::_loop ), this );
}

void Elevator::addTargetFloor( int floor ) {
    _floorsToServe.set( true, floor, _driver );
}

void Elevator::removeTargetFloor( int floor ) {
    _floorsToServe.set( false, floor, _driver );
}

void Elevator::assertConsistency() {
    assert( _driver.alive(), "elevator hardware error" );
    assert_lt( _driver.minFloor(), _driver.maxFloor(), "invalid floor bounds" );
    assert( _direction == Direction::Up || _direction == Direction::Down
            || _direction == Direction::None, "invalid direction" );
    assert( _lastDirection == Direction::Up || _direction == Direction::Down
            || _direction == Direction::None, "invalid direction" );
    assert( _floorsToServe.consistent( _driver ), "invalid floor set to serve" );
    assert_leq( _driver.minFloor(), _lastFloor, "last floor out of bounds" );
}

Button buttonByDirection( Direction dir, int floor ) {
    assert_neq( dir, Direction::None, "Need direction" );

    return dir == Direction::Down
        ? Button( ButtonType::CallDown, floor )
        : Button( ButtonType::CallUp, floor );
}

int Elevator::_updateAndGetFloor() {
    int f = _driver.getFloor();
    if ( f != INT_MIN )
        _lastFloor = f;
    return f;
}

void Elevator::_stopElevator() {
    if ( _direction != Direction::None )
        _lastDirection = _direction;
    _driver.stopElevator();
}

void Elevator::_startElevator() {
    _startElevator( _lastDirection );
}

void Elevator::_startElevator( Direction direction ) {
    _direction = direction;
    // we may not have current information in _lastFloor, on the other hand
    // sensor never lies but may not know
    _updateAndGetFloor(); // update information if sensor senses floor
    if ( _lastFloor == _driver.minFloor() )
        _direction = Direction::Up;
    else if ( _lastFloor == _driver.maxFloor() )
        _direction = Direction::Down;
    // if we have no clue select direction by requests
    if ( _direction == Direction::None )
        _direction = _optimalDirection();
    _driver.setMotorSpeed( _direction, _speed );
}

Direction Elevator::_optimalDirection() const {
    const int floors = _driver.maxFloor() - _driver.minFloor() + 1;
    int lowerThan{ 0 }, higherThan { 0 };
    for ( int i = 0; i < floors; ++i ) {
        if ( _floorsToServe.get( i, _driver ) )
            continue;
        if ( i > _lastFloor )
            ++higherThan;
        if ( i < _lastFloor )
            ++lowerThan;
    }
    return higherThan >= lowerThan ? Direction::Up : Direction::Down;
}

bool Elevator::_floorsInDirection( Direction direction ) const {
    if ( direction == Direction::Down )
        return _floorsToServe.anyLower( _lastFloor, _driver );
    if ( direction == Direction::Up )
        return _floorsToServe.anyHigher( _lastFloor, _driver );
    return false;
}

void Elevator::_loop() {
    // no matter whether exit is caused by terminate flag or exception
    // we want to stop elevator (ok, it works only for exceptions caught somewhere
    // above, but nothing better exists and we are catching assertions and
    // restarting)
    wibble::raii::defer( [&]() { _stopElevator(); } );

    // for some buttons, we need to keep track about changes
    FloorSet inFloorButtons, inFloorButtonsLast;
    bool stopLast{ false }, stopNow{ false };

    enum class State { Normal, WaitingForInButton, Stopped };
    State state = State::Normal;

    while ( !_terminate.load( std::memory_order::memory_order_relaxed ) ) {
        // initialize cycle
        inFloorButtonsLast = inFloorButtons;
        inFloorButtons.reset();
        stopLast = stopNow;

        // whole loop body is locked to prevent concurrent run with dirrectCommand
        SpinLock guard( _lock );

        assertConsistency();

        // handle buttons and lamps
        for ( auto b : _floorButtons ) {
            if ( _driver.getButtonSignal( b ) ) {
                if ( !_driver.getButtonLamp( b ) ) { // new press
                    _driver.setButtonLamp( b, true );
                    if ( b.type() == ButtonType::TargetFloor )
                        // we need to serve this one on this elevator
                        addTargetFloor( b.floor() );
                }
                // furthermore we need to keep track of pressed inside buttons
                // regardless if they were already lit, this is for the purpose
                // of detection re-pressing inside button to get elevator moving
                // after entering
                if ( b.type() == ButtonType::TargetFloor )
                    inFloorButtons.set( true, b.floor(), _driver );
            }
        }
        if ( (stopNow = _driver.getStop()) && stopNow != stopLast ) {
            _driver.setStopLamp( stopNow );

            /* stop button will (surprise) stop the elevator right where it is
             * movement is resumed once button is presses again */
            if ( stopNow ) {
                _stopElevator();
                state = State::Stopped;
            } else {
                state = State::Normal;
                _startElevator();
            }
        }
        if ( _driver.getObstruction() ) {
            _driver.stopElevator();
            while ( _driver.getObstruction() ) {
                /* obstruction causes infinite loop which it turn causes
                 * heartbeat timeout, which will of course repeat while
                 * obstruction is pressed */
            }
        }

        int currentFloor = _updateAndGetFloor();
        // safety precautions
        if ( currentFloor == _driver.maxFloor() && _direction == Direction::Up )
            _stopElevator();
        if ( currentFloor == _driver.minFloor() && _direction == Direction::Down )
            _stopElevator();

        if ( currentFloor != INT_MIN )
            _driver.setFloorIndicator( currentFloor );

        // note: we are here working wich _floorsToServe shared atomic variable
        // it can change between subsequent loads, but only this thread
        // ever removes any floor from it, so there is nothing dangerous in this

        // now do what is needed depending on state
        if ( state == State::Normal ) {
            if ( _direction != Direction::None ) {

                // check if we arrived at any floor which was scheduled for us
                if ( currentFloor != INT_MIN && _floorsToServe.get( currentFloor, _driver ) ) {
                    _stopElevator();
                    // turn off button lights
                    _driver.setButtonLamp( Button( ButtonType::TargetFloor, currentFloor ), false );
                    _driver.setButtonLamp( buttonByDirection( _direction, currentFloor ), false );
                    // open doors
                    _driver.setDoorOpenLamp( true );
                    state = State::WaitingForInButton;
                    // this floor is served (??)
                    removeTargetFloor( currentFloor );
                }
            } else if ( _floorsToServe.hasAny() ) {
                // we are not moving but we can
                if ( _floorsInDirection( _lastDirection ) )
                    _startElevator( _lastDirection );
                else
                    _startElevator( Direction::None ); // decides which direction is better itself
            }

        } else if ( state == State::WaitingForInButton ) {
            if ( FloorSet::hasAdditional( inFloorButtonsLast, inFloorButtons ) ) {
                _driver.setDoorOpenLamp( false );
                state = State::Normal;
            }
        } else if ( state == State::Stopped ) {
            // nothing to do
        } else {
            assert_unreachable();
        }

        // it is important to do heartbeat at the end so that we don't end up
        // beating even in case we are repeatedlay auto-restarted due to assertion
        // we don't need to care about beating too often, it is cheap and safe
        _heartbeat.beat();
    }
}

}
