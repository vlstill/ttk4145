#include <stdio.h>
#include <sys/types.h>
#include "udptools.h"
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

using namespace udp;

const Address SendAdd{IPv4Address::localhost, Port{ 64123 } };
const Address RcvAdd{IPv4Address::localhost, Port{ 64125 } };


void  Primary(int);
void  Listener(int);
void startNewPrim(int);

int  main(void)
{
      Listener( 0 );   
}


void startNewPrim(int y)
{
     pid_t  pid;
     pid = fork();
     if (pid == 0) 
          Primary(y);
     else if (pid > 0)
          Listener( pid );
	 else
          printf("Error creating child");
}

void  Primary(int z)
{
     Socket snd_sock{SendAdd, true};
     printf("Startig Primary\n");
     for (int i=z; i>0; i++)
     {
		  printf("%d \n", i);
          Packet pack(sizeof(int));
          pack.address() = RcvAdd;
          pack.get<int>() = i;
          snd_sock.sendPacket(pack);
          usleep(500000);
     } 
}

void  Listener(int childPid)
{
    Socket rd_sock{RcvAdd, true};
    int x = 0;
    printf("Starting Listener\n");
    while(true){
            Packet pack = rd_sock.recvPacketWithTimeout( 3000 );
            if (pack.size() == 0)
                 break;
            x = pack.get< int >();
            printf("Received Packet is: %d \n", x);
    }
	if ( childPid != 0 ) {
		kill( childPid, SIGKILL );
		waitpid( childPid, NULL, 0 );
	}
    startNewPrim(x + 1);
}
