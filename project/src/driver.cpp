#include <array>

#include "test.h"
#include "driver.h"
#include "channels.h"

namespace elevator {

static const int N_FLOORS = 4;
static const int N_BUTTONS = 3;

static const std::array< std::array< int, N_BUTTONS >, N_FLOORS > lampChannelMatrix{ {
    { { LIGHT_UP1, LIGHT_DOWN1, LIGHT_COMMAND1 } },
    { { LIGHT_UP2, LIGHT_DOWN2, LIGHT_COMMAND2 } },
    { { LIGHT_UP3, LIGHT_DOWN3, LIGHT_COMMAND3 } },
    { { LIGHT_UP4, LIGHT_DOWN4, LIGHT_COMMAND4 } },
} };

static const std::array< std::array< int, N_BUTTONS >, N_FLOORS > buttonChannelMatrix{ {
    { { FLOOR_UP1, FLOOR_DOWN1, FLOOR_COMMAND1 } },
    { { FLOOR_UP2, FLOOR_DOWN2, FLOOR_COMMAND2 } },
    { { FLOOR_UP3, FLOOR_DOWN3, FLOOR_COMMAND3 } },
    { { FLOOR_UP4, FLOOR_DOWN4, FLOOR_COMMAND4 } },
} };

int lamp( Button btn ) {
    assert_leq( btn.floor(), int( lampChannelMatrix.size() ), "out-of-bounds" );
    assert_leq( 1, btn.floor(), "out-of-bounds" );

    return lampChannelMatrix[ btn.floor() - 1 ][ int( btn.type() ) ];
}

int button( Button btn ) {
    assert_leq( btn.floor(), int( buttonChannelMatrix.size() ), "out-of-bounds" );
    assert_leq( 1, btn.floor(), "out-of-bounds" );

    return buttonChannelMatrix[ btn.floor() - 1 ][ int( btn.type() ) ];
}

Driver::Driver() : _minFloor( 1 ), _maxFloor( 4 ) {
    for ( int i = _minFloor; i <= _maxFloor; ++i ) {
        if ( i != _maxFloor )
            setButtonLamp( Button{ ButtonType::CallUp, i }, false );
        if ( i != _minFloor )
            setButtonLamp( Button{ ButtonType::CallDown, i }, false );
        setButtonLamp( Button{ ButtonType::TargetFloor, i }, false );
    }

    setStopLamp( false );
    setDoorOpenLamp( false );
    setFloorIndicator( _minFloor );
}

void Driver::setButtonLamp( Button btn, bool state ) {
    if ( state )
        _lio.io_set_bit( lamp( btn ) );
    else
        _lio.io_clear_bit( lamp( btn ) );
}

void Driver::setStopLamp( bool state ) {
    if ( state )
        _lio.io_set_bit( LIGHT_STOP );
    else
        _lio.io_clear_bit( LIGHT_STOP );
}

void Driver::setDoorOpenLamp( bool state ) {
    if ( state )
        _lio.io_set_bit( DOOR_OPEN );
    else
        _lio.io_clear_bit( DOOR_OPEN );

}

void Driver::setFloorIndicator( int floor ) {
    assert_leq( _minFloor, floor, "floor out of bounds" );
    assert_leq( floor, _maxFloor, "floor out of bounds" );
    --floor; // floor numers are from 0 in backend but from 1 in driver and labels

    if (floor & 0x02)
        _lio.io_set_bit(FLOOR_IND1);
    else
        _lio.io_clear_bit(FLOOR_IND1);

    if (floor & 0x01)
        _lio.io_set_bit(FLOOR_IND2);
    else
        _lio.io_clear_bit(FLOOR_IND2);
}

}
