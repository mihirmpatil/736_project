/* A simple server in the internet domain using TCP
   The port number is passed as an argument */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include "allStats.h"


//Parses and frees buffer
SystemStats* parseBuffer(char* buffer)
{
	ProcStats* system=malloc(sizeof(ProcStats));
	size_t curr=0;
	SystemStats *sStats= malloc(sizeof(SystemStats));
	//First: integer for numProcs
	memcpy(&(sStats->numProcs), buffer,sizeof(sStats->numProcs));
	curr+=sizeof(sStats->numProcs);
	//Second: ProcStat for system
	memcpy(system, buffer+curr, sizeof(ProcStats));
	curr+=sizeof(ProcStats);
	sStats->procStats=malloc((sStats->numProcs)*sizeof(ProcStats));
	//Last: Array of ProcStats for all pids
	memcpy(sStats->procStats, buffer+curr, (sStats->numProcs)*sizeof(ProcStats));

	//Set system fields
	sStats->cpuUsage=system->cpuUsage;

	free(system);
	free(buffer);
	buffer=NULL;
	return sStats;
	
}


void error(const char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
	 int i;
	 size_t *buffSize=malloc(sizeof(size_t));
     SystemStats *sStats;
	
	 int sockfd, newsockfd, portno;
     socklen_t clilen;
     char *buffer;
     struct sockaddr_in serv_addr, cli_addr;
     int n;
     if (argc < 2) {
         fprintf(stderr,"ERROR, no port provided\n");
         exit(1);
     }
     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0) 
        error("ERROR opening socket");
     bzero((char *) &serv_addr, sizeof(serv_addr));
     portno = atoi(argv[1]);
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(portno);
     if (bind(sockfd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0) 
              error("ERROR on binding");
     listen(sockfd,5);
     clilen = sizeof(cli_addr);

   while (1)
   {
     newsockfd = accept(sockfd, 
                 (struct sockaddr *) &cli_addr, 
                 &clilen);
     if (newsockfd < 0) 
          error("ERROR on accept");

	 //Server reads size of next message
     n = read(newsockfd,(char*)buffSize,sizeof(size_t));
     if (n < 0) error("ERROR reading from socket");

	 //allocate buffer
	 buffer=malloc(*buffSize);

	 //Read the actual message
     n = read(newsockfd,buffer,*buffSize);
     if (n < 0) error("ERROR reading from socket");
	 //parse out info from buffer
	 sStats=parseBuffer(buffer);


	 //Print out info
	 printf("Total CPU usage: %Lf%\n", sStats->cpuUsage);
	 printf("Number of processes:%d\n",sStats->numProcs);	
	 for(i=0; i< sStats->numProcs; i++)
	 {
		if(sStats->procStats[i].cpuUsage>0)
		{
			printf("Pid:%ld cpuUsage:%Lf%\n",sStats->procStats[i].pid, sStats->procStats[i].cpuUsage);
		}
	 }

	 //free memory for next round
	 free(sStats->procStats);
	 free(sStats);

/*
     printf("Here is the message: %s\n",buffer);
     n = write(newsockfd,"I got your message",18);
     if (n < 0) error("ERROR writing to socket");
*/ 
   }
     close(newsockfd);
     close(sockfd);
     return 0; 
}
