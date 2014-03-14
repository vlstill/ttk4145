#include <elevator/test.h>
#include <elevator/time.h>
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
    MillisecondTime delta;
    MillisecondTime threshold;

    HeartBeatException( MillisecondTime delta, MillisecondTime threshold ) :
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

    HeartBeat( MillisecondTime threshold ) : _threshold( threshold ) { }
    HeartBeat( const HeartBeat & ) = delete; // disable copying

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

    MillisecondTime threshold() const { return _threshold; }

  private:
    std::atomic< MillisecondTime > _lastBeat;
    const MillisecondTime _threshold;

    MillisecondTime _delta() const {
        MillisecondTime lastBeat = _lastBeat.load( std::memory_order_acquire );
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
    HeartBeat &getNew( MillisecondTime threshold ) {
        MillisecondTime thisRerun = threshold / 10;
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
            auto next = std::chrono::steady_clock::now() + toSystemTime( rerunTime );

            for ( auto &b : beats )
                b.throwIfLate();

            std::this_thread::sleep_until( next );
        }
    }

  private:
    std::deque< HeartBeat > beats;
    std::thread thr;
    std::atomic< bool > terminate;
    MillisecondTime rerunTime;
};

}

#endif // SRC_HEARTBEAT_H
