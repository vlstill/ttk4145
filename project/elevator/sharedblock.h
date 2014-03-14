// C++11    (c) 2014 Vladimír Štill <xstill@fi.muni.cz>

#include <mutex>
#include <type_traits>

#ifndef SRC_SHARED_BLOCK_H
#define SRC_SHARED_BLOCK_H

template< typename T >
struct SharedBlock {

    SharedBlock() : _data() { }
    explicit SharedBlock( T data ) : _data( std::move( data ) ) { }

    template< typename... Ts >
    SharedBlock( Ts &&...args ) : _data( std::forward< Ts >( args )... ) { }


    /** returns snapshot of consistent data
     */
    T snapshot() {
        Guard g{ _lock };
        return _data;
    }

    /** atomically modify data using given function,
     * data are passed to function and return value is passed out if function
     * return non-void value
     */
    template< typename Function >
    auto atomically( Function function ) -> typename
        std::enable_if<
            !std::is_void< typename std::result_of< Function( T & ) >::type >::value,
            typename std::result_of< Function( T & ) >::type >::type
    { // non-void version
        Guard g{ _lock };
        return function( _data );
    }

    template< typename Function >
    auto atomically( Function function ) -> typename
        std::enable_if< std::is_void< typename std::result_of< Function( T & ) >::type >::value >::type
    { // void version
        Guard g{ _lock };
        function( _data );
    }

  private:
    using Guard = std::unique_lock< std::mutex >;
    std::mutex _lock;
    T _data;
};

#endif // SRC_SHARED_BLOCK_H
