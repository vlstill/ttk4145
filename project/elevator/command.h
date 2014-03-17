#include <elevator/test.h>
#include <elevator/serialization.h>

#ifndef SRC_COMMAND_H
#define SRC_COMMAND_H

namespace elevator {

enum class CommandType {
    Empty,
    CallToFloorAndGoUp,
    CallToFloorAndGoDown,

    TurnOnLightUp,
    TurnOffLightUp,
    TurnOnLightDown,
    TurnOffLightDown,
};

struct Command {
    static const int NO_ID = INT_MIN;
    static const int ANY_ID = INT_MAX;

    CommandType commandType;
    int targetElevatorId;
    int targetFloor;

    Command() :
        commandType( CommandType::Empty ), targetElevatorId( NO_ID ), targetFloor( NO_ID )
    { }
    Command( CommandType type, int targetElevId, int floor ) :
        commandType( type ), targetElevatorId( targetElevId ), targetFloor( floor )
    { }

    // serialization
    explicit Command( std::tuple< CommandType, int, int > tuple ) :
        Command( std::get< 0 >( tuple ), std::get< 1 >( tuple ), std::get< 2 >( tuple ) )
    { }
    static serialization::TypeSignature type() {
        return serialization::TypeSignature::ElevatorCommand;
    }
    std::tuple< CommandType, int, int > tuple() const {
        return std::make_tuple( commandType, targetElevatorId, targetFloor );
    }
};

}

#endif // SRC_COMMAND_H
