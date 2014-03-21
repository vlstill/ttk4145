#include <elevator/driver.h>
#include <elevator/test.h>
#include <unistd.h>
#include <vector>
#include <random>
#include <sched.h>

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

    Test buttons() {
#ifdef O_INTERACTIVE_UNIT_TESTS
        Driver driver;
        driver.init();

        std::vector< Button > buttons;
        for ( int i = 0; i < 4; ++i ) {
            buttons.push_back( Button( ButtonType::TargetFloor, i + 1 ) );
            if ( i < 3 )
                buttons.push_back( Button( ButtonType::CallUp, i + 1 ) );
            if ( i != 0 )
                buttons.push_back( Button( ButtonType::CallDown, i + 1 ) );
        }

        auto getButton = [&]() -> Button {
            for ( ;; )
                for ( auto b : buttons )
                    if ( driver.getButtonSignal( b ) )
                        return b;
        };

        for ( auto b : buttons )
            driver.setButtonLamp( b, false );

        std::cout << "Keep pressing lighted buttons" << std::endl;
        std::random_device rd;
        std::uniform_int_distribution< int > dist( 0, 9 );
        for ( int i = 0; i < 32; ++i ) {
            SLEEP;
            Button b = buttons[ dist( rd ) ];
            driver.setButtonLamp( b, true );
            assert( driver.getButtonLamp( b ), "lamp failed" );
            std::cout << "(" << int( b.type() ) << ", " << b.floor() << ")" << " ---> " << std::flush;
            Button b2 = getButton();
            std::cout << "(" << int( b2.type() ) << ", " << b2.floor() << ")" << std::endl;
            assert_eq( int( b.type() ), int( b2.type() ), "You pressed wrong button or there is error in code (bad type)" );
            assert_eq( b.floor(), b2.floor(), "You pressed wrong button or there is error in code (bad floor)" );
            driver.setButtonLamp( b, false );
        }
#endif
    }
};
