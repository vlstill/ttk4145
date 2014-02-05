#include "driver.h"
#include <wibble/test.h>

using namespace elevator;

struct TestDriver {
    Test init() {
        Driver driver;
    }

    Test lights() {
        Driver driver;
        driver.setStopLamp( true );
    }
};
