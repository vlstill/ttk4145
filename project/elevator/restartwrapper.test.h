#include <elevator/restartwrapper.h>
#include <elevator/test.h>

using namespace elevator;

static void sf( int ) { }

struct TestRestartWrapper {
    void f( int ) { }

    Test function() {
        restartWrapper( sf )( 1 );
    }

    Test method() {
        restartWrapper( &TestRestartWrapper::f )( *this, 1 );
    }

    Test methodWithPtr() {
        restartWrapper( &TestRestartWrapper::f )( this, 1 );
    }
};
