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

void sendToPeers( std::atomic< int > *initPhase ) {
    Socket snd_sock{ commSend, true };
    snd_sock.enableBroadcast();
    while( *initPhase < 2 ) {
          Packet pack;
          switch ( *initPhase ) {
                case 0: {
                    pack = Serializer::toPacket( Initial() );
                    break; }
                case 1: {
                    pack = Serializer::toPacket( Ready() );
                    break; }
          }
          pack.address() = commBroadcast;
          bool sent = snd_sock.sendPacket( pack );
          assert( sent, "send failed" );
          std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );
    }
}

void  findPeersListener( std::atomic< int > *initPhase, int count ) {
    Socket rd_sock{ commRcv, true };
    std::set< IPv4Address > barrier;

    while ( *initPhase < 2 ) {
        Packet pack = rd_sock.recvPacketWithTimeout( 300 );
        if ( pack.size() != 0 ) {
            switch ( Serializer::packetType( pack ) ) {
                case TypeSignature::InitialPacket:{
                    peers.insert( pack.address().ip() );
                    break;}
                case TypeSignature::ElevatorReady: {
                    peers.insert( pack.address().ip() ); // it might still be that we dont know
                    barrier.insert( pack.address().ip() );
                    break;}
                case TypeSignature::RecoveryState: {
                    _needRecovery = true;
                    auto maybeRecovered = Serializer::fromPacket< RecoveryState >( pack );
                    assert( !maybeRecovered.isNothing(), "error deserializing Recovery" );
                    RecoveryState z = maybeRecovered.value();
                    _state.update( state );
                    break;}
            }
            if ( int( barrier.size() ) == count )
                *initPhase = 2;
            else if ( int( peers.size() ) == count )
                *initPhase = 1;
        }
    }
}

void SessionManager::connect( int count ) {
    // first continue sending Initial messages untill we have count peers
    
    std::atomic< int > initPhase{ 0 };
    std::thread findPeers( &Main::sendToPeers, this, &initPhase );
    findPeersListener( &initPhase );
    findPeers.join();

    auto localAddrs = IPv4Address::getMachineAddresses();
    int i = 0;
    
    // the set is sorted (guaranteed by C++) and same on all elevators
    for ( auto addr : peers ) {
        if ( localAddrs.find( addr ) != localAddrs.end() ) {
            id = i;
            break;
        }
        ++i;
    }
    assert_leq( 0, id, "Could not assing ID" );
    std::cout << "detected id " << id << std::endl;

    // if RecoveryState is received use it for initialization, save it
    // and save recovery flag

    // when we have count peers start sending Ready message untill we have
    // count peers in ready set

    // if Ready is received add to ready set

    // now start monitoring thread
    _thr = std::thread( restartWrapper( &SessionManager::_loop ), this );
}

void SessionManager::_loop() {
    Socket rd_sock{ commRcv, true };

    while ( true ) {
        Packet pack = rd_sock.recvPacketWithTimeout( 300 );
        if ( pack.size() != 0 ) {
        }
    }
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
