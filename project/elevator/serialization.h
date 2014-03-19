// C++11    (c) 2014 Vladimír Štill <xstill@fi.muni.cz>

/* serialization support for network
 */

#include <tuple>
#include <type_traits>
#include <memory>
#include <utility>
#include <wibble/maybe.h>

#include <elevator/test.h>
#include <elevator/udptools.h>
#include <elevator/internal/serialization.h>

#ifndef SRC_SERIALIZATION_H
#define SRC_SERIALIZATION_H

namespace serialization {

enum class TypeSignature {
    NoType,
    TestType,

    ElevatorCommand,
    ElevatorState,

    InitialPacket,
    ElevatorReady,
    RecoveryState,
    RecoveryPeers
};

template< typename T >
constexpr size_t sizeOf() {
    return std::is_empty< T >::value ? 0 : sizeof( T );
}

template< typename T >
struct Serializable :
    _internal::Serializable< T, _internal::trait< T >() >
{ };

struct Serialized {
    long size() const { return _datasize; }
    TypeSignature type() const { return _datatype; }
    const char *rawData() const { return _data.get(); }
    char *rawDataRW() { return _data.get(); }

    Serialized( TypeSignature type, long size ) :
        _data( new char[ size ] ), _datasize( size ), _datatype( type )
    { }

    Serialized( TypeSignature type, const char *data, long size ) :
        Serialized( type, size )
    {
        std::copy( data, data + size, _data.get() );
    }

    Serialized() :
        _data( nullptr ), _datasize( 0 ), _datatype( TypeSignature::NoType )
    { }

  private:
    std::unique_ptr< char[] > _data;
    long _datasize;
    TypeSignature _datatype;
};

struct Serializer {
    template< typename What >
    static Serialized serialize( const What &w ) {
        Serialized serial{ w.type(), Serializable< What >::size( w ) };
        char *ptr = serial.rawDataRW();
        Serializable< What >::serialize( w, &ptr );
        assert_eq( ptr, serial.rawData() + Serializable< What >::size( w ), "wrong size" );
        return serial;
    }

    template< typename What >
    static wibble::Maybe< What > deserialize( Serialized &s ) {
        if ( What::type() != s.type() )
            return wibble::Maybe< What >::Nothing();
        const char *ptr = s.rawData();
        auto data = Serializable< What >::deserialize( &ptr );
        assert_eq( s.size(), Serializable< What >::size( data ), "wrong size" );
        return wibble::Maybe< What >::Just( data );
    }

    template< typename What >
    static What unsafeDeserialize( Serialized &s ) {
        auto result = deserialize< What >( s );
        assert( !result.isNothing(), "Wrong type for deserialization" );
        return result.value();
    }

    template< typename What >
    static udp::Packet toPacket( const What &w ) {
        Serialized serial = serialize( w );
        udp::Packet packet{ packetSize( serial ) };
        packet.get< TypeSignature >() = serial.type();
        packet.get< int >( sizeof( TypeSignature ) ) = serial.size();
        std::copy( serial.rawData(), serial.rawData() + serial.size(),
                packet.data() + packet_data_offset );
        return packet;
    }

    static TypeSignature packetType( const udp::Packet &packet ) {
        return packet.get< TypeSignature >();
    }

    template< typename What >
    static wibble::Maybe< What > fromPacket( const udp::Packet &packet ) {
        Serialized serial{ packet.get< TypeSignature >(), packet.cdata() + packet_data_offset,
            packet.get< int >( sizeof( TypeSignature ) ) };
        return deserialize< What >( serial );
    }

    template< typename What >
    static What unsafeFromPacket( const udp::Packet &packet ) {
        auto result = fromPacket< What >( packet );
        assert( !result.isNothing(), "Wrong type for deserialization" );
        return result.value();
    }

  private:
    static constexpr int packet_data_offset = sizeof( TypeSignature ) + sizeof( int );
    static int packetSize( const Serialized& serial ) {
        return packet_data_offset + serial.size();
    }
};

}

#endif // SRC_SERIALIZATION_H
