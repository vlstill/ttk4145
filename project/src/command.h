#include <src/test.h>
#include <src/serialization.h>

#ifndef SRC_COMMAND_H
#define SRC_COMMAND_H

namespace elevator {

enum class CommandType {
    Empty,
    CallToFloorAndGoUp,
    CallToFloorAndGoDown,
    GoToFloor,
    Park,
    ParkAndExit
};

struct Command {
    Command() : _elid( INT_MIN ), _type( CommandType::Empty ), _floor( INT_MIN ) { }
    Command( int targetElevId, CommandType type, int floor ) :
        _elid( targetElevId ), _type( type ), _floor( floor )
    { }

    int targetElevatorId() const {
        assert_neq( _elid, INT_MIN, "invalid elevator id" );
        return _elid;
    }
    CommandType commandType() const { return _type; }
    int targetFloor() const {
        assert_neq( _floor, INT_MIN, "invalid floor" );
        return _floor;
    }

    // serialization
    Command( std::tuple< int, CommandType, int > tuple ) :
        Command( std::get< 0 >( tuple ), std::get< 1 >( tuple ), std::get< 2 >( tuple ) )
    { }
    static serialization::TypeSignature type() {
        return serialization::TypeSignature::ElevatorCommand;
    }
    std::tuple< int, CommandType, int > tuple() const {
        return std::make_tuple( _elid, _type, _floor );
    }
  private:
    int _elid;
    CommandType _type;
    int _floor;
};

#endif // SRC_COMMAND_H

}
