// C++11 (c) 2014 Vladimír Štill

#include "udptools.h"
#include <wibble/test.h>
#include <thread>
#include <unistd.h>

struct TestUdp {
    Test basic() {
        udp::Socket sock{};
    }
    
     Test send() {
		udp::Address target{ udp::IPv4Address{ { 127, 0, 0, 1 } }, udp::Port{ 0 } };
        udp::Packet packet{"Test",5};
		packet.address() = target;
        udp::Socket sock{};
        bool sndpck = sock.sendPacket(packet);
        assert(sndpck);
	}

	Test sendRcv() {
		std::thread sender( []() {
				sleep( 1 );
				udp::Address target{ udp::IPv4Address{ { 127, 0, 0, 1 } }, udp::Port{ 64123 } };
       			udp::Packet packet{"Test",5};
				packet.address() = target;
        		udp::Socket sock{};
		        sock.sendPacket(packet);				
			} );
		
		udp::Socket recv{ udp::Address{ udp::IPv4Address{ 0 }, udp::Port{ 64123 } } };
		recv.recvPacket();
		sender.join();
	}
};
