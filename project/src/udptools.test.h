// C++11 (c) 2014 Vladimír Štill

#include "udptools.h"
#include <wibble/test.h>
#include <thread>
#include <cstring>
#include <unistd.h>

struct TestUdp {
    Test basic() {
        udp::Socket sock{};
    }

    Test send() {
        udp::Address target{ udp::IPv4Address{ 127, 0, 0, 1 }, udp::Port{ 64123 } };
        udp::Packet packet{ "Test", 5 };
        packet.address() = target;
        udp::Socket sock{};
        bool sent = sock.sendPacket(packet);
        assert( sent );
    }

    Test sendRcv() {
        udp::Address sndAddr{ udp::IPv4Address{ 127, 0, 0, 1 }, udp::Port{ 64124 } };
        udp::Address target{ udp::IPv4Address{ 127, 0, 0, 1 }, udp::Port{ 64123 } };

        std::thread sender( [=]() {
                sleep( 1 );
                udp::Packet packet{"Test",5};
                packet.address() = target;
                udp::Socket sock{ sndAddr };
                sock.sendPacket(packet);
            } );

        udp::Socket recv{ target };
        udp::Packet pck = recv.recvPacket();
        assert( pck.size() );
        assert_eq( std::strcmp( pck.data(), "Test" ), 0 );
        assert_eq( pck.address(), sndAddr );
        sender.join();
    }
};
