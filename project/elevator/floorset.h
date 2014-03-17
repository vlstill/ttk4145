#include <elevator/driver.h>
#include <cstdint>
#include <atomic>
#include <tuple>

/* Simple abstraction over set of floors
 * requires elevator driver to detect minimal and maximal foor
 * uses user defined indices (those that are on hardware), not zero based
 * can handle up to 64 floors
 *
 * The type is serializable, but does not provide type tag,
 * so it can be serialized only as part of tagged type
 */

#ifndef SRC_FLOOR_SET_H
#define SRC_FLOOR_SET_H

namespace elevator {

struct FloorSet {
    FloorSet() : _floors( 0 ) { }
    explicit FloorSet( std::tuple< uint64_t > t ) : _floors( std::get< 0 >( t ) ) { }
    std::tuple< uint64_t > tuple() const { return std::make_tuple( _floors ); }

    bool get( int floor, const BasicDriverInfo &d ) const {
        _checkBounds( floor, d );
        return _floors & (1ul << (floor - d.minFloor()));
    }

    bool set( bool value, int floor, const BasicDriverInfo &d ) {
        bool orig = get( floor, d );
        if ( value )
            _floors |= 1ul << (floor - d.minFloor());
        else
            _floors &= ~(1ul << (floor - d.minFloor()));
        return orig;
    }

    bool anyHigher( int floor, const BasicDriverInfo &d ) const {
        _checkBounds( floor, d );
        return _floors & ( ~((1ul << (floor - d.minFloor())) - 1) << 1);
    }

    bool anyLower( int floor, const BasicDriverInfo &d ) const {
        _checkBounds( floor, d );
        return _floors & ((1ul << (floor - d.minFloor())) - 1);
    }

    bool anyOther( int floor, const BasicDriverInfo &d ) const {
        _checkBounds( floor, d );
        return _floors & ~(1ul << (floor - d.minFloor()));
    }

    bool consistent( const BasicDriverInfo &d ) const {
        return !anyHigher( d.maxFloor(), d );
    }

    void reset() { _floors = 0; }

    bool hasAny() const { return _floors; }

    static bool hasAdditional( FloorSet a, FloorSet b ) {
        // calculate buttons which were pressed between a and b
        // xor means buttons which changed state, and filters only those
        // pressed now
        return (a._floors ^ b._floors) & b._floors;
    }

    FloorSet operator|=( FloorSet o ) {
        return FloorSet( _floors |= o._floors );
    }

    friend FloorSet operator|( FloorSet a, FloorSet b ) {
        return FloorSet( a._floors | b._floors );
    }

    friend bool operator==( FloorSet a, FloorSet b ) {
        return a._floors == b._floors;
    }

    friend bool operator!=( FloorSet a, FloorSet b ) {
        return a._floors != b._floors;
    }

  private:
    static void _checkBounds( int floor, const BasicDriverInfo &d ) {
        assert_leq( d.minFloor(), floor, "out-of-bounds floor (minimun)" );
        assert_leq( floor, d.maxFloor(), "out-of-bounds floor (maximum)" );
    }
    uint64_t _floors;

    explicit FloorSet( uint64_t val ) : _floors( val ) { }
};

}

#endif // SRC_FLOOR_SET_H
