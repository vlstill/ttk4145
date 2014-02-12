#include "driver.h"
#include "test.h"
#include <unistd.h>

using namespace elevator;

#define SLEEP usleep( 500 * 1000 )

struct TestDriver {
    Test construct() {
        Driver driver;
    }


    Test lights() {
        Driver driver;
        driver.setStopLamp( true );
        assert( driver.getStopLamp(), "driver feedback failure" );
        SLEEP;
        driver.setStopLamp( false );

        driver.setDoorOpenLamp( true );
        assert( driver.getDoorOpenLamp(), "driver feedback failure" );
        SLEEP;
        driver.setDoorOpenLamp( false );

        for ( int i = 1; i <= 4; ++i ) {
            driver.setFloorIndicator( i );
            SLEEP;
        }

        // we insert local function here which can access the driver
        auto _testButtonLamp = [&]( const Button btn ) {
            driver.setButtonLamp( btn, true );
            assert( driver.getButtonLamp( btn ), "driver feedback failure" );
            SLEEP;
            driver.setButtonLamp( btn, false );
        };

        for ( int i = 1; i < 4; ++i )
            _testButtonLamp( Button{ ButtonType::CallUp, i } );

        for ( int i = 2; i <= 4; ++i )
            _testButtonLamp( Button{ ButtonType::CallDown, i } );

        for ( int i = 1; i <= 4; ++i )
            _testButtonLamp( Button{ ButtonType::TargetFloor, i } );
    }

    Test init() {
        Driver driver;
        driver.init();
    }

    Test shutdown() {
        Driver driver;
        driver.shutdown();
    }

    Test upDown() {
        Driver driver;
        driver.init();
        driver.goToTop();
        driver.goToBottom();
        driver.shutdown();
    }

    Test moveAround() {
        Driver driver;
        driver.init();

        _move( driver );
    }

    Test moveAroundNoInit() {
        Driver driver;
        _move( driver );
    }

    void _move( Driver &driver ) {
        auto move = [&]( int floor ) {
            std::cout << "Moving to floor " << floor << "..." << std::flush;
            driver.goToFloor( floor );
            assert_eq( driver.getFloor(), floor, "Wrong floor" );
            std::cout << "OK" << std::endl;
        };

        move( 2 );
        move( 3 );
        move( 2 );
        move( 4 );
        move( 2 );
        move( 4 );
        move( 3 );
        move( 1 );
    }
};
