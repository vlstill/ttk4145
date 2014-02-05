// C++11    (c) 2014 Vladimír Štill <xstill@fi.muni.cz>

#include <cstdlib>
#include <iostream>
#include <sstream>

#ifndef SRC_TEST_H
#define SRC_TEST_H

#define UNIT_TEST_STRUCT struct
using Test = void;

#define CODE_LOCATION test::Location( __FILE__, __LINE__, __func__ )
#define assert( X, MSG ) test::assert_fn( !!(X), #X " (" MSG ")", CODE_LOCATION )
#define assert_eq( X, Y, MSG ) test::assert_eq_fn( X, Y, #X " == " #Y " (" MSG ")", CODE_LOCATION )
#define assert_leq( X, Y, MSG ) test::assert_leq_fn( X, Y, #X " <= " #Y " (" MSG ")", CODE_LOCATION )

namespace test {

struct Location {

    Location( const char *file, int line, const char *func ) :
        _file( file ), _line( line ), _func( func )
    { }

    friend std::ostream &operator<<( std::ostream &os, Location loc ) {
        os << loc._file << ":" << loc._line << " (" << loc._func << ")";
        return os;
    }

  private:
    const char *_file;
    const int _line;
    const char *_func;
};

struct AssertionFailed : std::exception {

    AssertionFailed( const char *what, Location loc ) :
        _what( what ), _where( loc )
    { }

    std::string message() const noexcept {
        std::stringstream ss;
        ss << "Assertion failed: " << _what << " on " << _where << ".";
        return ss.str();
    }

    const char *what() const noexcept override {
        auto str = message();
        char *msg = new char[ str.size() + 1 ];
        std::copy( str.begin(), str.end(), msg );
        return msg;
    }

  private:
    const char *_what;
    const Location _where;
};

static inline void assert_fn( bool x, const char *what, Location loc ) {
    if ( !x )
        throw AssertionFailed( what, loc );
}

template< typename X, typename Y >
static inline void assert_eq_fn( const X &x, const Y &y, const char *what, Location loc ) {
    if ( !( x == y ) )
        throw AssertionFailed( what, loc );
}

template< typename X, typename Y >
static inline void assert_leq_fn( const X &x, const Y &y, const char *what, Location loc ) {
    if ( !( x <= y ) )
        throw AssertionFailed( what, loc );
}

}

#endif // SRC_TEST_H
