// C++11 (c) 2014 Vladimír Štill

#include "serialization.h"

using namespace serialization;

struct TestSerialization {
    Test deserializeInt() {
        std::tuple< int > testdata{ 42 };

        serialization::Serialized::Slice slice{ reinterpret_cast< char * >( &testdata ), sizeof( int ) };

        int got = 0;
        std::tie( got ) = serialization::Serializer::deserializeAs< std::tuple< int > >( slice );
        assert_eq( got, 42, "Incorrect deserialization" );
    }

    Test serializeInt() {
        std::tuple< int > testdata{ 42 };
        Serialized serial{ TypeSignature::NoType, sizeof( int ) };
        Serializer::serializeAs( serial.slice(), testdata );
        assert_eq( *reinterpret_cast< const int * >( serial.rawData() ), 42, "Incorrect serialization" );
    }

    Test serializationDeserialization() {
        using Data = std::tuple< int, long, bool, int, bool >;
        Data origdata{ 0xff00ff00, 0x7700ff770077ff00, true, 42, false };

        Serialized serial{ TypeSignature::NoType, sizeof( Data ) };
        Serializer::serializeAs( serial.slice(), origdata );
        Data deserialized = Serializer::deserializeAs< Data >( serial.slice() );
        assert_eq( std::get< 0 >( origdata ), std::get< 0 >( deserialized ), "serialization-deserialization error" );
        assert_eq( std::get< 1 >( origdata ), std::get< 1 >( deserialized ), "serialization-deserialization error" );
        assert_eq( std::get< 2 >( origdata ), std::get< 2 >( deserialized ), "serialization-deserialization error" );
        assert_eq( std::get< 3 >( origdata ), std::get< 3 >( deserialized ), "serialization-deserialization error" );
        assert_eq( std::get< 4 >( origdata ), std::get< 4 >( deserialized ), "serialization-deserialization error" );
    }

    Test typed() {
        struct TestData {
            using Tuple = std::tuple< long, long, bool >;
            static TypeSignature type() { return TypeSignature::TestType; }
            static int size() { return sizeof( Tuple ); }
            Tuple tuple() const { return std::make_tuple( x, y, p ); }

            explicit TestData( const Tuple &data ) :
                x( std::get< 0 >( data ) ), y( std::get< 1 >( data ) ),
                p( std::get< 2 >( data ) )
            { }

            TestData( long x, long y, bool p ) : x( x ), y( y ), p( p ) { }
            TestData() = default;

            long x, y;
            bool p;
        };

        TestData data{ 0x7700ff770077ff00, 0x0077ff770077ff00, true };
        Serialized serial = Serializer::serialize( data );
        TestData deser = Serializer::deserialize< TestData >( serial );
        assert_eq( data.x, deser.x, "serialization-deserialization error" );
        assert_eq( data.y, deser.y, "serialization-deserialization error" );
        assert_eq( data.p, deser.p, "serialization-deserialization error" );
    }
};


