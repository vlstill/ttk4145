// C++11 (c) 2014 Vladimír Štill

#include "udptools.h"

namespace udp {

struct Socket::_Data {
    _Data( Address local, int rcvbufsize ) :
        localAddress( local ), rcvbufsize( rcvbufsize ), rcvbuf( new char[ rcvbufsize ] )
    { }

    Address localAddress;
    int rcvbufsize;
    std::unique_ptr< char > rcvbuf;

    // ... OS specific socket stuff
};


Socket::Socket( Address local, int rcvbuf ) : _data( new _Data( local, rcvbuf ) ) { }

// must be here -- where _Data definition is known
Socket::~Socket() = default;

void Socket::setRecvBufferSize( int size ) {
    // ...
}


bool Socket::sendPacket( Packet packet ) {
    // ...
}

Packet Socket::recvPacket() {
    // ...
}

Address Socket::localAddress() const { return _data->localAddress; }

}
