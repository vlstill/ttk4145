#include "concurrentqueue.h"
#include "test.h"

#include <thread>

struct TestConcurrentQueue {
    Test sequential() {
        ConcurrentQueue< int > q;
        for ( int i = 0; i < 100; ++i )
            q.enqueue( i );
        for ( int i = 0; i < 100; ++i )
            assert_eq( q.dequeue(), i, "Out of order element" );
        assert( q.empty(), "Extra element" );
    }
/*
    struct Writer {
        int tid;
        ConcurrentQueue< std::pair< int, int > > &queue;

        Write( int tid, ConcurrentQueue< std::pair< int, int > > &queue ) :
            tid( tid ), queue( queue )
        { }

        void operator()() {
            for ( int i = 0; i < 1000 * 1000 * 1000; ++i )
                queue.enqueue( std::make_pair( tid, i ) );
        }
    };

    struct Reader {
        int tid;
        ConcurrentQueue< std::pair< int, int > > &queue;

        Reader( int tid, ConcurrentQueue< std::pair< int, int > > &queue ) :
            tid( tid ), queue( queue )
        { }

        void operator()() {
            for ( int i = 0; i < 1000 * 1000 * 1000; ++i )
                queue.enqueue( std::make_pair( tid, i ) );
        }
    };

    void parallel() {
        ConcurrentQueue< std::pair< int, int > > q;
        std::thread w1{ Writer{ 0, q } };
        std::thread w2{ Writer{ 1, q } };
        std::thread r1{ Reader{ 0, q } };
        std::thread r2{ Reader{ 1, q } };
        w1.join(); w2.join();
        r1.join(); r2.join();
    }
    */
};
