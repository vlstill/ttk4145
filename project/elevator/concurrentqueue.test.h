// C++11    (c) 2014 Vladimír Štill <xstill@fi.muni.cz>

#include <thread>
#include <vector>
#include <atomic>

#include <elevator/concurrentqueue.h>
#include <elevator/test.h>

using namespace elevator;

struct TestConcurrentQueue {
    Test sequential() {
        ConcurrentQueue< int > q;
        for ( int i = 0; i < 100; ++i )
            q.enqueue( i );
        for ( int i = 0; i < 100; ++i )
            assert_eq( q.dequeue(), i, "invalid data" );
        assert( q.empty(), "should be empty" );
    }

    Test tryDequeue() {
        ConcurrentQueue< int > q;
        for ( int i = 0; i < 100; ++i )
            q.enqueue( i );
        for ( int i = 0; i < 100; ++i ) {
            auto x = q.tryDequeue();
            assert( !x.isNothing(), "should not be empty" );
            assert_eq( x.value(), i, "invalid data" );
        }
        assert( q.empty(), "should be empty" );
    }

    Test timeoutDequeue() {
        ConcurrentQueue< int > q;
        for ( int i = 0; i < 100; ++i )
            q.enqueue( i );
        for ( int i = 0; i < 100; ++i ) {
            auto x = q.timeoutDequeue( 1 );
            assert( !x.isNothing(), "should not be empty" );
            assert_eq( x.value(), i, "invalid data" );
        }
        assert( q.empty(), "should be empty" );
    }

    struct Writer {
        int tid;
        ConcurrentQueue< std::pair< int, int > > &queue;

        Writer( int tid, ConcurrentQueue< std::pair< int, int > > &queue ) :
            tid( tid ), queue( queue )
        { }

        void operator()() {
            for ( int i = 0; i < 1000 * 1000; ++i )
                queue.enqueue( std::make_pair( tid, i ) );
            queue.enqueue( std::make_pair( tid, -1 ) );
        }
    };

    struct Reader {
        int tid;
        ConcurrentQueue< std::pair< int, int > > &queue;
        std::vector< int > last;
        std::atomic< int > &end;

        Reader( int tid, int writers, std::atomic< int > &end, ConcurrentQueue< std::pair< int, int > > &queue ) :
            tid( tid ), queue( queue ), last( writers ), end( end )
        { }

        void operator()() {
            for ( auto &x : last )
                x = -1;
            for ( int i = 0; end < int( last.size() ); ++i ) {
                auto x = queue.dequeue();
                assert_leq( -1, x.first, "" );
                assert_leq( x.first, int( last.size() ), "" );
                assert_leq( -1, x.second, "" );
                if ( x.second == -1 ) {
                    ++end;
                    continue;
                }
                assert_leq( last[ x.first ] + 1, x.second, "" );
            }
            queue.enqueue( std::make_pair( -1, -1 ) ); // just to prevent stuck other readers
        }
    };

    Test parallel() {
        ConcurrentQueue< std::pair< int, int > > q;
        std::thread w1{ Writer{ 0, q } };
        std::thread w2{ Writer{ 1, q } };
        std::atomic< int > end{ 0 };
        std::thread r1{ Reader{ 0, 2, end, q } };
        std::thread r2{ Reader{ 1, 2, end, q } };
        w1.join(); w2.join();
        r1.join(); r2.join();
    }
};
