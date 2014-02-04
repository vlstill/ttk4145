// C++11    (c) 2014 Vladimír Štill <xstill@fi.muni.cz>

/* serialization support for network
 */

#include <tuple>
#include <type_traits>
#include <memory>
#include <utility>

#include "test.h"

#ifndef SRC_SERIALIZATION_H
#define SRC_SERIALIZATION_H

namespace serialization {

enum class TypeSignature {
    NoType,
    TestType,

    Command,
    Status
};

template< typename T >
constexpr size_t sizeOf() {
    return std::is_empty< T >::value ? 0 : sizeof( T );
}

struct Serialized {
    int size() const { return _datasize; }
    TypeSignature type() const { return _datatype; }
    const char *rawData() const { return _data.get(); }

    Serialized( TypeSignature type, int size ) :
        _data( new char[ size ] ), _datasize( size ), _datatype( type )
    { }

    Serialized( TypeSignature type, char *data, int size ) :
        Serialized( type, size )
    {
        std::copy( data, data + size, _data.get() );
    }

    Serialized() :
        _data( nullptr ), _datasize( 0 ), _datatype( TypeSignature::NoType )
    { }

    struct Slice {

        Slice() = default;
        Slice( char *data, int size ) : _data( data ), _datasize( size ) { }

        template< typename T >
        std::tuple< Slice, T, bool > get( const int size = sizeOf< T >() ) {
            if ( size > _datasize )
                return std::make_tuple( Slice(), T(), false );
            T &t = *reinterpret_cast< T * >( _data );
            return std::make_tuple( advance( size ), t, true );
        }

        template< typename T >
        std::tuple< Slice, bool > set( const T &value, const int size = sizeOf< T >() ) {
            if ( size > _datasize )
                return std::make_tuple( Slice(), false );
            *reinterpret_cast< T * >( _data ) = value;
            return std::make_tuple( advance( size ), true );
        }

        Slice advance( const int size ) {
            auto copy = *this;
            copy._data += size;
            copy._datasize -= size;
            return copy;
        }

      private:
        char *_data;
        int _datasize;
    };

    Slice slice() { return Slice( _data.get(), _datasize ); }

  private:
    std::unique_ptr< char > _data;
    int _datasize;
    TypeSignature _datatype;
};

template< typename T >
struct BinarySerializer {
    static_assert( std::is_integral< T >::value,
        "Automatic serialization works only for integers, please implement specific serialization support" );

    static int size( const T & ) { return sizeOf< T >(); }

    static std::tuple< Serialized::Slice, T, bool > get( Serialized::Slice data ) {
        // we just interpret the memory as number and advance the data pointer
        return data.get< T >();
    }

    static std::tuple< Serialized::Slice, bool > write( Serialized::Slice data, T value ) {
        // write number to memory and avance the data pointer
        return data.set( value );
    }
};

struct Serializer {
    template< typename What >
    static Serialized serialize( const What &w ) {
        auto tuple = w.tuple();
        Serialized serial{ w.type(), w.size() };
        serializeAs( serial.slice(), tuple );
        return serial;
    }

    template< typename What >
    static What deserialize( Serialized &s ) {
        assert_eq( What::type(), s.type(), "Wrong type for deserialization" );
        auto tuple = deserializeAs< typename What::Tuple >( s.slice() );
        return What( tuple );
    }

    template< typename Tuple >
    static Tuple deserializeAs( Serialized::Slice slice ) {
        return deserializeAs< Tuple, 0 >( slice );
    }

    template< typename Tuple >
    static void serializeAs( Serialized::Slice slice, const Tuple &tuple ) {
        serializeAs< 0 >( slice, tuple );
    }

  private:
    template< typename Tuple, int I, typename... Ts >
    static auto deserializeAs( Serialized::Slice slice, Ts &&...data ) ->
        typename std::enable_if< I == std::tuple_size< Tuple >::value, Tuple >::type
    {
        return std::make_tuple( std::forward< Ts >( data )... );
    }

    template< typename Tuple, int I, typename... Ts >
    static auto deserializeAs( Serialized::Slice slice, Ts &&...data ) ->
        typename std::enable_if< ( I < std::tuple_size< Tuple >::value ), Tuple >::type
    {
        using TypeToGet = typename std::tuple_element< I, Tuple >::type;
        TypeToGet value;
        Serialized::Slice cont;
        bool ok;
        std::tie( cont, value, ok ) = slice.get< TypeToGet >();
        assert( ok, "deserialization failure" );
        return deserializeAs< Tuple, I + 1 >( cont, std::forward< Ts >( data )..., value );
    }

    template< int I, typename Tuple >
    static auto serializeAs( Serialized::Slice, const Tuple & ) ->
        typename std::enable_if< I == std::tuple_size< Tuple >::value >::type
    { }

    template< int I, typename Tuple >
    static auto serializeAs( Serialized::Slice slice, const Tuple &tuple ) ->
        typename std::enable_if< ( I < std::tuple_size< Tuple >::value ) >::type
    {
        Serialized::Slice cont;
        bool ok;
        std::tie( cont, ok ) = slice.set( std::get< I >( tuple ) );
        assert( ok, "serialization failure" );
        serializeAs< I + 1 >( cont, tuple );
    }
};

}

#endif // SRC_SERIALIZATION_H
