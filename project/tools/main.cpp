#include <climits>
#include <iostream>
#include <atomic>
#include <thread>
#include <chrono>
#include <set>

#include <wibble/commandline/parser.h>

#include <elevator/elevator.h>
#include <elevator/scheduler.h>
#include <elevator/udptools.h>
#include <elevator/udpqueue.h>


namespace elevator {

using namespace wibble;
using namespace wibble::commandline;
using namespace udp;

struct Main {

    const Address commSend{ IPv4Address::any, Port{ 64123 } };
    const Address commRcv{ IPv4Address::any, Port{ 64125 } };
    const Address commBroadcast{ IPv4Address::broadcast, Port{ 64125 } };

    const Port stateChangePort{ 64016 };
    const Port commandPort{ 64017 };

    StandardParser opts;
    OptionGroup *execution;
    IntOption *optNodes;
    BoolOption *avoidRecovery;
    const int peerMsg = 1000;
    std::set< IPv4Address > peerAddresses;
    int id = INT_MIN;
    int nodes = 1;

    Main( int argc, const char **argv ) : opts( "elevator", "0.1" ) {
        // setup options
        execution = opts.createGroup( "Execution options" );
        optNodes = execution->add< IntOption >(
                "nodes", 'n', "nodes", "",
                "specifies number of elevator nodes to expect" );

        avoidRecovery = execution->add< BoolOption >(
                "avoid recovery", 0, "avoid-recovery", "",
                "avoid auto-recovery when program is killed (do not fork)" );

        opts.usage = "";
        opts.description = "Elevator control software as a project for the "
                           "TTK4145 Real-Time Programming at NTNU. Controls "
                           "multiple elevator connected with local network.\n"
                           "(c) 2014, Vladimír Štill and Sameh Khalil\n"
                           "https://github.com/vlstill/ttk4145/tree/master/project";

        opts.add( execution );

        // parse options
        try {
            opts.parse( argc, argv );
        } catch ( exception::BadOption &ex ) {
            std::cerr << "FATAL: " << ex.fullInfo() << std::endl;
            exit( 1 );
        }
        if ( opts.help->boolValue() || opts.version->boolValue() )
            exit( 0 );
    }

    // initPhase == 0 means we don't know other peers
    // initPhase == 1 means we know but they may not know (so we must wait)
    // initPhase == 2 means all peers know other peers
    void sendToPeers( std::atomic< int > *initPhase ) {
        Socket snd_sock{ commSend, true };
        snd_sock.enableBroadcast();
        while( *initPhase < 2 ) {
            Packet pack( sizeof( int ) );
            pack.address() = commBroadcast;
            pack.get< int >() = peerMsg + (*initPhase > 0 ? 1 : 0);
            bool sent = snd_sock.sendPacket( pack );
            assert( sent, "send failed" );
            std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );
        }
    }

    void  findPeersListener( std::atomic< int > *initPhase ) {
        Socket rd_sock{ commRcv, true };
        std::set< IPv4Address > barrier;

        while ( *initPhase < 2 ) {
            Packet pack = rd_sock.recvPacketWithTimeout( 300 );
            if ( pack.size() != 0 ) {
                if ( pack.get< int >() == peerMsg ) {
                    peerAddresses.insert( pack.address().ip() );
                } else if ( pack.get< int >() == peerMsg + 1 ) {
                    peerAddresses.insert( pack.address().ip() ); // it might still be that we dont know
                    barrier.insert( pack.address().ip() );
                }
                if ( int( barrier.size() ) == nodes )
                    *initPhase = 2;
                else if ( int( peerAddresses.size() ) == nodes )
                    *initPhase = 1;
            }
        }
    }

    void main() {

        if ( optNodes->boolValue() && optNodes->intValue() > 1 ) {
            nodes = optNodes->intValue();
            std::atomic< int > initPhase{ 0 };
            std::thread findPeers( &Main::sendToPeers, this, &initPhase );
            findPeersListener( &initPhase );
            findPeers.join();

            auto localAddrs = IPv4Address::getMachineAddresses();
            int i = 0;
            // the set is sorted (guaranteed by C++) and same on all elevators
            for ( auto addr : peerAddresses ) {
                if ( localAddrs.find( addr ) != localAddrs.end() ) {
                    id = i;
                    break;
                }
                ++i;
            }
            assert_leq( 0, id, "Could not assing ID" );
            std::cout << "detected id " << id << std::endl;
        } else {
            id = 0;
        }

        if ( avoidRecovery->boolValue() ) {
            runElevator( id );
        } else {
            assert_unimplemented();
        }
    }

    void runElevator( int id ) {
        std::cout << "starting elevator, id " << id << " of " << nodes << " elevators" << std::endl;
        HeartBeatManager heartbeatManager;

        ConcurrentQueue< Command > commandsToLocalElevator;
        ConcurrentQueue< Command > commandsToOthers;
        ConcurrentQueue< StateChange > stateChangesIn;
        ConcurrentQueue< StateChange > stateChangesOut;

        std::unique_ptr< QueueReceiver< Command > > commandsToLocalElevatorReceiver;
        std::unique_ptr< QueueReceiver< StateChange > > stateChangesInReceiver;
        std::unique_ptr< QueueSender< StateChange > > stateChangesOutSender;
        std::unique_ptr< QueueSender< Command > > commandsToOthersReceiver;

        if ( nodes > 1 ) {
            commandsToOthersReceiver.reset( new QueueSender< Command >{
                    commSend,
                    Address{ IPv4Address::broadcast, commandPort },
                    commandsToOthers
                } );

            commandsToLocalElevatorReceiver.reset( new QueueReceiver< Command >{
                    Address{ IPv4Address::any, commandPort },
                    commandsToLocalElevator,
                    [id]( const Command &comm ) { return comm.targetElevatorId == id; }
                } );

            stateChangesInReceiver.reset( new QueueReceiver< StateChange >{
                    Address{ IPv4Address::any, stateChangePort },
                    stateChangesIn
                } );

            stateChangesOutSender.reset( new QueueSender< StateChange >{
                    commSend,
                    Address{ IPv4Address::broadcast, stateChangePort },
                    stateChangesOut
                } );
        }

        /* about heartbeat lengths:
         * - elevator loop is polling and never sleeping, therefore its scheduling
         *   is quite relieable and it can have short heartbeat,
         *   furthermore we need to make sure we will detect any floor change,
         *   therefore heartbeat it must respond faster then is time to cross sensor
         *   (approx 400ms), also note that even with heartbeat of 10ms the elevator
         *   seems to be running reliably
         * - scheduler again has quite tight demand, as it should be able to handle
         *   all state changes from elevator, but we have to be more carefull here
         *   as it is sleeping sometimes
         */
        Elevator elevator{ id, heartbeatManager.getNew( 50 /* ms */ ),
            commandsToLocalElevator, stateChangesIn };
        Scheduler scheduler{ id, heartbeatManager.getNew( 100 /* ms */ ),
            elevator.info(),
            stateChangesIn, stateChangesOut, commandsToOthers, commandsToLocalElevator };

        if ( nodes > 1 ) {
            commandsToLocalElevatorReceiver->run();
            stateChangesInReceiver->run();
            commandsToOthersReceiver->run();
            stateChangesOutSender->run();
        }
        elevator.run();
        scheduler.run();

        // wait for 2 seconds so that everything has chance to start
        // it something fails to start in this time heartbeat will
        // fail and process will be terminated
        std::this_thread::sleep_for( std::chrono::seconds( 2 ) );
        // heartbeat failure will throw an exception which will terminate
        // process (as it is not catched)
        heartbeatManager.runInThisThread();
    }
};

}

int main( int args, const char **argv ) {
    elevator::Main m( args, argv );
    m.main();
}
