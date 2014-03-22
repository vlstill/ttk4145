#include <climits>
#include <elevator/elevator.h>
#include <elevator/restartwrapper.h>
#include <elevator/test.h>
#include <wibble/raii.h>

namespace elevator {

std::vector< Button > genFloorButtons( const BasicDriverInfo &bi ) {
    std::vector< Button > btns;
    for ( int i = bi.minFloor(); i <= bi.maxFloor(); ++i ) {
        btns.push_back( Button( ButtonType::TargetFloor, i ) );
        if ( i < bi.maxFloor() )
            btns.push_back( Button( ButtonType::CallUp, i ) );
        if ( i > bi.minFloor() )
            btns.push_back( Button( ButtonType::CallDown, i ) );
    }
    return btns;
}

Elevator::Elevator(
        int id,
        ConcurrentQueue< Command > &inCommands,
        ConcurrentQueue< StateChange > &outState
    ) : _terminate( false ),
        _inCommands( inCommands ),
        _outState( outState ),
        _previousDirection( Direction::None ),
        _lastStateUpdate( 0 ),
        _floorButtons( genFloorButtons( _driver ) )
{
    _elevState.lastFloor = _driver.minFloor();
    _elevState.id = id;
}

Elevator::~Elevator() {
    if ( _thread.joinable() )
        terminate();
}

void Elevator::recover( ElevatorState state ) {
    assert( !_thread.joinable() && !_terminate, "cannot recover after start" );
    _elevState = state;
    _elevState.direction = Direction::None;
}

void Elevator::terminate() {
    assert( _thread.joinable(), "control loop not running" );
    _terminate = true;
    _thread.join();
}

void Elevator::run( HeartBeat &heartbeat ) {
    _thread = std::thread( restartWrapper( &Elevator::_loop ), this, &heartbeat );
}

void Elevator::_addTargetFloor( int floor ) {
    _elevState.insideButtons.set( true, floor, _driver );
}

void Elevator::_removeTargetFloor( int floor ) {
    _elevState.insideButtons.set( false, floor, _driver );
}

void Elevator::assertConsistency() {
    assert_lt( _driver.minFloor(), _driver.maxFloor(), "invalid floor bounds" );
    assert( _previousDirection == Direction::Up || _previousDirection == Direction::Down
            || _previousDirection == Direction::None, "invalid direction" );
    _elevState.assertConsistency( _driver );
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
        _previousDirection = _elevState.direction;
    _elevState.direction = Direction::None;
    _driver.stopElevator();
}

void Elevator::_startElevator() {
    _startElevator( Direction::None );
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
    if ( btn.type() == ButtonType::TargetFloor )
        _elevState.insideButtons.set( val, btn.floor(), _driver );
}

ChangeType changeTypeByButton( ButtonType btnt ) {
    switch ( btnt ) {
        case ButtonType::CallUp:
            return ChangeType::ButtonUpPressed;
        case ButtonType::CallDown:
            return ChangeType::ButtonDownPressed;
        case ButtonType::TargetFloor:
            return ChangeType::InsideButtonPresed;
        default: assert_unreachable();
    }
}

FloorSet Elevator::_allButtons() const {
    return _elevState.insideButtons | _elevState.upButtons | _elevState.downButtons;
}

bool Elevator::_shouldStop( int currentFloor ) const {
    if ( _elevState.insideButtons.get( currentFloor, _driver ) )
        return true;
    FloorSet all = _allButtons();
    if ( _elevState.direction == Direction::Up )
        return _elevState.upButtons.get( currentFloor, _driver )
            || ( !all.anyHigher( currentFloor, _driver )
                    && all.get( currentFloor, _driver ) );
    if ( _elevState.direction == Direction::Down )
        return _elevState.downButtons.get( currentFloor, _driver )
            || ( !all.anyLower( currentFloor, _driver )
                    && all.get( currentFloor, _driver ) );
    return (currentFloor == _driver.minFloor() && all.get( currentFloor, _driver ))
        || (currentFloor == _driver.maxFloor() && all.get( currentFloor, _driver ));
}

void Elevator::_clearDirectionButtonLamp() {
    Button b;
    if ( _elevState.lastFloor == _driver.maxFloor() )
        b = Button{ ButtonType::CallDown, _driver.maxFloor() };
    else if ( _elevState.lastFloor == _driver.minFloor() )
        b = Button{ ButtonType::CallUp, _driver.minFloor() };
    else if ( _elevState.direction != Direction::None )
        b = buttonByDirection( _elevState.direction, _elevState.lastFloor );
    else
        return; // no change requred

    if ( b.type() == ButtonType::CallUp ) {
        if ( _elevState.upButtons.set( false, b.floor(), _driver ) )
            _emitStateChange( ChangeType::ServedUp, b.floor() );
    } else {
        if ( _elevState.downButtons.set( false, b.floor(), _driver ) )
            _emitStateChange( ChangeType::ServedDown, b.floor() );
    }
    _driver.setButtonLamp( b, false );
}

void Elevator::_initializeElevator() {
    // initialize according to lights
    for ( int i = _driver.minFloor(); i <= _driver.maxFloor(); ++i ) {
        Button bi{ ButtonType::TargetFloor, i };
        if ( _driver.getButtonLamp( bi ) )
            _elevState.insideButtons.set( true, i, _driver );
        // it can be that in recovered state more buttons were pressed
        if ( _elevState.insideButtons.get( i, _driver ) )
            _driver.setButtonLamp( bi, true );

        // we can't set outside buttons by lights as we would assign even buttons
        // handled by other elevators
        Button bu{ ButtonType::CallUp, i };
        _driver.setButtonLamp( bu, _elevState.upButtons.get( i, _driver ) );

        Button bd{ ButtonType::CallDown, i };
        _driver.setButtonLamp( bd, _elevState.downButtons.get( i, _driver ) );
    }
    if ( _driver.getStopLamp() )
        _elevState.stopped = true;
    if ( _driver.getDoorOpenLamp() )
        _elevState.doorOpen = true;
}

void Elevator::_loop( HeartBeat *heartbeat ) {
    // no matter whether exit is caused by terminate flag or exception
    // we want to stop elevator (ok, it works only for exceptions caught somewhere
    // above, but nothing better exists and we are catching assertions and
    // restarting)
    auto d_stop = wibble::raii::defer( [&]() { _stopElevator(); } );

    // for some buttons, we need to keep track about changes, so that we
    // can detect button press/release event no just the fact that button
    // is now activated
    FloorSet inFloorButtons, inFloorButtonsLast;
    bool stopLast{ false }, stopNow{ false };

    int prevFloor{ INT_MIN }; // to keep track when to send state update

    MillisecondTime doorWaitingStarted{ 0 }; // for closing doors

    enum class State { Normal, WaitingForInButton, Stopped };
    State state = State::Normal;

    _initializeElevator();
    if ( _driver.getStopLamp() )
        state = State::Stopped;

    while ( !_terminate.load( std::memory_order::memory_order_relaxed ) ) {
        // initialize cycle
        inFloorButtonsLast = inFloorButtons;
        inFloorButtons.reset();
        stopLast = stopNow;

        assertConsistency();

        // handle buttons and lamps
        for ( auto b : _floorButtons ) {
            if ( _driver.getButtonSignal( b ) ) {
                if ( !_driver.getButtonLamp( b ) ) { // new press
                    _setButtonLampAndFlag( b, true );
                    if ( b.type() == ButtonType::TargetFloor ) {
                        // we need to serve this one on this elevator
                        _addTargetFloor( b.floor() );
                    }
                    _emitStateChange( changeTypeByButton( b.type() ), b.floor() );
                }
                // furthermore we need to keep track of pressed inside buttons
                // regardless if they were already lit, this is for the purpose
                // of detection re-pressing inside button to get elevator moving
                // after entering
                if ( b.type() == ButtonType::TargetFloor ) {
                    inFloorButtons.set( true, b.floor(), _driver );
                }
            }
        }

        if ( (stopNow = _driver.getStop()) && stopNow != stopLast ) {
            _elevState.stopped = !_driver.getStopLamp();
            _driver.setStopLamp( _elevState.stopped );

            /* stop button will (surprise) stop the elevator right where it is
             * movement is resumed once button is presses again */
            if ( _elevState.stopped ) {
                _stopElevator();
                state = State::Stopped;
                _emitStateChange( ChangeType::OtherChange, _updateAndGetFloor() );
            } else {
                _startElevator( _previousDirection );
                state = State::Normal;
                _emitStateChange( ChangeType::OtherChange, _updateAndGetFloor() );
            }
        }

        if ( _driver.getObstruction() ) {
            _driver.shutdown();
            while ( _driver.getObstruction() ) {
                /* obstruction causes infinite loop which it turn causes
                 * heartbeat timeout and terminates whole process */
            }
        }

        // must be nonblocking
        auto maybeCommand = _inCommands.tryDequeue();
        if ( !maybeCommand.isNothing() ) {
            auto command = maybeCommand.value();
            assert( command.targetElevatorId == _elevState.id
                    || command.targetElevatorId == Command::ANY_ID, "command to other elevator" );
            switch ( command.commandType ) {
            case CommandType::Empty:
                break;
            case CommandType::CallToFloorAndGoUp:
                _elevState.upButtons.set( true, command.targetFloor, _driver );
                _driver.setButtonLamp( Button{ ButtonType::CallUp, command.targetFloor }, true );
                _emitStateChange( ChangeType::GoingToServeUp, command.targetFloor );
                break;
            case CommandType::CallToFloorAndGoDown:
                _elevState.downButtons.set( true, command.targetFloor, _driver );
                _driver.setButtonLamp( Button{ ButtonType::CallDown, command.targetFloor }, true );
                _emitStateChange( ChangeType::GoingToServeDown, command.targetFloor );
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
                _elevState.doorOpen = true;
                state = State::WaitingForInButton;
                doorWaitingStarted = now();
                _stopElevator();
                // this floor is served
                _removeTargetFloor( currentFloor );
                _emitStateChange( ChangeType::Served, currentFloor );
            } else if ( _elevState.direction == Direction::None ) {
                if ( _allButtons().hasAny() ) {
                    // we are not moving but we can
                    if ( _priorityFloorsInDirection( _previousDirection ) )
                        _startElevator( _previousDirection );
                    else
                        _startElevator(); // decides which direction is better itself

                    _emitStateChange( ChangeType::OtherChange, currentFloor );
                }
                _clearDirectionButtonLamp();
            }

        } else if ( state == State::WaitingForInButton ) {
            bool timeout = (doorWaitingStarted + _waitThreshold) < now();
            if ( FloorSet::hasAdditional( inFloorButtonsLast, inFloorButtons ) || timeout )
            {
                _driver.setDoorOpenLamp( false );
                _elevState.doorOpen = false;
                state = State::Normal;
                _emitStateChange( ChangeType::OtherChange, currentFloor );
                if ( timeout ) {
                    _elevState.downButtons.set( false, currentFloor, _driver );
                    _elevState.upButtons.set( false, currentFloor, _driver );
                    _driver.setButtonLamp( Button{ ButtonType::CallUp, currentFloor }, false );
                    _driver.setButtonLamp( Button{ ButtonType::CallDown, currentFloor }, false );
                    _emitStateChange( ChangeType::ServedUp, currentFloor );
                    _emitStateChange( ChangeType::ServedDown, currentFloor );
                } else {
                    _clearDirectionButtonLamp();
                }
            }
        } else if ( state == State::Stopped ) {
            // nothing to do
        } else {
            assert_unreachable();
        }

        if ( _lastStateUpdate + keepAlive <= now() )
            _emitStateChange( ChangeType::KeepAlive, currentFloor );

        // it is important to do heartbeat at the end so that we don't end up
        // beating even in case we are repeatedlay auto-restarted due to assertion
        // we don't need to care about beating too often, it is cheap and safe
        heartbeat->beat();
        prevFloor = currentFloor;
    }
}

}
