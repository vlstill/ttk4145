#include <elevator/state.h>
#include <elevator/test.h>
#include <elevator/serialization.h>
#include <elevator/udptools.h>

struct TestState {

    Test serialize() {
        elevator::StateChange st;
        udp::Packet pck = serialization::Serializer::toPacket( st );
        auto st2 = serialization::Serializer::fromPacket< elevator::StateChange >( pck );
    }

};
