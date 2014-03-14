#include <elevator/heartbeat.h>
#include <elevator/test.h>
#include <unistd.h>

using namespace elevator;

struct TestHeartBeat {
    Test inTime() {
        HeartBeat hb( 500 );
        hb.beat();
        hb.throwIfLate();
    }

    Test late() {
        HeartBeat hb( 1 );
        hb.beat();
        usleep( 100 * 1000 );
        try {
            hb.throwIfLate();
            assert( false, "should have thrown" );
        } catch ( HeartBeatException & )
        { }
    }
};
