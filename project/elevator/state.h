#include <unordered_map>
#include <tuple>

#include <elevator/driver.h>
#include <elevator/serialization.h>
#include <elevator/floorset.h>

#ifndef SRC_STATE_H
#define SRC_STATE_H

namespace elevator {

enum class ChangeType {
    NoType,
    KeepAlive,
    ButtonDownPressed,
    ButtonUpPressed,
    GoingToServeDown,
    GoingToServeUp,
    ServedDown,
    ServedUp
};

struct State {

    using Tuple = std::tuple< int, Direction, bool, bool, FloorSet, FloorSet, FloorSet >;

    State( Tuple tuple ) :
        lastFloor( std::get< 0 >( tuple ) ),
        lastDirection( std::get< 1 >( tuple ) ),
        stoped( std::get< 2 >( tuple ) ),
        doorOpen( std::get< 3 >( tuple ) ),
        insideButtons( std::get< 4 >( tuple ) ),
        upButtons( std::get< 5 >( tuple ) ),
        downButtons( std::get< 6 >( tuple ) )
    { }
    State() = default;

    Tuple tuple() const {
        return std::make_tuple( lastFloor, lastDirection, stoped, doorOpen,
                insideButtons, upButtons, downButtons );
    }

    int lastFloor;
    Direction lastDirection;
    bool stoped;
    bool doorOpen;
    FloorSet insideButtons;
    FloorSet upButtons;
    FloorSet downButtons;
};

struct RemoteElevatorState : State {
    int id;
    long timestamp;
};

struct StateChange {
    using Tuple = std::tuple< ChangeType, int, State >;

    StateChange() = default;
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
    State state;
};

struct GlobalState {
    void update( RemoteElevatorState state ) {
        _elevators[ state.id ] = state;
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
    std::unordered_map< int, RemoteElevatorState > elevators() const {
        return _elevators;
    }

  private:
    FloorSet _upButtons;
    FloorSet _downButtons;
    std::unordered_map< int, RemoteElevatorState > _elevators;
};

}

#endif // SRC_STATE_H
