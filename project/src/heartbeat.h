#include <src/test.h>
#include <thread>
#include <chrono>
#include <cstdint>
#include <atomic>
#include <exception>
#include <string>
#include <deque>

#ifndef SRC_HEARTBEAT_H
#define SRC_HEARTBEAT_H

namespace elevator {

struct HeartBeatException : std::exception {
    int64_t delta;
    int64_t threshold;

    HeartBeatException( int64_t delta, int64_t threshold ) :
        delta( delta ), threshold( threshold )
    { }

    const char *what() const noexcept override {
        std::string str = "Heart-beat missed, real delta is "
            + std::to_string( delta ) + " ms, threshold is "
            + std::to_string( threshold ) + " ms.";
        char *msg = new char[ str.size() + 1 ];
        msg[ str.size() ] = 0;
        std::copy( str.begin(), str.end(), msg );
        return msg;
    }
};

/* heart-beat helper with millisecond precision
 * you have to make sure first beat will happen before first check */
struct HeartBeat {

    HeartBeat( int64_t threshold ) : _threshold( threshold ) { }
    HeartBeat( const HeartBeat & ) = delete; // disable copying

    static int64_t now() {
        return std::chrono::duration_cast< std::chrono::milliseconds >(
                std::chrono::steady_clock::now().time_since_epoch() ).count();
    }

    void beat() {
        _lastBeat.store( now(), std::memory_order_release );
    }

    bool check() const {
        return _delta() <= _threshold;
    }

    void throwIfLate() const {
        int delta = _delta();
        if ( delta > _threshold )
            throw HeartBeatException( delta, _threshold );
    }

  private:
    std::atomic< int64_t > _lastBeat;
    const int64_t _threshold;

    int64_t _delta() const {
        int64_t lastBeat = _lastBeat.load( std::memory_order_acquire );
        return now() - lastBeat;
    }
};

struct HeartBeatManager {
    HeartBeatManager() : terminate( false ), rerunTime( 10000 ) { }
    ~HeartBeatManager() {
        if ( thr.joinable() ) {
            terminate = true;
            thr.join();
        }
    }

    /* creates new HeartBeat objects and sets rerun time
     * to at least 10 times shorter period that is threshold
     * of this beat
     */
    HeartBeat &getNew( int64_t threshold ) {
        int64_t thisRerun = threshold / 10;
        if ( rerunTime > thisRerun )
            rerunTime = thisRerun;
        beats.emplace_back( threshold );
        return beats.back();
    }

    /* Runs thread running checking loop in background,
     * this function can be run at most once
     * thread will stop when HeartBeatManager is destructed
     * this loop runs every rerunTime milliseconds (+ scheduler delay)
     * and checks validity of all heartbeats
     * it heartbeat fails it will throw HeartBeatException which will
     * terminate program if uncought
     */
    void run() {
        assert( !thr.joinable(), "attempt to re-run HeartBeatManager's check thread" );
        thr = std::thread( &HeartBeatManager::runInThisThread, this );
    }

    void runInThisThread() {
        while ( !terminate.load( std::memory_order::memory_order_relaxed ) ) {
            auto next = std::chrono::steady_clock::now() + std::chrono::milliseconds( rerunTime );

            for ( auto &b : beats )
                b.throwIfLate();

            std::this_thread::sleep_until( next );
        }
    }

  private:
    std::deque< HeartBeat > beats;
    std::thread thr;
    std::atomic< bool > terminate;
    int64_t rerunTime;
};

}

#endif // SRC_HEARTBEAT_H
