// C++11    (c) 2014 Vladimír Štill <xstill@fi.muni.cz>

/** Beware, magic tend to live in this file.
 * Those are low-level implementation details of serialization framework
 * they should not be used directly at any cost.
 */


#include <type_traits>
#include <vector>
#include <tuple>

#include <wibble/sfinae.h>

#include <elevator/test.h>

#ifndef SRC_INTERNAL_SERIALIZATION_H
#define SRC_INTERNAL_SERIALIZATION_H

namespace serialization {
namespace _internal {

enum Trait {
    Other = 0,
    Fundamental,
    Enum,
    Tuple,
    Container,
    TupleSerializable // user defined type with tuple conversions
};

template< typename... X >
Trait declcheck( X ... ) { assert_unreachable( "compile time only" ); }

template< typename T > constexpr Trait trait();

/* type withc is tuple convertible, that is it has method tuple and the
 * it can be constructed from its tuple encoding
 */
template< typename T >
constexpr auto trait_5( wibble::Preferred ) ->
    decltype( declcheck( T( std::declval< T >().tuple() ),
                std::tuple_size< decltype( std::declval< T >().tuple() ) >::value ) )
{
    return Trait::TupleSerializable;
}
template< typename T >
constexpr auto trait_5( wibble::NotPreferred ) -> Trait {
    return Trait::Other;
}

template< typename T >
constexpr auto trait_4( wibble::Preferred ) ->
    decltype( declcheck( typename T::value_type(), // nested typedef value_type
                T( std::declval< typename T::value_type * >(), std::declval< typename T::value_type * >() ), // constructible from rage
                std::declval< T >().begin(), std::declval< T >().end(), // iterators
                std::declval< T >().size() ) )
{
    return trait< typename T::value_type >() == Trait::Other
        ? Trait::Other
        : Trait::Container;
}

template< typename T >
constexpr auto trait_4( wibble::NotPreferred ) -> Trait {
    return trait_5< T >( wibble::Preferred() );
}

template< typename T >
constexpr auto trait_3( wibble::Preferred ) -> typename
    std::enable_if< std::is_enum< T >::value, Trait >::type
{ return Trait::Enum; }

template< typename T >
constexpr auto trait_3( wibble::NotPreferred ) -> Trait {
    return trait_4< T >( wibble::Preferred() );
}

template< typename T >
constexpr auto trait_2( wibble::Preferred ) -> typename
    std::enable_if< std::is_fundamental< T >::value, Trait >::type
{ return Trait::Fundamental; }

template< typename T >
constexpr auto trait_2( wibble::NotPreferred ) -> Trait {
    return trait_3< T >( wibble::Preferred() );
}

template< typename T >
constexpr auto trait_1( wibble::Preferred ) ->
    decltype( declcheck( std::tuple_size< T >::value ) )
{ return Trait::Tuple; }

template< typename T >
constexpr auto trait_1( wibble::NotPreferred ) -> Trait {
    return trait_2< T >( wibble::Preferred() );
}

template< typename T >
constexpr Trait trait() { return trait_1< T >( wibble::Preferred() ); }

template< typename T, Trait tr >
struct SerializableImpl { }; // for Trait::Other, static_assert will fail anyway

template< typename T, Trait tr >
struct Serializable : SerializableImpl< T, tr > {
    static_assert( tr != Trait::Other, "Trying to serialize type which is neither "
            "fundamental type (numerical types) nor collection or tuple of serializable types" );
    using Base = SerializableImpl< T, tr >;

    using Base::deserialize;
    static T deserialize( char **from ) {
        return Base::deserialize( const_cast< const char ** >( from ) );
    }
};

template< typename T >
struct SerializableImpl< T, Trait::Fundamental > {
    static long size( T ) { return sizeof( T ); }

    static void serialize( T source, char **to ) {
        *reinterpret_cast< T * >( *to ) = source;
        *to += sizeof( T );
    }

    static T deserialize( const char **from ) {
        T t = *reinterpret_cast< const T * >( *from );
        *from += sizeof( T );
        return t;
    }
};

template< typename T >
struct SerializableImpl< T, Trait::Container > {
    using ValueType = typename T::value_type;
    using ValueSerializable = Serializable< ValueType, trait< ValueType >() >;

    static long size( const T &container ) {
        long s = sizeof( long ); // constant overhead for storing size of container
        for ( const ValueType &val : container )
            s += ValueSerializable::size( val );
        return s;
    }

    static void serialize( const T &source, char **to ) {
        *reinterpret_cast< long * >( *to ) = source.size();
        *to += sizeof( long );
        for ( const ValueType &val : source )
            ValueSerializable::serialize( val, to );
    }

    static T deserialize( const char **from ) {
        const long count = *reinterpret_cast< const long * >( *from );
        *from += sizeof( long );
        std::vector< ValueType > tempstore;
        for ( long i = 0; i < count; ++i )
            tempstore.push_back( ValueSerializable::deserialize( from ) );
        return T( tempstore.begin(), tempstore.end() );
    }
};

template< typename T >
struct SerializableImpl< T, Trait::Tuple > {
    template< long i >
    using NestedType = typename std::tuple_element< i, T >::type;
    template< long i >
    using NestedSerializable = Serializable< NestedType< i >, trait< NestedType< i > >() >;
    static constexpr long tuple_size = std::tuple_size< T >::value;

    static long size( const T &tuple ) { return _size< 0 >( 0, tuple ); }

    static void serialize( const T &source, char **to ) {
        _serialize< 0 >( source, to );
    }

    static T deserialize( const char **from ) {
        return _deserialize< 0 >( from );
    }

  private:
    template< long i >
    static auto _size( long accum, const T &tuple ) -> typename
        std::enable_if< i != tuple_size, long >::type
    {
        return _size< i + 1 >( accum + NestedSerializable< i >::size( std::get< i >( tuple ) ), tuple );
    }
    template< long i >
    static auto _size( long accum, const T& ) -> typename
        std::enable_if< i == tuple_size, long >::type
    {
        return accum;
    }

    template< long i >
    static auto _serialize( const T &tuple, char **to ) -> typename
        std::enable_if< i != tuple_size >::type
    {
        NestedSerializable< i >::serialize( std::get< i >( tuple ), to );
        _serialize< i + 1 >( tuple, to );
    }
    template< long i >
    static auto _serialize( const T &, char ** ) -> typename
        std::enable_if< i == tuple_size >::type
    { }

    template< long i, typename... Args >
    static auto _deserialize( const char **from, Args &&...args ) -> typename
        std::enable_if< i != tuple_size, T >::type
    {
        NestedType< i > elem = NestedSerializable< i >::deserialize( from );
        return _deserialize< i + 1 >( from, std::forward< Args >( args )...,
                std::forward< NestedType< i > >( elem ) );
    }
    template< long i, typename... Args >
    static auto _deserialize( const char **, Args &&...args ) -> typename
        std::enable_if< i == tuple_size, T >::type
    {
        return T{ std::forward< Args >( args )... };
    }
};

template< typename T >
struct SerializableImpl< T, Trait::TupleSerializable > {
    using TupleType = decltype( std::declval< T >().tuple() );
    using TupleSerializable = Serializable< TupleType, trait< TupleType >() >;

    static long size( const T &value ) {
        return TupleSerializable::size( value.tuple() );
    }

    static void serialize( const T &source, char **to ) {
        TupleSerializable::serialize( source.tuple(), to );
    }

    static T deserialize( const char **from ) {
        return T( TupleSerializable::deserialize( from ) );
    }
};

template< typename T >
struct SerializableImpl< T, Trait::Enum > {
    using BaseType = typename std::underlying_type< T >::type;
    using BaseSerializable = Serializable< BaseType, trait< BaseType >() >;

    static long size( const T &value ) {
        return BaseSerializable::size( BaseType( value ) );
    }

    static void serialize( const T &source, char **to ) {
        BaseSerializable::serialize( BaseType( source ), to );
    }

    static T deserialize( const char **from ) {
        return T( BaseSerializable::deserialize( from ) );
    }
};

}
}

#endif // SRC_INTERNAL_SERIALIZATION_H
