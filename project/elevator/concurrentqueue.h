// C++11    (c) 2014 Vladimír Štill <xstill@fi.muni.cz>

#include <mutex>
#include <deque>
#include <condition_variable>

#include <elevator/time.h>
#include <wibble/maybe.h>

#ifndef SRC_CONCURRENT_QUEUE_H
#define SRC_CONCURRENT_QUEUE_H

namespace elevator {

/* Concurrent queue
 * so far just make sure it is consisten, no performance optimizations
 * but interface should be general enought to replace queue with faster one
 */

template< typename T >
struct ConcurrentQueue {

    void enqueue( const T &data ) {
        Guard g{ _lock };
        _queue.push_back( data );
        _cond.notify_one();
    }

    /** get and pop head of queue, this will block if queue is empty
     */
    T dequeue() {
        Guard g{ _lock };
        // wait for queue to become non-empty
        _cond.wait( g, [&]() { return !_queue.empty(); } );
        T data = _queue.front();
        _queue.pop_front();
        return data;
    }

    /** get and pop head of queue, this will block for up to given number
     * of milliseconds, and it nothing arrives return nothing
     */
    wibble::Maybe< T > timeoutDequeue( long ms ) {
        Guard g{ _lock };
        // wait for queue to become non-empty
        if ( _cond.wait_for( g, toSystemTime( ms ),
                [&]() { return !_queue.empty(); } ) )
        {
            T data = _queue.front();
            _queue.pop_front();
            return wibble::Maybe< T >::Just( data );
        } else {
            return wibble::Maybe< T >::Nothing();
        }
    }

    /** Try getting head of queue, or nothing if it is empty
     */
    wibble::Maybe< T > tryDequeue() {
        Guard g{ _lock };
        if ( _queue.empty() ) {
            return wibble::Maybe< T >::Nothing();
        } else {
            auto it = wibble::Maybe< T >::Just( _queue.front() );
            _queue.pop_front();
            return it;
        }
    }

    /** not safe -- the fact that empty returns false does not guearantee
     * dequeue will not block
     */
    bool empty() {
        Guard g{ _lock };
        return _queue.empty();
    }

  private:
    std::mutex _lock;
    std::deque< T > _queue;
    std::condition_variable _cond;
    using Guard = std::unique_lock< std::mutex >;
};

}

#endif // SRC_CONCURRENT_QUEUE_H
