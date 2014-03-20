// C++11    (c) 2014 Vladimír Štill <xstill@fi.muni.cz>

/** the middle level API for elevator */

#include <climits>
#include <tuple>
#include <cstdint>
#include <elevator/io.h>

#ifndef SRC_DRIVER_H
#define SRC_DRIVER_H

namespace elevator {

enum class ButtonType {
    CallUp = 0,
    CallDown = 1,
    TargetFloor = 2
};

struct Button {
    using Tuple = std::tuple< int, int >;
    Button() : _type( ButtonType::CallUp ), _floor( INT_MIN ) { }
    Button( ButtonType type, int floor ) : _type( type ), _floor( floor ) { }
    Button( Tuple tuple ) : _type( ButtonType( std::get< 0 >( tuple ) ) ), _floor( std::get< 1 >( tuple ) ) { }

    Tuple tuple() const { return std::make_tuple( int( _type ), _floor ); }

    ButtonType type() { return _type; }
    int floor() { return _floor; }

  private:
    ButtonType _type;
    int _floor;
};

enum class Direction { None, Up, Down };

struct BasicDriverInfo {
    BasicDriverInfo( int min, int max ) : _minFloor( min ), _maxFloor( max ) { }
    int minFloor() const { return _minFloor; }
    int maxFloor() const { return _maxFloor; }
  protected:
    const int _minFloor;
    const int _maxFloor;
};

struct Driver : BasicDriverInfo {

    Driver();
    ~Driver();

    /* initialize the elevator -- disable all lights and run to lowest floor */
    void init();
    void shutdown();

    void setButtonLamp( Button button, bool state );
    void setStopLamp( bool state );
    void setDoorOpenLamp( bool state );
    void setFloorIndicator( int floor );

    bool getButtonLamp( Button button );
    bool getStopLamp();
    bool getDoorOpenLamp();
    int getFloorIndicator();
    bool getButtonSignal( Button button );

    void stopElevator();
    int getFloor();
    bool getStop();
    bool getObstruction();

    int minFloor() const { return _minFloor; }
    int maxFloor() const { return _maxFloor; }

    void setMotorSpeed( Direction, int );


  private:
    Direction _lastDirection;
    lowlevel::IO _lio;
};

}

#endif // SRC_DRIVER_H
