#include <elevator/elevator.h>
#include <wibble/commandline/parser.h>
#include <iostream>

namespace elevator {

using namespace wibble;
using namespace wibble::commandline;

struct Main {

    StandardParser opts;
    OptionGroup *execution;
    IntOption *nodes;
    BoolOption *avoidRecovery;
    IntOption *elevatorId;

    Main( int argc, const char **argv ) : opts( "elevator", "0.1" ) {
        // setup options
        execution = opts.createGroup( "Execution options" );
        nodes = execution->add< IntOption >(
                "nodes", 'n', "nodes", "",
                "specifies number of elevator nodes to expect" );
        elevatorId = execution->add< IntOption >(
                "elevator id", 'i', "elevator-id", "",
                "explicitly set elevator id" );

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

    void main() {
        int id = INT_MIN;
        if ( elevatorId->boolValue() ) {
            id = elevatorId->intValue();
        } else {
            // TODO
        }

        if ( avoidRecovery->boolValue() ) {
            runElevator( id );
        } else {
            assert_unimplemented();
        }
    }

    void runElevator( int id ) {
        HeartBeatManager heartbeatManager;

        ConcurrentQueue< Command > commandsToLocalElevator;
        /*
        ConcurrentQueue< Status > statusFromLocalElevator;
        ConcurrentQueue< Command > commandsToOthers;
        ConcurrentQueue< Status > statusFromOthers;
        */

        Elevator elevator{ id, heartbeatManager.getNew( 100 /* ms */ ), &commandsToLocalElevator };

        elevator.run();

        // wait for 2 seconds so that everything has chance to start
        // it something fails to start in this time heartbeat will
        // fail and process will be terminated
        std::this_thread::sleep_for( std::chrono::seconds( 2 ) );
        heartbeatManager.runInThisThread();
    }
};

}

int main( int args, const char **argv ) {
    elevator::Main m( args, argv );
    m.main();
}
