#include <climits>
#include <elevator/elevator.h>
#include <elevator/restartwrapper.h>
#include <elevator/test.h>
#include <wibble/raii.h>
#include <thread>

namespace elevator {

Elevator::Elevator(
        int id,
        HeartBeat &heartbeat,
        ConcurrentQueue< Command > &inCommands,
        ConcurrentQueue< StateChange > &outState
    ) : _terminate( false ),
        _inCommands( inCommands ),
        _outState( outState ),
        _heartbeat( heartbeat ),
        _lastDirection( Direction::None ),
        _lastStateUpdate( 0 )
{
    _lock.clear();
    for ( int i = 0; i < 4; ++i ) {
        _floorButtons.push_back( Button( ButtonType::TargetFloor, i + 1 ) );
        if ( i < 3 )
            _floorButtons.push_back( Button( ButtonType::CallUp, i + 1 ) );
        if ( i != 0 )
            _floorButtons.push_back( Button( ButtonType::CallDown, i + 1 ) );
    }
    _elevState.lastFloor = _driver.minFloor();
    _elevState.id = id;
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
    _elevState.insideButtons.set( true, floor, _driver );
}

void Elevator::removeTargetFloor( int floor ) {
    _elevState.insideButtons.set( false, floor, _driver );
}

void Elevator::assertConsistency() {
    assert_lt( _driver.minFloor(), _driver.maxFloor(), "invalid floor bounds" );
    assert( _elevState.direction == Direction::Up || _elevState.direction == Direction::Down
            || _elevState.direction == Direction::None, "invalid direction" );
    assert( _lastDirection == Direction::Up || _lastDirection == Direction::Down
            || _lastDirection == Direction::None, "invalid direction" );
    assert( _elevState.insideButtons.consistent( _driver ), "invalid floor set" );
    assert( _elevState.upButtons.consistent( _driver ), "invalid floor set" );
    assert( _elevState.downButtons.consistent( _driver ), "invalid floor set" );
    assert_leq( _driver.minFloor(), _elevState.lastFloor, "last floor out of bounds" );
    assert_leq( _elevState.lastFloor, _driver.maxFloor(), "last floor out of bounds" );
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
        _elevState.lastFloor = f;
    return f;
}

void Elevator::_stopElevator() {
    if ( _elevState.direction != Direction::None )
        _lastDirection = _elevState.direction;
    _elevState.direction = Direction::None;
    _driver.stopElevator();
}

void Elevator::_startElevator() {
    _startElevator( _lastDirection );
}

void Elevator::_startElevator( Direction direction ) {
    _elevState.direction = direction;
    // we may not have current information in _elevState.lastFloor, on the other hand
    // sensor never lies but may not know
    _updateAndGetFloor(); // update information if sensor senses floor
    if ( _elevState.lastFloor == _driver.minFloor() )
        _elevState.direction = Direction::Up;
    else if ( _elevState.lastFloor == _driver.maxFloor() )
        _elevState.direction = Direction::Down;
    // if we have no clue select direction by requests
    if ( _elevState.direction == Direction::None )
        _elevState.direction = _optimalDirection();
    _driver.setMotorSpeed( _elevState.direction, _speed );
    _driver.setDoorOpenLamp( false );
}

Direction Elevator::_optimalDirection() const {
    int lowerThan{ 0 }, higherThan { 0 };
    FloorSet floorsToServe = _elevState.insideButtons;
    if ( !floorsToServe.hasAny() ) {
        floorsToServe |= _elevState.upButtons;
        floorsToServe |= _elevState.downButtons;
    }
    for ( int i = _driver.minFloor(); i <= _driver.maxFloor(); ++i ) {
        if ( floorsToServe.get( i, _driver ) ) {
            if ( i > _elevState.lastFloor )
                ++higherThan;
            if ( i < _elevState.lastFloor )
                ++lowerThan;
        }
    }
    return higherThan >= lowerThan ? Direction::Up : Direction::Down;
}

/* we want to prioritize inside buttons, imagine:
 * - elevator is on floor 3 and last went up, inside is pressed 2, outside 4
 *   normally we preffer continuing withing last direction to avoid malicious
 *   co-travellers from calling elevator to other direction from inside,
 *   but this must work only for inside buttons, we don't want elevator
 *   to be called to floor 4 in this situation
 */
bool Elevator::_priorityFloorsInDirection( Direction direction ) const {
    if ( direction == Direction::Down )
        return _elevState.insideButtons.anyLower( _elevState.lastFloor, _driver );
    if ( direction == Direction::Up )
        return _elevState.insideButtons.anyHigher( _elevState.lastFloor, _driver );
    return false;
}

void Elevator::_emitStateChange( ChangeType type, int floor ) {
    StateChange change;
    change.state = _elevState;
    change.changeType = type;
    change.changeFloor = floor;
    _lastStateUpdate = change.state.timestamp = now();
    _outState.enqueue( change );
}

void Elevator::_setButtonLampAndFlag( Button btn, bool val ) {
    _driver.setButtonLamp( btn, val );
    FloorSet *toUpdate = nullptr;
    if ( btn.type() == ButtonType::CallUp )
        toUpdate = &_elevState.upButtons;
    else if ( btn.type() == ButtonType::CallDown )
        toUpdate = &_elevState.downButtons;
    else if ( btn.type() == ButtonType::TargetFloor )
        toUpdate = &_elevState.insideButtons;
    assert_neq( toUpdate, nullptr, "unhandled button type" );
    toUpdate->set( val, btn.floor(), _driver );
}

ChangeType changeTypeByButton( ButtonType btnt ) {
    switch ( btnt ) {
    case ButtonType::CallUp:
        return ChangeType::ButtonDownPressed;
    case ButtonType::CallDown:
        return ChangeType::ButtonUpPressed;
    case ButtonType::TargetFloor:
        return ChangeType::InsideButtonPresed;
    default: assert_unreachable();
    }
}

FloorSet Elevator::_allButtons() const {
    return _elevState.insideButtons | _elevState.upButtons | _elevState.downButtons;
}

bool Elevator::_shouldStop( int currentFloor ) const {
    /* stop if this floor was requested from inside or if we are moving
     * in direction of pressed outside button
     * the special case of topmost/bottommost floor need not to be
     * handled as elevator stops there anyway
     */
    return   _elevState.insideButtons.get( currentFloor, _driver )
        || ( _elevState.direction == Direction::Up
                && _elevState.upButtons.get( currentFloor, _driver ) )
        || ( _elevState.direction == Direction::Down
                && _elevState.downButtons.get( currentFloor, _driver ) );
}

void Elevator::_clearDirectionButtonLamp() {
    Button b;
    if ( _elevState.lastFloor == _driver.maxFloor() )
        b = Button{ ButtonType::CallDown, _driver.maxFloor() };
    else if ( _elevState.lastFloor == _driver.minFloor() )
        b = Button{ ButtonType::CallUp, _driver.minFloor() };
    else
        b = buttonByDirection( _elevState.direction, _elevState.lastFloor );

    if ( b.type() == ButtonType::CallUp ) {
        if ( _elevState.upButtons.set( false, b.floor(), _driver ) )
            _emitStateChange( ChangeType::ServedUp, b.floor() );
    } else {
        if ( _elevState.downButtons.set( false, b.floor(), _driver ) )
            _emitStateChange( ChangeType::ServedDown, b.floor() );
    }
    _driver.setButtonLamp( b, false );
}

void Elevator::_loop() {
    // no matter whether exit is caused by terminate flag or exception
    // we want to stop elevator (ok, it works only for exceptions caught somewhere
    // above, but nothing better exists and we are catching assertions and
    // restarting)
    auto d_stop = wibble::raii::defer( [&]() { _stopElevator(); } );

    // for some buttons, we need to keep track about changes
    FloorSet inFloorButtons, inFloorButtonsLast;
    bool stopLast{ false }, stopNow{ false };
    int prevFloor = INT_MIN;
    MillisecondTime waitingStarted{ 0 };

    enum class State { Normal, WaitingForInButton, Stopped };
    State state = State::Normal;

    // initialize according to lights
    if ( _driver.getStopLamp() )
        state = State::Stopped;
    for ( auto b : _floorButtons ) {
        if ( _driver.getButtonLamp( b ) ) {
            if ( b.type() == ButtonType::TargetFloor )
                addTargetFloor( b.floor() );
            else if ( b.type() == ButtonType::CallDown )
                _elevState.downButtons.set( true, b.floor(), _driver );
            else if ( b.type() == ButtonType::CallUp )
                _elevState.upButtons.set( true, b.floor(), _driver );
        }
    }

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
                    _setButtonLampAndFlag( b, true );
                    if ( b.type() == ButtonType::TargetFloor )
                        // we need to serve this one on this elevator
                        addTargetFloor( b.floor() );
                    _emitStateChange( changeTypeByButton( b.type() ), b.floor() );
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
            bool stopValue = !_driver.getStopLamp();
            _driver.setStopLamp( stopValue );

            /* stop button will (surprise) stop the elevator right where it is
             * movement is resumed once button is presses again */
            if ( stopValue ) {
                _stopElevator();
                state = State::Stopped;
                _elevState.stopped = true;
                _emitStateChange( ChangeType::OtherChange, _updateAndGetFloor() );
            } else {
                state = State::Normal;
                _startElevator();
                _driver.setDoorOpenLamp( false );
                _elevState.stopped = false;
                _emitStateChange( ChangeType::OtherChange, _updateAndGetFloor() );
            }
        }
        if ( _driver.getObstruction() ) {
            _stopElevator();
            for ( auto b : _floorButtons )
                _driver.setButtonLamp( b, false );
            _driver.setStopLamp( false );
            _driver.setDoorOpenLamp( false );
            while ( _driver.getObstruction() ) {
                /* obstruction causes infinite loop which it turn causes
                 * heartbeat timeout and terminates whole process */
            }
        }
        auto maybeCommand = _inCommands.tryDequeue();
        if ( !maybeCommand.isNothing() ) {
            auto command = maybeCommand.value();
            assert_eq( command.targetElevatorId, _elevState.id, "command to other elevator" );
            switch ( command.commandType ) {
            case CommandType::Empty:
                break;
            case CommandType::CallToFloorAndGoUp:
                _elevState.upButtons.set( true, command.targetFloor, _driver );
                _driver.setButtonLamp( Button{ ButtonType::CallUp, command.targetFloor }, true );
                break;
            case CommandType::CallToFloorAndGoDown:
                _elevState.downButtons.set( true, command.targetFloor, _driver );
                _driver.setButtonLamp( Button{ ButtonType::CallDown, command.targetFloor }, true );
                break;
            case CommandType::TurnOnLightUp:
                _driver.setButtonLamp( Button{ ButtonType::CallUp, command.targetFloor }, true );
                break;
            case CommandType::TurnOffLightUp:
                _driver.setButtonLamp( Button{ ButtonType::CallUp, command.targetFloor }, false );
                break;
            case CommandType::TurnOnLightDown:
                _driver.setButtonLamp( Button{ ButtonType::CallDown, command.targetFloor }, true );
                break;
            case CommandType::TurnOffLightDown:
                _driver.setButtonLamp( Button{ ButtonType::CallDown, command.targetFloor }, false );
                break;
            }
        }

        int currentFloor = _updateAndGetFloor();
        // safety precautions
        if ( currentFloor == _driver.maxFloor() && _elevState.direction == Direction::Up )
            _stopElevator();
        if ( currentFloor == _driver.minFloor() && _elevState.direction == Direction::Down )
            _stopElevator();

        if ( currentFloor != INT_MIN )
            _driver.setFloorIndicator( currentFloor );

        if ( currentFloor != prevFloor )
            _emitStateChange( ChangeType::OtherChange, currentFloor );

        // note: we are here working wich _floorsToServe shared atomic variable
        // it can change between subsequent loads, but only this thread
        // ever removes any floor from it, so there is nothing dangerous in this

        // now do what is needed depending on state
        if ( state == State::Normal ) {

            // check if we arrived at any floor which was scheduled for us
            if ( currentFloor != INT_MIN && _shouldStop( currentFloor ) ) {
                // turn off button lights
                _setButtonLampAndFlag( Button( ButtonType::TargetFloor, currentFloor ), false );
                // open doors
                _driver.setDoorOpenLamp( true );
                state = State::WaitingForInButton;
                waitingStarted = now();
                _stopElevator();
                // this floor is served
                removeTargetFloor( currentFloor );
                _emitStateChange( ChangeType::Served, currentFloor );
            } else if ( _elevState.direction == Direction::None && _allButtons().hasAny() ) {
                // we are not moving but we can
                if ( _priorityFloorsInDirection( _lastDirection ) )
                    _startElevator( _lastDirection );
                else
                    _startElevator( Direction::None ); // decides which direction is better itself

                _emitStateChange( ChangeType::OtherChange, currentFloor );
                _clearDirectionButtonLamp();
            }

        } else if ( state == State::WaitingForInButton ) {
            if ( FloorSet::hasAdditional( inFloorButtonsLast, inFloorButtons )
                    || (waitingStarted + _waitThreshold) < now() )
            {
                _driver.setDoorOpenLamp( false );
                state = State::Normal;
            }
        } else if ( state == State::Stopped ) {
            // nothing to do
        } else {
            assert_unreachable();
        }

        if ( _lastStateUpdate + _keepAlive <= now() )
            _emitStateChange( ChangeType::KeepAlive, currentFloor );

        // it is important to do heartbeat at the end so that we don't end up
        // beating even in case we are repeatedlay auto-restarted due to assertion
        // we don't need to care about beating too often, it is cheap and safe
        _heartbeat.beat();
        prevFloor = currentFloor;
    }
}

}
