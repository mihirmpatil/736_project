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

int main(int argc, char *argv[])
{

daemonize();
syslog(LOG_NOTICE, "daemonize complete");
while (1)
{
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char buffer[256];
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
//    printf("Please enter the message: ");
    bzero(buffer,256);
    strcpy(buffer, "THis is a test message\n");
//    fgets(buffer,255,stdin);
    n = write(sockfd,buffer,strlen(buffer));
syslog(LOG_NOTICE, "Written data to socket");
    if (n < 0) 
         error("ERROR writing to socket");
    bzero(buffer,256);
    n = read(sockfd,buffer,255);
    if (n < 0) 
         error("ERROR reading from socket");
syslog(LOG_NOTICE, "Read data from socket");
//    printf("%s\n",buffer);
    close(sockfd);
    sleep(2);	//give back cpu
}
    return 0;
}
