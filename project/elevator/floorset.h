#include <elevator/driver.h>
#include <cstdint>
#include <atomic>
#include <tuple>

/* Simple abstraction over set of floors
 * requires elevator driver to detect minimal and maximal foor
 * uses user defined indices (those that are on hardware), not zero based
 * can handle up to 64 floors
 *
 * The base class is templated to allow atomic sets, see bottom of source
 *
 * The type is serializable, but does not provide type tag,
 * so it can be serialized only as part of tagged type
 */

#ifndef SRC_FLOOR_SET_H
#define SRC_FLOOR_SET_H

namespace elevator {

template< typename Base >
struct _FloorSet {
    _FloorSet() : _floors( 0 ) { }
    explicit _FloorSet( std::tuple< uint64_t > t ) : _floors( std::get< 0 >( t ) ) { }
    std::tuple< uint64_t > tuple() const { return std::make_tuple( _floors ); }

    bool get( int floor, const Driver &d ) const {
        _checkBounds( floor, d );
        return _floors & (1ul << (floor - d.minFloor()));
    }

    void set( bool value, int floor, const Driver &d ) {
        _checkBounds( floor, d );
        if ( value )
            _floors |= 1ul << (floor - d.minFloor());
        else
            _floors &= ~(1ul << (floor - d.minFloor()));
    }

    bool anyHigher( int floor, const Driver &d ) const {
        _checkBounds( floor, d );
        return _floors & ( ~((1ul << (floor - d.minFloor())) - 1) << 1);
    }

    bool anyLower( int floor, const Driver &d ) const {
        _checkBounds( floor, d );
        return _floors & ((1ul << (floor - d.minFloor())) - 1);
    }

    bool consistent( const Driver &d ) const {
        return !anyHigher( d.maxFloor(), d );
    }

    void reset() { _floors = 0; }

    bool hasAny() const { return _floors; }

    static bool hasAdditional( _FloorSet a, _FloorSet b ) {
        // calculate buttons which were pressed between a and b
        // xor means buttons which changed state, and filters only those
        // pressed now
        return (a._floors ^ b._floors) & b._floors;
    }

    _FloorSet operator|=( _FloorSet o ) {
        return _FloorSet( _floors |= o._floors );
    }

  private:
    static void _checkBounds( int floor, const Driver &d ) {
        assert_leq( d.minFloor(), floor, "out-of-bounds floor (minimun)" );
        assert_leq( floor, d.maxFloor(), "out-of-bounds floor (maximum)" );
    }
    Base _floors;

    explicit _FloorSet( uint64_t val ) : _floors( val ) { }

    static_assert( std::is_same< uint64_t, Base >::value
            || std::is_same< std::atomic< uint64_t >, Base >::value,
            "Invalid base type, valid is only uint64_t and atomic uint64_t" );
};

using FloorSet = _FloorSet< uint64_t >;
using AtomicFloorSet = _FloorSet< std::atomic< uint64_t > >;

}

#endif // SRC_FLOOR_SET_H
