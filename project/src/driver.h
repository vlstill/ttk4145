// C++11    (c) 2014 Vladimír Štill <xstill@fi.muni.cz>

/** the higher level API for libcomedy device of elevator */

#include "io.h"

#ifndef SRC_DRIVER_H
#define SRC_DRIVER_H

namespace elevator {

enum class ButtonType {
    CallUp = 0,
    CallDown = 1,
    TargetFloor = 2
};

struct Button {
    Button() = default;
    Button( ButtonType type, int floor ) : _type( type ), _floor( floor ) { }

    ButtonType type() { return _type; }
    int floor() { return _floor; }

  private:
    ButtonType _type;
    int _floor;
};

struct Driver {

    Driver();

    void setButtonLamp( Button button, bool state );
    void setStopLamp( bool state );
    void setDoorOpenLamp( bool state );
    void setFloorIndicator( int floor );


  private:
    int _minFloor;
    int _maxFloor;
    lowlevel::IO _lio;
};

}

#endif // SRC_DRIVER_H
