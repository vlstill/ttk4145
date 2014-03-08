#include "state.h"
#include "test.h"
#include "serialization.h"
#include "udptools.h"

struct TestState {

    Test serialize() {
        elevator::State st;
        udp::Packet pck = serialization::Serializer::toPacket( st );
        wibble::Maybe< elevator::State > st2 = serialization::Serializer::fromPacket< elevator::State >( pck );
    }

};
