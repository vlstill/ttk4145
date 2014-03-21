#include <thread>
#include <elevator/udptools.h>
#include <elevator/heartbeat.h>
#include <elevator/globalstate.h>

#ifndef ELEVATOR_SESSION_MANAGER_H
#define ELEVATOR_SESSION_MANAGER_H

namespace elevator {

struct SessionManager {
    static const udp::Address commSend;
    static const udp::Address commRcv;
    static const udp::Address commBroadcast;

    SessionManager( GlobalState & );

    /* initializes connection (blocking) with count members and then spawns
     * thread for recovery assistance if count > 1 */
    void connect( HeartBeat &, int count );
    bool connected() const { return _id != INT_MIN; }
    bool needRecoveryState() const { return !_recoveryState.isNothing(); }
    ElevatorState recoveryState() const { return _recoveryState.value(); }

    int id() const { return _id; }

  private:
    GlobalState &_state;
    std::set< udp::IPv4Address > _peers;
    int _id;
    bool _initialized;
    wibble::Maybe< ElevatorState > _recoveryState;
    udp::Socket _sendSock;
    udp::Socket _recvSock;

    std::thread _thr;

    void _loop( HeartBeat & );
    void _initSender( std::atomic< int > * );
    void _initListener( std::atomic< int > *, int );
};

}

#endif // ELEVATOR_SESSION_MANAGER_H
