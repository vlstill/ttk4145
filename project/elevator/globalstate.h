#include <elevator/state.h>
#include <elevator/requestqueue.h>

#ifndef ELEVATOR_GLOBAL_STATE_H
#define ELEVATOR_GLOBAL_STATE_H

namespace elevator {

struct GlobalState {
    /* note on locking (from inside this class):
     * - all methodst that manipulate elevators set must be explicitly locked
     * - on the other hand methods accessing request queue should not be locked
     *   as requset queue is thread-safe
     *   - but those methodst MUST NOT access elevators state
     */
    void update( ElevatorState state ) {
        Guard g{ _lock };
        _elevators[ state.id ] = state;
        updateButtons( g );
    }

    FloorSet upButtons() const { return _upButtons; }
    FloorSet downButtons() const { return _downButtons; }
    std::unordered_map< int, ElevatorState > elevators() const {
        Guard g{ _lock };
        return _elevators;
    }

    bool has( int i ) const {
        Guard g{ _lock };
        return _elevators.find( i ) != _elevators.end();
    }

    ElevatorState get( int i ) const {
        Guard g{ _lock };
        auto it = _elevators.find( i );
        assert_neq( it, _elevators.end(), "state not found" );
        return it->second;
    }

    void assertConsistency( const BasicDriverInfo &bi ) const {
        Guard g{ _lock };
        assert( _upButtons.consistent( bi ), "consistency check failed" );
        assert( _upButtons.consistent( bi ), "consistency check failed" );
        for ( auto &e : _elevators )
            e.second.assertConsistency( bi );
    }

    // has its own locking, which is needed since we are returning reference
    RequestQueue &requests() { return _requests; }

  private:
    using Guard = std::unique_lock< std::mutex >;
    mutable std::mutex _lock;
    FloorSet _upButtons;
    FloorSet _downButtons;
    std::unordered_map< int, ElevatorState > _elevators;
    RequestQueue _requests;

    void updateButtons( Guard & ) {
        _upButtons.reset();
        _downButtons.reset();
        for ( const auto &el : _elevators ) {
            _upButtons |= el.second.upButtons;
            _downButtons |= el.second.downButtons;
        }
    }
};

}

#endif // ELEVATOR_GLOBAL_STATE_H

