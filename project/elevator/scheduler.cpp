#include <elevator/scheduler.h>
#include <elevator/restartwrapper.h>

namespace elevator {

Scheduler::Scheduler( int localId, HeartBeat &hb, BasicDriverInfo info,
        ConcurrentQueue< StateChange > &stateUpdateIn,
        ConcurrentQueue< StateChange > &stateUpdateOut,
        ConcurrentQueue< Command > &commandsToRemote,
        ConcurrentQueue< Command > &commandsToLocal ) :
    _localElevId( localId ),
    _heartbeat( hb ),
    _bounds( info ),
    _stateUpdateIn( stateUpdateIn ),
    _stateUpdateOut( stateUpdateOut ),
    _commandsToRemote( commandsToRemote ),
    _commandsToLocal( commandsToLocal ),
    _terminate( false )
{ }

Scheduler::~Scheduler() {
    if ( _thr.joinable() ) {
        _terminate = true;
        _thr.join();
    }
}

void Scheduler::run() {
    _thr = std::thread( restartWrapper( &Scheduler::_runLocal ), this );
}

const char *showChange( ChangeType );

void Scheduler::_forwardToTargets( Command comm ) {
    if ( comm.targetElevatorId == Command::ANY_ID || comm.targetElevatorId == _localElevId )
        _commandsToLocal.enqueue( comm );
    if ( comm.targetElevatorId != _localElevId )
        _commandsToRemote.enqueue( comm );
}

void Scheduler::_handleButtonPress( ButtonType type, int floor ) {
    // first setup lights
    Command lights{ type == ButtonType::CallUp
                      ? CommandType::TurnOnLightUp : CommandType::TurnOnLightDown,
                    Command::ANY_ID, floor };
    _forwardToTargets( lights );

    // now find optimal elevator
    int minDistance = INT_MAX;
    int minId = INT_MIN;

    for ( auto &statepair : _globalState.elevators() ) {
        const ElevatorState &state = statepair.second;

        int dist = std::abs( state.lastFloor - floor );

        if ( state.stopped )
            dist += 10 * (_bounds.maxFloor() - _bounds.minFloor()); // penalisation

        if (    ( state.direction == Direction::Up
                        && ( (type == ButtonType::CallUp && floor > state.lastFloor )
                            || floor == _bounds.maxFloor() ) )
            ||
                ( state.direction == Direction::Down
                        && ( (type == ButtonType::CallDown && floor < state.lastFloor )
                            || floor == _bounds.minFloor() ) )
           )
            dist += _bounds.maxFloor() - _bounds.minFloor() + 1; // busy penalisation
        else
            // as a last resort we can schedule floor even to elevator which is
            // running in different direction
            dist += 2 * (_bounds.maxFloor() - _bounds.minFloor() + 1); // penalisation

        if ( dist < minDistance ) {
            minDistance = dist;
            minId = state.id;
        }
    }
    assert_leq( 0, minId, "no minimal distance found" );

    Command comm{ type == ButtonType::CallUp
                      ? CommandType::CallToFloorAndGoUp
                      : CommandType::CallToFloorAndGoDown,
                  minId, floor };
    _forwardToTargets( comm );
}

void Scheduler::_runLocal() {
    while ( !_terminate.load( std::memory_order::memory_order_relaxed ) ) {
        auto maybeUpdate = _stateUpdateIn.timeoutDequeue( _heartbeat.threshold() / 2 );
        if ( !maybeUpdate.isNothing() ) {
            auto update = maybeUpdate.value();
            _globalState.update( update.state );
            std::cerr << "state update: { id = " << update.state.id
                << ", timestamp = " << update.state.timestamp
                << ", changeType = " << showChange( update.changeType )
                << ", changeFloor = " << update.changeFloor << " }" << std::endl;

            if ( update.state.id == _localElevId ) {
                _stateUpdateOut.enqueue( update ); // propagate update

                // each elevator is responsible for scheduling commnads from its hardware
                switch ( update.changeType ) {
                    case ChangeType::None:
                    case ChangeType::KeepAlive:
                    case ChangeType::OtherChange:
                        continue;
                    case ChangeType::ButtonUpPressed:
                        _handleButtonPress( ButtonType::CallUp, update.changeFloor );
                        break;
                    case ChangeType::ButtonDownPressed:
                        _handleButtonPress( ButtonType::CallDown, update.changeFloor );
                        break;
                }
            }
        }

        _heartbeat.beat();
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

        show( OtherChange );
    }
#undef show
    return "<<unknown>>";
}

}
