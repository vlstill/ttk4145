#include <elevator/sessionmanager.h>
#include <elevator/serialization.h>
#include <elevator/restartwrapper.h>

namespace elevator {

using namespace serialization;

const udp::Address commSend{ udp::IPv4Address::any, udp::Port{ 64032 } };
const udp::Address commRcv{ udp::IPv4Address::any, udp::Port{ 64033 } };
const udp::Address commBroadcast{ udp::IPv4Address::broadcast, udp::Port{ 64033 } };

struct Initial {
    Initial() = default;
    Initial( std::tuple<> ) { };
    std::tuple<> tuple() const { return std::tuple<>(); }
    static TypeSignature type() {
        return TypeSignature::InitialPacket;
    }
};

struct Ready {
    Ready() = default;
    Ready( std::tuple<> ) { };
    std::tuple<> tuple() const { return std::tuple<>(); }
    static TypeSignature type() {
        return TypeSignature::ElevatorReady;
    }
};

struct RecoveryState {
    RecoveryState() = default;
    explicit RecoveryState( ElevatorState state ) : state( state ) { }
    RecoveryState( std::tuple< ElevatorState > t )
        : RecoveryState( std::get< 0 >( t ) )
    { }
    static TypeSignature type() { return TypeSignature::RecoveryState; }
    std::tuple< ElevatorState > tuple() const { return std::make_tuple( state ); }

    ElevatorState state;
};

void SessionManager::connect( int count ) {
    // first continue sending Initial messages untill we have count peers

    // if RecoveryState is received use it for initialization, save it
    // and save recovery flag

    // when we have count peers start sending Ready message untill we have
    // count peers in ready set

    // if Ready is received add to ready set

    // now start monitoring thread
    _thr = std::thread( restartWrapper( &SessionManager::_loop ), this );
}

void SessionManager::_loop() {
}

void bla() {
    auto test = Serializer::toPacket( Initial() );
    switch ( Serializer::packetType( test ) ) {
        case TypeSignature::InitialPacket: {
            auto maybeInitial = Serializer::fromPacket< Initial >( test );
            assert( !maybeInitial.isNothing(), "error deserializing Initial" );
            Initial x = maybeInitial.value();
            break; }
        case TypeSignature::ElevatorReady: {
           break; }
    }
}

}
