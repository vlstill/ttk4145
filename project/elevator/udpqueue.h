#include <elevator/udptools.h>
#include <elevator/concurrentqueue.h>
#include <elevator/restartwrapper.h>
#include <elevator/serialization.h>
#include <thread>

#ifndef ELEVATOR_UDP_QUEUE_H
#define ELEVATOR_UDP_QUEUE_H

namespace elevator {

template< typename T >
struct QueueSender {
    QueueSender( udp::Address bindAddr, udp::Address sendAddr, ConcurrentQueue< T > &queue ) :
        _sock( bindAddr, true ), _sendAddr( sendAddr ), _queue( queue )
    {
        _sock.enableBroadcast();
    }
    void run() {
        _thr = std::thread( restartWrapper( &QueueSender::_runLocal ), this );
    }

  private:
    void _runLocal();
    udp::Socket _sock;
    udp::Address _sendAddr;
    ConcurrentQueue< T > &_queue;
    std::thread _thr;
};

template< typename T >
struct QueueReceiver {
    QueueReceiver( udp::Address bindAddr, ConcurrentQueue< T > &queue ) :
        _sock( bindAddr, true ), _queue( queue )
    {
        _sock.enableBroadcast();
    }

    QueueReceiver( udp::Address bindAddr, ConcurrentQueue< T > &queue,
            std::function< bool( T & ) > predicate ) :
        _sock( bindAddr, true ), _queue( queue ), _pred( predicate )
    {
        _sock.enableBroadcast();
    }
    void run() {
        _thr = std::thread( restartWrapper( &QueueReceiver::_runLocal ), this );
    }

  private:
    void _runLocal();
    udp::Socket _sock;
    ConcurrentQueue< T > &_queue;
    std::thread _thr;
    std::function< bool( T & ) > _pred;
};

template< typename T >
void QueueSender< T >::_runLocal() {
    while ( true ) {
        auto x = _queue.dequeue();
        auto pack = serialization::Serializer::toPacket( x );
        pack.address() = _sendAddr;
        _sock.sendPacket( pack );
    }
}

template< typename T >
void QueueReceiver< T >::_runLocal() {
    while ( true ) {
        auto pack = _sock.recvPacket();
        if ( pack.address().ip() == _sock.localAddress().ip() )
            continue; // ignore local feedback
        auto mx = serialization::Serializer::fromPacket< T >( pack );
        assert( !mx.isNothing(), "Received invalid message" );
        if ( !_pred || _pred( mx.value() ) )
            _queue.enqueue( mx.value() );
    }
}

}

#endif // ELEVATOR_UDP_QUEUE_H
