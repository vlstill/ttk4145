#include <vector>
#include <tuple>

#include "driver.h"
#include "serialization.h"

#ifndef SRC_STATE_H
#define SRC_STATE_H

namespace elevator {

struct State {

    using Tuple = std::tuple< int, int, bool, bool, bool, std::vector< Button > >;

    State( Tuple tuple ) :
        _lastFloor( std::get< 0 >( tuple ) ),
        _lastDirection( Direction( std::get< 1 >( tuple ) ) ),
        _moving( std::get< 2 >( tuple ) ),
        _stopLight( std::get< 3 >( tuple ) ),
        _doorOpenLight( std::get< 4 >( tuple ) ),
        _buttonLights( std::get< 5 >( tuple ) )
    { }
    State() = default;

    Tuple tuple() const {
        return std::make_tuple( _lastFloor, int( _lastDirection ),
                _moving, _stopLight, _doorOpenLight, _buttonLights );
    }

    static constexpr serialization::TypeSignature type() {
        return serialization::TypeSignature::ElevatorState;
    }

  private:
    int _lastFloor;
    Direction _lastDirection;
    bool _moving;
    bool _stopLight;
    bool _doorOpenLight;
    std::vector< Button > _buttonLights;
};

}

#endif // SRC_STATE_H
