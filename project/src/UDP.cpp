#include<iostream>
#include<arpa/inet.h>
#include<unistd.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
using namespace std;

struct udp_writer{
	int sockfd = socket(AF_INET,SOCK_DGRAM,0);
	struct sockaddr_in serv,client;
	char buffer[256];
	socklen_t l = sizeof(client);
	int wr= sendto(sockfd,"Test",4,0,(struct sockaddr *)&client,l);
}

struct udp_reader{
	int sockfd = socket(AF_INET,SOCK_DGRAM,0);
	struct sockaddr_in serv,client;
	char buffer[256];
	socklen_t l = sizeof(client);
	socklen_t m = sizeof(serv);
	int rc = recvfrom(sockfd,buffer,256,0,(struct sockaddr *)&client,&l);
}

int main(){

	int recvlen;                    /* # bytes received */
	udp_writer writer;
	udp_reader reader;

	 /* create a UDP socket */

        if (writer.sockfd < 0) {
                perror("cannot create socket\n");
                return 0;
        }

		 /* bind the socket to any valid IP address and a specific port */

        myaddr.sin_family = AF_INET;
        myaddr.sin_addr.s_addr = htonl(IPAddress); //Fill in with a valid IP address
        myaddr.sin_port = htons(PORT);  //Fill in with a valid Port

		if (bind(writer.sockfd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
                perror("bind failed");
                return 0;
        }

		/*Receiving Data*/

		for (;;) {
                printf("waiting on port %d\n", PORT); //Fill in the Port
                recvlen = reader.rc;
                printf("received %d bytes\n", recvlen);
                if (recvlen > 0) {
                        reader.buf[recvlen] = 0;
                        printf("received message: \"%s\"\n", reader.buf);
                }
        }
}