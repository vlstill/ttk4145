#include <thread>
#include <elevator/state.h>
#include <elevator/udptools.h>

#ifndef ELEVATOR_SESSION_MANAGER_H
#define ELEVATOR_SESSION_MANAGER_H

namespace elevator {

struct SessionManager {
    static const udp::Address commSend;
    static const udp::Address commRcv;
    static const udp::Address commBroadcast;

    SessionManager( GlobalState &glo ) : _state( glo ) { }

    /* initializes connection (blocking) with count members and then spawns
     * thread for recovery assistance if count > 1 */
    void connect( int count );
    bool connected() const { return _id != INT_MIN; }
    bool needRecoveryState() const { return _needRecovery; }
    ElevatorState recoveryState() const;

    int id() const { return _id; }

  private:
    GlobalState &_state;
    std::set< udp::IPv4Address > peers;
    int _id;
    bool _initialized;
    bool _needRecovery;

    std::thread _thr;

    void _loop();
};

}

#endif // ELEVATOR_SESSION_MANAGER_H
