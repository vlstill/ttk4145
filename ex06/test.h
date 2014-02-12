// C++11    (c) 2014 Vladimír Štill <xstill@fi.muni.cz>

#include <cstdlib>
#include <iostream>
#include <sstream>

#ifndef SRC_TEST_H
#define SRC_TEST_H

using Test = void;

#define CODE_LOCATION test::Location( __FILE__, __LINE__, __func__ )

#undef assert
#undef assert_eq
#undef assert_neq
#undef assert_leq
#undef assert_le
#undef assert_unimplemented
#undef assert_unreachable

#define assert( X, MSG ) test::assert_fn( !!(X), #X " (" MSG ")", CODE_LOCATION )
#define assert_eq( X, Y, MSG ) test::assert_eq_fn( X, Y, #X " == " #Y " (" MSG ")", CODE_LOCATION )
#define assert_neq( X, Y, MSG ) test::assert_neq_fn( X, Y, #X " != " #Y " (" MSG ")", CODE_LOCATION )
#define assert_leq( X, Y, MSG ) test::assert_leq_fn( X, Y, #X " <= " #Y " (" MSG ")", CODE_LOCATION )
#define assert_lt( X, Y, MSG ) test::assert_lt_fn( X, Y, #X " < " #Y " (" MSG ")", CODE_LOCATION )
#define assert_unimplemented()  test::assert_unimplemented_fn( CODE_LOCATION )
#define assert_unreachable( MSG )    test::assert_unreachable_fn( "unreachable: " MSG, CODE_LOCATION )

namespace test {

enum class AssertionTag {
    Generall,
    Unimplemented,
    Unreachable,
};

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

    AssertionFailed( const char *what, Location loc, AssertionTag tag ) :
        _what( what ), _where( loc ), _tag( tag )
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

    AssertionTag tag() const { return _tag; }

  private:
    const char *_what;
    const Location _where;
    AssertionTag _tag;
};

struct GenerallAssertionFailed : AssertionFailed {
    GenerallAssertionFailed( const char *what, Location loc ) :
        AssertionFailed( what, loc, AssertionTag::Generall )
    { }
};

struct UnimplementedAssertionFailed : AssertionFailed {
    UnimplementedAssertionFailed( const char *what, Location loc ) :
        AssertionFailed( what, loc, AssertionTag::Unimplemented )
    { }
};

struct UnreachableAssertionFailed : AssertionFailed {
    UnreachableAssertionFailed( const char *what, Location loc ) :
        AssertionFailed( what, loc, AssertionTag::Unreachable )
    { }
};

static inline void assert_fn( bool x, const char *what, Location loc ) {
    if ( !x )
        throw GenerallAssertionFailed( what, loc );
}

template< typename X, typename Y >
static inline void assert_eq_fn( const X &x, const Y &y, const char *what, Location loc ) {
    if ( !( x == y ) )
        throw GenerallAssertionFailed( what, loc );
}

template< typename X, typename Y >
static inline void assert_neq_fn( const X &x, const Y &y, const char *what, Location loc ) {
    if ( !( x != y ) )
        throw GenerallAssertionFailed( what, loc );
}

template< typename X, typename Y >
static inline void assert_leq_fn( const X &x, const Y &y, const char *what, Location loc ) {
    if ( !( x <= y ) )
        throw GenerallAssertionFailed( what, loc );
}

template< typename X, typename Y >
static inline void assert_lt_fn( const X &x, const Y &y, const char *what, Location loc ) {
    if ( !( x < y ) )
        throw GenerallAssertionFailed( what, loc );
}

[[noreturn]] static inline void assert_unimplemented_fn( Location loc ) {
    throw UnimplementedAssertionFailed( "unimplemented", loc );
}

[[noreturn]] static inline void assert_unreachable_fn( const char *what, Location loc ) {
    throw UnreachableAssertionFailed( what, loc );
}

}

namespace std {
static inline std::string to_string( test::AssertionTag at ) {
#define TO_STRING( AT ) if ( at == test::AssertionTag:: AT ) return #AT
    TO_STRING( Generall );
    TO_STRING( Unimplemented );
    TO_STRING( Unreachable );
    assert_unreachable( "Unhandled case" );
#undef TO_STRING
}

}

#endif // SRC_TEST_H
