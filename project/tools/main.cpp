#include <climits>
#include <elevator/elevator.h>
#include <elevator/scheduler.h>
#include <wibble/commandline/parser.h>
#include <iostream>
#include <elevator/udptools.h>
#include <atomic>
#include <thread>
#include <chrono>
#include <set>

namespace elevator {

using namespace wibble;
using namespace wibble::commandline;
using namespace udp;

struct Main {

	const Address sendAdd{ IPv4Address::localhost, Port{ 64123 } };
	const Address rcvAdd{ IPv4Address::localhost, Port{ 64125 } };
	const Address brdcastAdd{ IPv4Address::broadcast, Port{ 64125 } };
    StandardParser opts;
    OptionGroup *execution;
    IntOption *nodes;
    BoolOption *avoidRecovery;
    IntOption *elevatorId;
	const int peerMsg = 1000;
	std::set<IPv4Address> peerAddresses;
	int id = INT_MIN;

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

	void sendToPeers( atomic <bool> *trmntSend )
	{
		while( !*trmntSend ) {
			Socket snd_sock{ sendAdd, true };
			Packet pack( sizeof( int ) );
			pack.address() = brdcastAdd;
			pack.get<int>() = peerMsg;
			snd_sock.sendPacket( pack );
			std::this_thread::sleep_for( std::chrono::seconds( 0.5 ) );
		}
	}

	void  findPeersListener()
	{
		Socket rd_sock{ rcvAdd, true };
		//printf( "Starting Listener\n" );
		while( true ) {
			Packet pack = rd_sock.recvPacketWithTimeout( 300 );
            if ( pack.size() != 0 )
			{
				//printf( "Received Packet is: %d \n", pack.get< int >() );
				if( pack.get< int >() == peerMsg )
				{
					peerAddresses.insert( pack.address() );
					if( peerAddresses.size() == 3)
						trmntSend = true;
				}
			}
		}
	}

    void main() {
        
		std::atomic<bool> trmntSend (false);
		std::thread findPeers(sendToPeers, this, &trmntSend);
		findPeersListener();
		findPeers.join();

		Socket m_sock{ sendAdd, true }
		
		int i = 0;
		for( auto address : peerAddresses )
		{
			Packet pack;
			pack.address() = brdcastAdd;
			pack.get<int>() = i;
			i++;
		}
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
        ConcurrentQueue< Command > commandsToOthers;
        ConcurrentQueue< StateChange > stateChanges;

        Elevator elevator{ id, heartbeatManager.getNew( 100 /* ms */ ),
            commandsToLocalElevator, stateChanges };
        Scheduler scheduler{ heartbeatManager.getNew( 500 /* ms */ ),
            stateChanges, commandsToLocalElevator, commandsToOthers };

        elevator.run();
        scheduler.run();

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
