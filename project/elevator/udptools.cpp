// C++11 (c) 2014 Vladimír Štill

#include <elevator/udptools.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ifaddrs.h>

namespace udp {

const IPv4Address IPv4Address::any{ 0, 0, 0, 0 };
const IPv4Address IPv4Address::localhost{ 127, 0, 0, 1 };
const IPv4Address IPv4Address::broadcast{ 255, 255, 255, 255 };

sockaddr_in getNetAddress( Address addr ) {
    sockaddr_in netAddr;
    memset( &netAddr, 0, sizeof( sockaddr_in ) );
    netAddr.sin_family = AF_INET;
    netAddr.sin_addr.s_addr = htonl( addr.ip().asInt() );
    netAddr.sin_port = htons( addr.port().get() );
    return netAddr;
}

Address fromNetAddress( sockaddr_in netAddr ) {
    return Address{
        IPv4Address{ ntohl( netAddr.sin_addr.s_addr ) },
        Port{ ntohs( netAddr.sin_port ) }
    };
}

std::set< IPv4Address > IPv4Address::getMachineAddresses() {
    std::set< IPv4Address > addresses;
    struct ifaddrs *interfaces;
    int rc = getifaddrs( &interfaces );
    assert_eq( rc, 0, "getifaddrs failed" );
    for ( struct ifaddrs *addr = interfaces; addr != NULL; addr = addr->ifa_next ) {
        if ( addr->ifa_addr && addr->ifa_addr->sa_family == AF_INET )
            addresses.insert( fromNetAddress( *reinterpret_cast< sockaddr_in * >( addr->ifa_addr ) ).ip() );
    }
    return addresses;
}

struct Socket::_Data {
    _Data( Address local, bool reuseAddr, int rcvbufsize ) :
        localAddress( local ), rcvbufsize( rcvbufsize ), rcvbuf( new char[ rcvbufsize ] )
    {
        fd = socket(AF_INET, SOCK_DGRAM, 0);
        assert( fd > 0, "Cannot create socket" );
        sockaddr_in locAddrNet = getNetAddress( localAddress );
        if ( reuseAddr ) {
            int yes = 1;
            int rc = setsockopt( fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof( int ) );
            assert_neq( rc, -1, "setting socket reuse address option failed" );
        }
        int bnd = bind( fd, reinterpret_cast< sockaddr * >( &locAddrNet ), sizeof( sockaddr_in ) );
        assert( bnd==0, "Bind failed" );
    }

    Address localAddress;
    int rcvbufsize;
    std::unique_ptr< char[] > rcvbuf;

    int fd;
};


Socket::Socket( Address local, bool reuseAddr, int rcvbuf ) : _data( new _Data( local, reuseAddr, rcvbuf ) ) { }

// must be here -- where _Data definition is known
Socket::~Socket() = default;

void Socket::setRecvBufferSize( int size ) {
    assert_leq( 1, size, "invalid size" );
    _data->rcvbufsize = size;
    _data->rcvbuf.reset( new char[ size ] );
}

struct GuardTimout {
    GuardTimout( int fd, long msTimeout ) : fd( fd ) {
        struct timeval tv;
        tv.tv_sec = msTimeout / 1000;
        tv.tv_usec = (msTimeout % 1000) * 1000;

        setsockopt( fd, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast< char * >( &tv ), sizeof( struct timeval ) );
    }
    ~GuardTimout() {
        struct timeval tv;
        tv.tv_sec = tv.tv_usec = 0;

        setsockopt( fd, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast< char * >( &tv ), sizeof( struct timeval ) );
    }

  private:
    int fd;
};


bool Socket::sendPacket( Packet &packet ) {
    sockaddr_in remote = getNetAddress( packet.address() );
    int snd = sendto( _data->fd, packet.data(), packet.size(), 0,
                reinterpret_cast< struct sockaddr * >( &remote ), sizeof( sockaddr_in ) );
    return snd == packet.size();
}

Packet Socket::recvPacket() {
    sockaddr_in remote;
    socklen_t remlen = sizeof( sockaddr_in );

    int rc = recvfrom( _data->fd, _data->rcvbuf.get(), _data->rcvbufsize,
            0, reinterpret_cast< struct sockaddr * >( &remote ), &remlen );
    assert_leq( remlen, sizeof( sockaddr_in ), "Invalid address returned" );
    return rc > 0
        ? Packet( _data->rcvbuf.get(), rc, fromNetAddress( remote ) )
        : Packet();
}

Packet Socket::recvPacketWithTimeout( long ms ) {
    GuardTimout timeout{ _data->fd, ms }; /* sets timeout and resets it when
                                             of exit of this scope (on return) */
    return recvPacket();
}

Address Socket::localAddress() const { return _data->localAddress; }

void setBroadcast( int broadcastPermission, int sock ) {
    assert( broadcastPermission == 0 || broadcastPermission == 1, "invalid option" );
    int rc = setsockopt( sock, SOL_SOCKET, SO_BROADCAST,
            reinterpret_cast< void * >( &broadcastPermission ),
            sizeof( broadcastPermission ) );
    assert_eq( 0, rc, "setsockopt failed" );
}

void Socket::enableBroadcast() {
    setBroadcast( 1, _data->fd );
}

void Socket::disableBroadcast() {
    setBroadcast( 0, _data->fd );
}

}
