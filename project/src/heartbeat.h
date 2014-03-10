#include <chrono>
#include <cstdint>
#include <atomic>
#include <exception>
#include <string>

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

}

#endif // SRC_HEARTBEAT_H
