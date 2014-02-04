#include <mutex>
#include <deque>
#include <condition_variable>

#include <wibble/maybe.h>

#ifndef SRC_CONCURRENT_QUEUE_H
#define SRC_CONCURRENT_QUEUE_H

/* Concurrent queue
 * so far just make sure it is consisten, no performance optimizations
 * but interface should be general enought to replace queue with faster one
 */

template< typename T >
struct ConcurrentQueue {

    void enqueue( const T &data ) {
        Guard g{ _lock };
        _queue.push_back( data );
        g.unlock();
        _cond.notify_one();
    }

    /** get and pop head of queue, this will block if queue is empty
     */
    T dequeue() {
        Guard g{ _lock };
        while ( _queue.empty() )
            _cond.wait( g );
        T data = _queue.front();
        _queue.pop_front();
        return data;
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

    /** not safe
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

#endif // SRC_CONCURRENT_QUEUE_H
