#include <climits>
#include <unordered_map>
#include <tuple>
#include <mutex>

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

    GoingToServeUp,
    GoingToServeDown,

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
        direction( Direction::None ), stopped( false ), doorOpen( true )
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

}

#endif // SRC_STATE_H
