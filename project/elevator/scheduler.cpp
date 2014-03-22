#include <elevator/scheduler.h>
#include <elevator/restartwrapper.h>
#include <elevator/elevator.h>

namespace elevator {

Scheduler::Scheduler( int localId, BasicDriverInfo info, GlobalState &global,
        ConcurrentQueue< StateChange > &stateUpdateIn,
        ConcurrentQueue< StateChange > &stateUpdateOut,
        ConcurrentQueue< Command > &commandsToRemote,
        ConcurrentQueue< Command > &commandsToLocal ) :
    _localElevId( localId ),
    _bounds( info ),
    _globalState( global ),
    _stateUpdateIn( stateUpdateIn ),
    _stateUpdateOut( stateUpdateOut ),
    _commandsToRemote( commandsToRemote ),
    _commandsToLocal( commandsToLocal ),
    _terminate( false )
{ }

Scheduler::~Scheduler() {
    if ( _thrSched.joinable() ) {
        _terminate = true;
        _thrSched.join();
        _thrReq.join();
    }
}

void Scheduler::run( HeartBeat &shed, HeartBeat &req ) {
    _thrSched = std::thread( restartWrapper( &Scheduler::_schedLoop ), this, &shed );
    _thrReq = std::thread( restartWrapper( &Scheduler::_reqCheckLoop ), this, &req );
}

const char *showChange( ChangeType );
const char *showCommand( CommandType );

void Scheduler::_forwardToTargets( Command comm ) {
    if ( comm.targetElevatorId == Command::ANY_ID || comm.targetElevatorId == _localElevId )
        _commandsToLocal.enqueue( comm );
    if ( comm.targetElevatorId != _localElevId )
        _commandsToRemote.enqueue( comm );
}

int Scheduler::_optimalElevator( ButtonType type, int floor ) {
    int minDistance = INT_MAX;
    int minId = INT_MIN;

    MillisecondTime now = elevator::now();
    // somawhat arbitrary thresholds for time-aware scheduling
    MillisecondTime outdatedThresh = Elevator::keepAlive * 1.2;
    MillisecondTime deadThresh = Elevator::keepAlive * 3;

    for ( auto &statepair : _globalState.elevators() ) {
        const ElevatorState &state = statepair.second;

        MillisecondTime age = now - state.timestamp;

        int dist = std::abs( state.lastFloor - floor );

        // time-aware penasisation
        if ( age > outdatedThresh )
            dist += 10 * (_bounds.maxFloor() - _bounds.minFloor());
        if ( age > deadThresh )
            continue; // skip this one completely, it is most likely dead
        // note: At least local cannot be dead so we should always find minumum

        // penalizations for non-idle elevators
        if ( state.stopped )
            dist += 10 * (_bounds.maxFloor() - _bounds.minFloor()); // stopped penalization

        if (    ( state.direction == Direction::Up
                        && ( (type == ButtonType::CallUp && floor > state.lastFloor )
                            || floor == _bounds.maxFloor() ) )
            ||
                ( state.direction == Direction::Down
                        && ( (type == ButtonType::CallDown && floor < state.lastFloor )
                            || floor == _bounds.minFloor() ) )
           )
            dist += _bounds.maxFloor() - _bounds.minFloor() + 1; // busy penalization
        else if ( state.direction != Direction::None ) // not idle
            // as a last resort we can schedule floor even to elevator which is
            // running in different direction
            dist += 2 * (_bounds.maxFloor() - _bounds.minFloor() + 1); // penalization

        if ( dist < minDistance ) {
            minDistance = dist;
            minId = state.id;
        }
    }
    assert_leq( 0, minId, "no minimal distance found" );
    return minId;
}

void Scheduler::_resendRequest( Request r ) {
    std::cerr << "WARNING: re-sending request: { type = "
              << (r.type == RequestType::NotAcknowledged ? "NotAcknowledged" : "NotDone")
              << ", repeated = " << r.repeated << ", command = { elevator = "
              << r.command.targetElevatorId << ", floor = " << r.command.targetFloor
              << ", type = " << showCommand( r.command.commandType ) << " } }" << std::endl;
    ++r.repeated;
    if ( r.repeated > 3 || r.type == RequestType::NotDone ) {
        // reschedule
        r.command.targetElevatorId = _optimalElevator(
                r.command.commandType == CommandType::CallToFloorAndGoUp
                    ? ButtonType::CallUp : ButtonType::CallDown,
                r.command.targetFloor );
        r.type = RequestType::NotAcknowledged;
    }
    r.updateDeadline();
    _globalState.requests().push( r );
    _forwardToTargets( r.command );
}

void Scheduler::_addAndForwardRequest( Command comm ) {
    Request r( comm, 100 ); // we need to be fast here
    _globalState.requests().push( r );
    _forwardToTargets( comm );
}

void Scheduler::_handleButtonPress( int updateElId, ButtonType type, int floor ) {
    // first setup lights
    Command lights{ type == ButtonType::CallUp
                      ? CommandType::TurnOnLightUp : CommandType::TurnOnLightDown,
                    Command::ANY_ID, floor };
    _forwardToTargets( lights );

    // each elevator schedules changes originating from it
    if ( updateElId == _localElevId ) {
        int minId = _optimalElevator( type, floor );

        Command comm{ type == ButtonType::CallUp
                          ? CommandType::CallToFloorAndGoUp
                          : CommandType::CallToFloorAndGoDown,
                      minId, floor };
        _addAndForwardRequest( comm );
    }
}

void Scheduler::_schedLoop( HeartBeat *heartbeat ) {
    while ( !_terminate.load( std::memory_order::memory_order_relaxed ) ) {
        auto maybeUpdate = _stateUpdateIn.timeoutDequeue( heartbeat->threshold() / 10 );
        if ( !maybeUpdate.isNothing() ) {
            auto update = maybeUpdate.value();
            _globalState.update( update.state );
            std::cerr << "state update: { id = " << update.state.id
                << ", timestamp = " << update.state.timestamp
                << ", changeType = " << showChange( update.changeType )
                << ", changeFloor = " << update.changeFloor
                << ", stopped = " << update.state.stopped
                << ", direction = " << int( update.state.direction )
                << " }" << std::endl;

            if ( update.state.id == _localElevId ) {
                _stateUpdateOut.enqueue( update ); // propagate update
            }

            // each elevator is responsible for scheduling commnads from its hardware
            switch ( update.changeType ) {
                case ChangeType::None:
                case ChangeType::KeepAlive:
                case ChangeType::OtherChange:
                case ChangeType::InsideButtonPresed:
                case ChangeType::Served:
                    break;
                case ChangeType::ButtonUpPressed:
                    _handleButtonPress( update.state.id, ButtonType::CallUp, update.changeFloor );
                    break;
                case ChangeType::ButtonDownPressed:
                    _handleButtonPress( update.state.id, ButtonType::CallDown, update.changeFloor );
                    break;
                case ChangeType::ServedDown:
                    _forwardToTargets( Command{ CommandType::TurnOffLightDown,
                            _localElevId, update.changeFloor } );
                    _globalState.requests().ackRequest( update );
                    break;
                case ChangeType::ServedUp:
                    _forwardToTargets( Command{ CommandType::TurnOffLightUp,
                            _localElevId, update.changeFloor } );
                    _globalState.requests().ackRequest( update );
                    break;
                case ChangeType::GoingToServeUp:
                case ChangeType::GoingToServeDown:
                    // the deadline here is quite high, because it takes time
                    // to do the job
                    _globalState.requests().ackRequest( update, 30 * 1000 );
                    break;
            }
        }

        heartbeat->beat();
    }
}

void Scheduler::_reqCheckLoop( HeartBeat *heartbeat ) {
    while ( !_terminate.load( std::memory_order::memory_order_relaxed ) ) {

        auto mreq = _globalState.requests().waitForEarliestDeadline( heartbeat->threshold() / 10 );
        if ( !mreq.isNothing() )
            _resendRequest( mreq.value() );

        heartbeat->beat();
    }
}

const char *showChange( ChangeType t ) {
#define show( X ) case ChangeType::X: return #X
    switch ( t ) {
        show( None );
        show( KeepAlive );

        show( InsideButtonPresed );
        show( ButtonDownPressed );
        show( ButtonUpPressed );

        show( Served );
        show( ServedUp );
        show( ServedDown );

        show( GoingToServeDown );
        show( GoingToServeUp );

        show( OtherChange );
    }
#undef show
    return "<<unknown>>";
}

const char *showCommand( CommandType t ) {
#define show( X ) case CommandType::X: return #X
    switch ( t ) {
        show( Empty );
        show( CallToFloorAndGoUp );
        show( CallToFloorAndGoDown );
        show( TurnOffLightUp );
        show( TurnOffLightDown );
        show( TurnOnLightDown );
        show( TurnOnLightUp );
    }
#undef show
    return "<<unknown>>";
}


}
