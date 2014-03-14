// C++11    (c) 2014 Vladimír Štill <xstill@fi.muni.cz>

#include <elevator/sharedblock.h>
#include <elevator/test.h>

struct TestSharedBlock {
    Test simple() {
        SharedBlock< int > block{ 42 };
        assert_eq( block.snapshot(), 42, "snapshot" );
    }

    Test modify() {
        SharedBlock< int > block{ 0 };
        assert_eq( block.atomically( []( int &data ) -> int {
                assert_eq( data , 0, "original state" );
                data = 42;
                return data;
            } ), 42, "atomically" );
        assert_eq( block.snapshot(), 42, "new state" );
    }
};
