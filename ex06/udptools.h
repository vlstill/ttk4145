// C++11    (c) 2014 Vladimír Štill <xstill@fi.muni.cz>

#include <cstdint>
#include <array>
#include <memory>
#include <tuple>

#include "test.h"

#ifndef SRC_UDP_TOOLS_H
#define SRC_UDP_TOOLS_H

namespace udp {

/** structure for representing IPv4 address */
struct IPv4Address {

    /** create address -- explicit to avoid implicit cast from int */
    explicit IPv4Address( uint32_t addr ) : _addr( addr ) { }
    explicit IPv4Address( std::array< uint8_t, 4 > arr ) :
        _addr( (arr[ 0 ] << 24) | (arr[ 1 ] << 16) | (arr[ 2 ] << 8) | arr[ 3 ] )
    { }
    IPv4Address( uint8_t a, uint8_t b, uint8_t c, uint8_t d ) :
        _addr( (a << 24) | (b << 16) | (c << 8) | d )
    { }
    IPv4Address() = default;

    /** read address */
    uint32_t asInt() const { return _addr; }
    std::array< uint8_t, 4 > asArray() const {
        std::array< uint8_t, 4 > arr;
        arr[ 0 ] = (_addr >> 24) & 0xff;
        arr[ 1 ] = (_addr >> 16) & 0xff;
        arr[ 2 ] = (_addr >> 8 ) & 0xff;
        arr[ 3 ] = (_addr >> 0 ) & 0xff;
        return arr;
    }

    friend bool operator==( IPv4Address a, IPv4Address b ) {
        return a._addr == b._addr;
    }

    friend bool operator!=( IPv4Address a, IPv4Address b ) {
        return a._addr != b._addr;
    }

    friend std::ostream &operator<<( std::ostream &os, IPv4Address addr ) {
        auto arr = addr.asArray();
        os << int( arr[ 0 ] ) << "." << int( arr[ 1 ] ) << "."
           << int( arr[ 2 ] ) << "." << int( arr[ 3 ] );
        return os;
    }

    static const IPv4Address localhost;
    static const IPv4Address any;

  private:
    uint32_t _addr;
};

/** struct for UDP port */
struct Port {

    explicit Port( uint16_t port ) : _port( port ) { }
    Port() = default;

    uint16_t get() const { return _port; }

    friend bool operator==( Port a, Port b ) { return a._port == b._port; }
    friend bool operator!=( Port a, Port b ) { return a._port != b._port; }

    friend std::ostream &operator<<( std::ostream &os, Port port ) {
        return os << port._port;
    }

  private:
    uint16_t _port;
};

/** complete UDP address
 * it is intentionally prohibited at compile time to change
 * part of address -- this is ensured by definition of ddress itsenfA
 * and its components
 */
struct Address {

    Address( IPv4Address address, Port port ) :
        _ip( address ), _port( port )
    { }
    Address() = default;

    IPv4Address ip() const { return _ip; }
    Port port() const { return _port; }

    friend bool operator==( Address a, Address b ) {
        return a._ip == b._ip && a._port == b._port;
    }

    friend bool operator!=( Address a, Address b ) {
        return a._ip != b._ip && a._port != b._port;
    }

    friend std::ostream &operator<<( std::ostream &os, Address addr ) {
        return os << addr._ip << ":" << addr._port;
    }

  private:
    IPv4Address _ip;
    Port _port;
};

/** abstraction over UDP packet
 * packet copletely owns its data and it get dealocated when packet
 * object sease to exist
 */
struct Packet {

    Packet() = default;
    explicit Packet( int size ) : _data( new char[ size ] ), _size( size ) {
        assert_leq( 1, size, "invalid size" );
    }
    explicit Packet( const char *data, int size, Address addr = Address() ) :
        _address( addr ), _data( new char[ size ] ), _size( size )
    {
        assert_leq( 1, size, "invalid size" );
        assert( data != nullptr, "data must be given" );
        std::copy( data, data + size, _data.get() );
    }

    char *data() { return _data.get(); }
    const char *data() const { return cdata(); }
    const char *cdata() const { return _data.get(); }

    template< typename T = char >
    T &get( int position = 0 ) { return *reinterpret_cast< T * >( data() + position ); }

    template< typename T = char >
    T get( int position = 0 ) const { return cget< T >( position ); }

    template< typename T = char >
    T cget( int position = 0 ) const { return *reinterpret_cast< const T * >( cdata() + position ); }

    Address address() const { return _address; }
    Address &address() { return _address; }

    int size() const { return _size; }

    /** allocate packet of given size, drops all current data */
    void allocate( int size ) {
        assert_leq( 1, size, "invalid size" );
        _size = size;
        _data.reset( new char[ size ] );
    }

  private:
    Address _address;
    std::unique_ptr< char[] > _data;
    int _size;
};

enum { standardMTU = 1500 };

/** UDP socket (reader and writer) abstraction,
 * local address can be zero (or IP in address can be zero),
 * menaning all local addresses and arbitrary port
 */
struct Socket {
    explicit Socket( Address local = Address(), bool reuseAddr = false, int rcvbuf = standardMTU );
    ~Socket();

    void setRecvBufferSize( int size );
    /* note: we don't need send buffer, as packet is already allocated when sending */

    bool sendPacket( Packet & );
    Packet recvPacket();
    /** receive packet or timeout after given number of milliseconds
     * and return Nothing
     */
    Packet recvPacketWithTimeout( long ms );

    Address localAddress() const;

  private:
    /* separate private data to provide better abstraction and avoid
     * including messy linux headers with too much macros into our
     * .h files
     */
    struct _Data;
    std::unique_ptr< _Data > _data;
};

}

#endif // SRC_UDP_TOOLS_H
