#include <elevator/test.h>
#include <elevator/serialization.h>

#ifndef SRC_COMMAND_H
#define SRC_COMMAND_H

namespace elevator {

enum class CommandType {
    Empty,
    CallToFloorAndGoUp,
    CallToFloorAndGoDown,
    ParkAndExit
};

struct Command {
    CommandType commandType;
    int targetElevatorId;
    int targetFloor;

    Command() :
        commandType( CommandType::Empty ), targetElevatorId( INT_MIN ), targetFloor( INT_MIN )
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
