#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <fcntl.h>
#include <sys/stat.h>
#include <syslog.h>
#include"allStats.h"

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

static void daemonize(void)
{
    pid_t pid, sid;
    int fd; 

    /* already a daemon */
    if ( getppid() == 1 ) return;

    /* Fork off the parent process */
    pid = fork();
    if (pid < 0)  
    {   
        exit(EXIT_FAILURE);
    }   

    if (pid > 0)  
    {   
        exit(EXIT_SUCCESS); /*Killing the Parent Process*/
    }   

    /* At this point we are executing as the child process */

    /* Create a new SID for the child process */
    sid = setsid();
    if (sid < 0)  
    {
        exit(EXIT_FAILURE);
    }

    /* Change the current working directory. */
    if ((chdir("/")) < 0)
    {
        exit(EXIT_FAILURE);
    }


    fd = open("/dev/null",O_RDWR, 0);

    if (fd != -1)
    {
        dup2 (fd, STDIN_FILENO);
        dup2 (fd, STDOUT_FILENO);
        dup2 (fd, STDERR_FILENO);

        if (fd > 2)
        {
            close (fd);
        }
    }

    /*resettign File Creation Mask */
    umask(027);
    openlog("client", LOG_PID, LOG_DAEMON);
}



void error(const char *msg)
{
    syslog(LOG_ERR,msg);
//    exit(0);
}

char* getSerializedBuffer(size_t* buffsize)
{
	//Get All System Info
	SystemStats *sStats=getAllProcStats();
	//Allocate apporpriate buffer size
	//first int is number of procs, the first proc struct is for the system
	size_t size= sizeof(int) + (sStats->numProcs+1)*sizeof(ProcStats);
	*buffsize=size;
	size_t curr=0;
    char *buffer=malloc(size);
	ProcStats *system=malloc(sizeof(ProcStats));
	system->cpuUsage=sStats->cpuUsage;
	//First: integer for numProcs
	memcpy(buffer,&(sStats->numProcs),sizeof(sStats->numProcs));
	curr+=sizeof(sStats->numProcs);
	//Second: ProcStat for system
	memcpy(buffer+curr, system, sizeof(ProcStats));
	curr+=sizeof(ProcStats);
	//Last: Array of ProcStats for all pids
	memcpy(buffer+curr, sStats->procStats, (sStats->numProcs)*sizeof(ProcStats));

	free(sStats->procStats);	
	free(sStats);
	free(system);
	return buffer;
}


int main(int argc, char *argv[])
{

//daemonize();
//syslog(LOG_NOTICE, "daemonize complete");

size_t* buffsize=malloc(sizeof(size_t));

while (1)
{
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

	//Get stats in serialized form
	char *buffer=getSerializedBuffer(buffsize);
    if (argc < 3) {
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(0);
    }
    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
        error("ERROR connecting");

	//Tell server what size to expect
	n = write(sockfd, (char*)buffsize, sizeof(size_t));
syslog(LOG_NOTICE, "Written data to socket");
    if (n < 0) 
         error("ERROR writing to socket");

	n = write(sockfd,buffer,*buffsize);
syslog(LOG_NOTICE, "Written data to socket");
    if (n < 0) 
         error("ERROR writing to socket");
	

/*		 
    bzero(buffer,256);
    n = read(sockfd,buffer,255);
    if (n < 0) 
         error("ERROR reading from socket");
syslog(LOG_NOTICE, "Read data from socket");
//    printf("%s\n",buffer);
 */
 
    close(sockfd);
    sleep(2);	//give back cpu
}
    return 0;
}
