// C++11    (c) 2014 Vladimír Štill <xstill@fi.muni.cz>

#include <atomic>

/* simple spin-lock raii guard to be used with atomic_flag */

#ifndef SRC_SPIN_LOCK_H
#define SRC_SPIN_LOCK_H

namespace elevator {

struct SpinLock {
    SpinLock( std::atomic_flag &flag ) : flag( flag ) {
        while ( flag.test_and_set( std::memory_order_acquire ) )
        { }
    }

    ~SpinLock() {
        flag.clear( std::memory_order_release );
    }

  private:
    std::atomic_flag &flag;
};

}

#endif // SRC_SPIN_LOCK_H
