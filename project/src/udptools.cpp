// C++11 (c) 2014 Vladimír Štill

#include "udptools.h"
#include<iostream>
#include<arpa/inet.h>
#include<unistd.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<stdio.h>
#include<string.h>
#include<stdlib.h>

namespace udp {

struct Socket::_Data {
    _Data( Address local, int rcvbufsize ) :
        localAddress( local ), rcvbufsize( rcvbufsize ), rcvbuf( new char[ rcvbufsize ] )
    {
          struct sockaddr_in myaddr;
          fd = socket(AF_INET, SOCK_DGRAM, 0);
          assert(fd>0, "Cannot create socket\n");
          myaddr.sin_family = AF_INET;
          myaddr.sin_addr.s_addr = htonl(localAddress.ip().asInt()); // IP Address
          myaddr.sin_port = htons(localAddress.port().get()); // Port numbrt
          int bnd = bind(fd, (struct sockaddr *)&myaddr, sizeof(myaddr));
          assert(bnd==0, "Bind failed");
 	}

    Address localAddress;
    int rcvbufsize;
    std::unique_ptr< char[] > rcvbuf;
    // ... OS specific socket stuff
    int fd;
};


Socket::Socket( Address local, int rcvbuf ) : _data( new _Data( local, rcvbuf ) ) { }

// must be here -- where _Data definition is known
Socket::~Socket() = default;

void Socket::setRecvBufferSize( int size ) {
    // ...
    
    assert_leq( 1, size, "invalid size" );
    _data->rcvbufsize = size;
    _data->rcvbuf.reset( new char[ size ] );
}


bool Socket::sendPacket( Packet &packet ) {
    // ...
    struct sockaddr_in remote;
    remote.sin_family = AF_INET;
    remote.sin_addr.s_addr = htonl(packet.address().ip().asInt()); // IP Address
    remote.sin_port = htons(packet.address().port().get()); // Port numbrt
    socklen_t l = sizeof(remote);
    int snd = sendto(_data->fd,packet.data(),packet.size(),0,(struct sockaddr *)&remote,l);
    return (packet.size()>0);
    
}

Packet Socket::recvPacket() {
    // ...
    struct sockaddr_in remote;
    socklen_t m = sizeof(remote);
    int rc = recvfrom(_data->fd,_data->rcvbuf.get(),_data->rcvbufsize,0,(struct sockaddr *)&remote,&m);
    assert(rc>=0, "Receive failed");
    return Packet(_data->rcvbuf.get(), rc);
}

Address Socket::localAddress() const { return _data->localAddress; }

}
