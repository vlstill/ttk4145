#include "state.h"
#include "test.h"
#include "serialization.h"
#include "udptools.h"

struct TestState {

    Test serialize() {
        elevator::StateChange st;
        udp::Packet pck = serialization::Serializer::toPacket( st );
        auto st2 = serialization::Serializer::fromPacket< elevator::StateChange >( pck );
    }

};
