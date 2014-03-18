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

SessionManager::SessionManager( GlobalState &glo ) : _state( glo ),
    _sendSock{ commSend, true }, _recvSock{ commRcv, true }
{ }

void SessionManager::_initSender( std::atomic< int > *initPhase ) {
    udp::Socket _sendSock{ commSend, true };
    _sendSock.enableBroadcast();
    while( *initPhase < 2 ) {
          udp::Packet pack;
          switch ( *initPhase ) {
                case 0: {
                    pack = Serializer::toPacket( Initial() );
                    break; }
                case 1: {
                    pack = Serializer::toPacket( Ready() );
                    break; }
          }
          pack.address() = commBroadcast;
          bool sent = _sendSock.sendPacket( pack );
          assert( sent, "send failed" );
          std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );
    }
}

void SessionManager::_initListener( std::atomic< int > *initPhase, int count ) {
    udp::Socket _recvSock{ commRcv, true };
    std::set< udp::IPv4Address > barrier;

    while ( *initPhase < 2 ) {
        udp::Packet pack = _recvSock.recvPacketWithTimeout( 300 );
        if ( pack.size() != 0 ) {
            switch ( Serializer::packetType( pack ) ) {
                case TypeSignature::InitialPacket: {
                    _peers.insert( pack.address().ip() );
                    break; }
                case TypeSignature::ElevatorReady: {
                    _peers.insert( pack.address().ip() ); // it might still be that we dont know
                    barrier.insert( pack.address().ip() );
                    break; }
                case TypeSignature::RecoveryState: {
                    _needRecovery = true;
                    auto maybeRecovered = Serializer::fromPacket< RecoveryState >( pack );
                    assert( !maybeRecovered.isNothing(), "error deserializing Recovery" );
                    RecoveryState recovered = maybeRecovered.value();
                    _state.update( recovered.state );
                    break; }
                default:
                    std::cerr << "Unknown packet received on service channel" << std::endl;
            }
            if ( int( barrier.size() ) == count )
                *initPhase = 2;
            else if ( int( _peers.size() ) == count )
                *initPhase = 1;
        }
    }
}

void SessionManager::connect( int count ) {
    // first continue sending Initial messages untill we have count peers

    std::atomic< int > initPhase{ 0 };
    std::thread findPeers( &SessionManager::_initSender, this, &initPhase );
    _initListener( &initPhase, count );
    findPeers.join();

    auto localAddrs = udp::IPv4Address::getMachineAddresses();
    int i = 0;

    // the set is sorted (guaranteed by C++) and therefore same on all elevators
    for ( auto addr : _peers ) {
        if ( localAddrs.find( addr ) != localAddrs.end() ) {
            _id = i;
            break;
        }
        ++i;
    }
    assert_leq( 0, _id, "Could not assing ID" );
    std::cout << "detected id " << _id << std::endl;

    // if RecoveryState is received use it for initialization, save it
    // and save recovery flag

    // when we have count peers start sending Ready message untill we have
    // count peers in ready set

    // if Ready is received add to ready set

    // now start monitoring thread
    _thr = std::thread( restartWrapper( &SessionManager::_loop ), this );
}

void SessionManager::_loop() {
    udp::Socket _recvSock{ commRcv, true };

    while ( true ) {
        udp::Packet pack = _recvSock.recvPacketWithTimeout( 300 );
        if ( pack.size() != 0 && Serializer::packetType( pack ) == TypeSignature::InitialPacket ) {
            int i = 0;
            bool found = false;
            for ( auto addr : _peers ) {
                if ( addr == pack.address().ip() ) {
                    found = true;
                    break;
                }
                ++i;
            }
            if ( !found ) {
                /* NOTE: we can't add unknown elevator, it would change IDs and make
                 * mess in recovery mechanisms */
                std::cerr << "WARNING: Attempt to socialization from unknown IP "
                          << pack.address().ip() << ". It will be ignored." << std::endl;
                continue;
            }

            std::cerr << "NOTICE: Sending recovery to elevator " << i << ", ("
                      << pack.address().ip() << ")" << std::endl;
            udp::Packet recovery = Serializer::toPacket( RecoveryState( _state.get( i ) ) );
            recovery.address() = pack.address();
            _sendSock.sendPacket( recovery );
        }
    }
}

}
