#include <elevator/scheduler.h>
#include <elevator/restartwrapper.h>

namespace elevator {

Scheduler::Scheduler( int localId, HeartBeat &hb, ConcurrentQueue< StateChange > &statUpd,
        ConcurrentQueue< Command > &local, ConcurrentQueue< Command > &remote ) :
    _localElevId( localId ),
    _heartbeat( hb ),
    _stateUpdateQueue( statUpd ),
    _localCommands( local ), _remoteCommands( remote ),
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

void Scheduler::_runLocal() {
    while ( !_terminate.load( std::memory_order::memory_order_relaxed ) ) {
        auto maybeUpdate = _stateUpdateQueue.timeoutDequeue( _heartbeat.threshold() / 2 );
        if ( !maybeUpdate.isNothing() ) {
            auto update = maybeUpdate.value();
            _globalState.update( update.state );
            std::cerr << "state update: { id = " << update.state.id
                << ", timestamp = " << update.state.timestamp
                << ", changeType = " << showChange( update.changeType )
                << ", changeFloor = " << update.changeFloor << " }" << std::endl;

            if ( update.state.id == _localElevId ) {
                // each elevator is responsible for scheduling commnads from its hardware
                switch ( update.changeType ) {
                    case ChangeType::None:
                    case ChangeType::KeepAlive:
                        continue;
                    case ChangeType::ButtonUpPressed:
                        _localCommands.enqueue(
                                Command{ CommandType::CallToFloorAndGoUp, _localElevId, update.changeFloor } );
                        break;
                    case ChangeType::ButtonDownPressed:
                        _localCommands.enqueue(
                                Command{ CommandType::CallToFloorAndGoDown, _localElevId, update.changeFloor } );
                        break;
                }
            }
        }

        _heartbeat.beat();
    }
}

const char *showChange( ChangeType t ) {
#define show( X ) if ( t == ChangeType::X ) return #X
    show( None );
    show( KeepAlive );

    show( InsideButtonPresed );
    show( ButtonDownPressed );
    show( ButtonUpPressed );

    show( GoingToServeDown );
    show( GoingToServeUp );

    show( Served );
    show( OnFloor );

    show( Stopped );
    show( Resumed );
#undef show
    return "<<unknown>>";
}

}
