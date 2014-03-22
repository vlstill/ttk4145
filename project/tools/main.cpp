#include <climits>
#include <iostream>
#include <set>
#include <cstring>

#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <wibble/commandline/parser.h>

#include <elevator/driver.h>
#include <elevator/elevator.h>
#include <elevator/scheduler.h>
#include <elevator/udpqueue.h>
#include <elevator/sessionmanager.h>

void handler( int sig, siginfo_t *info, void * ) {
    elevator::Driver driver;
    driver.stopElevator();
    std::cerr << "SIGHANDLER: elevator stopped" << std::endl;
    if ( sig != SIGCHLD ) {
        exit( sig );
    } else {
        int pid = info->si_pid;
        waitpid( pid, nullptr, WNOHANG );
    }
}

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

    Main( int argc, const char **argv ) : opts( "elevator", "1.0" ) {
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

    void setupChild() {
        struct sigaction act;
        memset( &act, 0, sizeof( struct sigaction ) );
        act.sa_handler = SIG_DFL;
        sigaction( SIGCHLD, &act, nullptr );
        sigaction( SIGINT, &act, nullptr );

        runElevator();
    }

    void watchChild() {
        while ( true ) {
            pause(); // wait for signal
            initRecovery();
        }
    }

    void setupSignals() {
        struct sigaction act;
        memset( &act, 0, sizeof( struct sigaction ) );
        act.sa_sigaction = handler;
        act.sa_flags = SA_SIGINFO;
        sigaction( SIGCHLD, &act, nullptr ); // child died
        sigaction( SIGINT, &act, nullptr ); // ctrl+c
    }

    void initRecovery() {
        setupSignals();
        std::cerr << "Starting auto-recovery wrapper... " << std::flush;
        int pid = fork();
        if ( pid == 0 ) { // child
            std::cerr << "Child started" << std::endl;
            setupChild();
        } else if ( pid > 0 ) { // parent
            std::cerr << "OK" << std::endl;
            watchChild();
        } else {
            std::cerr << "Fatal error: fork failed" << std::endl;
            exit( 1 );
        }
    }

    void main() {

        if ( avoidRecovery->boolValue() ) {
            runElevator();
        } else {
            initRecovery();
        }
    }

    void runElevator() {
        int id;
        GlobalState global;
        HeartBeatManager heartbeatManager;
        SessionManager sessman{ global };

        if ( optNodes->boolValue() && optNodes->intValue() > 1 ) {
            std::cerr << "Initializing network connections, this might take some time" << std::endl
                      << "Please start other peers and wait... " << std::flush;
            nodes = optNodes->intValue();
            sessman.connect( heartbeatManager.getNew( 5000 ), nodes );
            id = sessman.id();
            std::cerr << "OK" << std::endl;
        } else {
            id = 0;
        }

        std::cout << "Starting elevator, id " << id << " of " << nodes << " elevators." << std::endl;

        ConcurrentQueue< Command > commandsToLocalElevator;
        ConcurrentQueue< Command > commandsToOthers;
        ConcurrentQueue< StateChange > stateChangesIn;
        ConcurrentQueue< StateChange > stateChangesOut;

        QueueReceiver< Command > commandsToLocalElevatorReceiver {
            Address{ IPv4Address::any, commandPort },
            commandsToLocalElevator,
            [id]( const Command &comm ) { return comm.targetElevatorId == id; }
        };
        QueueReceiver< StateChange > stateChangesInReceiver {
            Address{ IPv4Address::any, stateChangePort },
            stateChangesIn,
            [id]( StateChange &chan ) {
                // this might seem weird, but clocks are not synchonized so the
                // best approximation of timestamp of change is the time
                // we received it
                chan.state.timestamp = now();
                return chan.state.id != id;
            }
        };
        QueueSender< StateChange > stateChangesOutSender {
            commSend,
            Address{ IPv4Address::broadcast, stateChangePort },
            stateChangesOut
        };
        QueueSender< Command > commandsToOthersReceiver {
            commSend,
            Address{ IPv4Address::broadcast, commandPort },
            commandsToOthers
        };

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
        Elevator elevator {
            id,
            commandsToLocalElevator,
            stateChangesIn
        };
        Scheduler scheduler {
            id,
            elevator.info(),
            global,
            stateChangesIn,
            stateChangesOut,
            commandsToOthers,
            commandsToLocalElevator
        };

        if ( nodes > 1 ) {
            commandsToLocalElevatorReceiver.run();
            stateChangesInReceiver.run();
            commandsToOthersReceiver.run();
            stateChangesOutSender.run();
        }

        if ( sessman.needRecoveryState() )
            elevator.recover( sessman.recoveryState() );

        elevator.run( heartbeatManager.getNew( 100 ) );
        scheduler.run( heartbeatManager.getNew( 200 ), heartbeatManager.getNew( 200 ) );

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
