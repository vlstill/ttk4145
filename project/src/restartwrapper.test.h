#include <src/restartwrapper.h>
#include <src/test.h>

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
