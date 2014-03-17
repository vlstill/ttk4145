#include <climits>
#include <unordered_map>
#include <tuple>

#include <elevator/driver.h>
#include <elevator/serialization.h>
#include <elevator/floorset.h>

#ifndef SRC_STATE_H
#define SRC_STATE_H

namespace elevator {

enum class ChangeType {
    None,
    KeepAlive,

    InsideButtonPresed,
    ButtonDownPressed,
    ButtonUpPressed,

    Served,
    ServedUp,
    ServedDown,

    OtherChange
};

struct ElevatorState {

    using Tuple = std::tuple< int, int, int, Direction, bool, bool, FloorSet, FloorSet, FloorSet >;

    ElevatorState( Tuple tuple ) :
        id( std::get< 0 >( tuple ) ),
        timestamp( std::get< 1 >( tuple ) ),
        lastFloor( std::get< 2 >( tuple ) ),
        direction( std::get< 3 >( tuple ) ),
        stopped( std::get< 4 >( tuple ) ),
        doorOpen( std::get< 5 >( tuple ) ),
        insideButtons( std::get< 6 >( tuple ) ),
        upButtons( std::get< 7 >( tuple ) ),
        downButtons( std::get< 8 >( tuple ) )
    { }
    ElevatorState() : id( INT_MIN ), timestamp( 0 ), lastFloor( INT_MIN ),
        direction( Direction::None ), stopped( true ), doorOpen( true )
    { }

    Tuple tuple() const {
        return std::make_tuple( id, timestamp, lastFloor, direction,
                stopped, doorOpen, insideButtons, upButtons, downButtons );
    }

    void assertConsistency( const BasicDriverInfo &bi ) const {
        assert_leq( 0, id, "invalid elevator id" );
        assert( insideButtons.consistent( bi ), "invalid floor set" );
        assert( upButtons.consistent( bi ), "invalid floor set" );
        assert( downButtons.consistent( bi ), "invalid floor set" );
        assert_leq( bi.minFloor(), lastFloor, "invalid lastFloor" );
        assert( direction == Direction::Up || direction == Direction::Down
            || direction == Direction::None, "invalid direction" );
    }

    int id;
    long timestamp;
    int lastFloor;
    Direction direction;
    bool stopped;
    bool doorOpen;
    FloorSet insideButtons;
    FloorSet upButtons;
    FloorSet downButtons;
};

struct StateChange {
    using Tuple = std::tuple< ChangeType, int, ElevatorState >;

    StateChange() : changeType( ChangeType::None ), changeFloor( INT_MIN ) { }
    StateChange( Tuple tuple ) :
        changeType( std::get< 0 >( tuple ) ),
        changeFloor( std::get< 1 >( tuple ) ),
        state( std::get< 2 >( tuple ) )
    { }

    Tuple tuple() const { return std::make_tuple( changeType, changeFloor, state ); }
    static constexpr serialization::TypeSignature type() {
        return serialization::TypeSignature::ElevatorState;
    }


    ChangeType changeType;
    int changeFloor;
    ElevatorState state;
};

struct GlobalState {
    void update( ElevatorState state ) {
        _elevators[ state.id ] = state;
        updateButtons();
    }

    void updateButtons() {
        _upButtons.reset();
        _downButtons.reset();
        for ( const auto &el : _elevators ) {
            _upButtons |= el.second.upButtons;
            _downButtons |= el.second.downButtons;
        }
    }

    FloorSet upButtons() const { return _upButtons; }
    FloorSet downButtons() const { return _downButtons; }
    const std::unordered_map< int, ElevatorState > &elevators() const {
        return _elevators;
    }

    void assertConsistency( const BasicDriverInfo &bi ) const {
        assert( _upButtons.consistent( bi ), "consistency check failed" );
        assert( _upButtons.consistent( bi ), "consistency check failed" );
        for ( auto &e : _elevators )
            e.second.assertConsistency( bi );
    }

  private:
    FloorSet _upButtons;
    FloorSet _downButtons;
    std::unordered_map< int, ElevatorState > _elevators;
};

}

#endif // SRC_STATE_H
