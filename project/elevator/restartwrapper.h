// C++11    (c) 2014 Vladimír Štill <xstill@fi.muni.cz>

#include <functional>
#include <elevator/test.h>

/* might look a bit magical, but the only thing this does is that is wraps
 * functions in such a way it is automatically restarted if it had thrown
 * given exception (which defaults to GenerallAssertionFailed, corresponding
 * to conditional assertion failure (not assert_unimplemented and assert_unreachable)
 *
 * You shoud alwais use the restartWrapper function, not the RestartWrapper
 * class itself
 *
 * the only thing which makes it long is the fact it behaves just like
 * std::thread constructor when passed member function pointers
 * (it auto calls them on first argument ontionally dereferencing it
 * if it is a pointer)
 */

#ifndef SRC_THREAD_RESTART_WRAPPER_H
#define SRC_THREAD_RESTART_WRAPPER_H

namespace elevator {

template< typename Exception, typename Function >
class RestartWrapper {
    Function f;
  public:
    RestartWrapper( Function &&f ) : f( std::forward< Function >( f ) ) { }

    template< typename... Args >
    auto operator()( Args ...args )
        -> decltype( (this->f)( args... ) )
    {
        for ( ;; ) {
            try {
                return f( args... );
            } catch ( Exception &ex ) { _catched( ex );  }
        }
    }

    template< typename That, typename... Args >
    auto operator()( That that, Args ...args )
        -> decltype( (that.*(this->f))( args... ) )
    {
        for ( ;; ) {
            try {
                return (that.*f)( args... );
            } catch ( Exception &ex ) { _catched( ex );  }
        }
    }

    template< typename That, typename... Args >
    auto operator()( That that, Args ...args )
        -> decltype( (that->*(this->f))( args... ) )
    {
        for ( ;; ) {
            try {
                return (that->*f)( args... );
            } catch ( Exception &ex ) { _catched( ex );  }
        }
    }

  private:
    void _catched( Exception &ex ) {
        std::cout << "Catched: " << ex.what() << " restarting." << std::endl;
    }
};

template< typename Exception = test::GenerallAssertionFailed,
          typename Function >
RestartWrapper< Exception, Function > restartWrapper( Function &&f ) {
    return RestartWrapper< Exception, Function >( std::forward< Function >( f ) );
}


}

#endif // SRC_THREAD_RESTART_WRAPPER_H
