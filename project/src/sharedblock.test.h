// C++11    (c) 2014 Vladimír Štill <xstill@fi.muni.cz>

#include "sharedblock.h"
#include "test.h"

struct TestSharedBlock {
    Test simple() {
        SharedBlock< int > block{ 42 };
        assert_eq( block.snapshot(), 42, "snapshot" );

    }
};
