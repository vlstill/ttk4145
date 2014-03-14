#include <chrono>
#include <cstdint>

#ifndef SRC_TIME_H
#define SRC_TIME_H

namespace elevator {

using MillisecondTime = int64_t;

static inline MillisecondTime now() {
    return std::chrono::duration_cast< std::chrono::milliseconds >(
            std::chrono::steady_clock::now().time_since_epoch() ).count();
}

static inline std::chrono::milliseconds toSystemTime( MillisecondTime mtime ) {
    return std::chrono::milliseconds( mtime );
}

template< class Rep, class Period >
static MillisecondTime fromSystemTime( const std::chrono::duration< Rep, Period >& d ) {
    return std::chrono::duration_cast< std::chrono::milliseconds >( d ).count();
}

}

#endif // SRC_TIME_H
