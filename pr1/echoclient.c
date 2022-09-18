#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

// A buffer large enough to contain the longest allowed string
#define BUFSIZE 16
#define FALSE 0
#define MAX_LENGTH_IPV6 128

#define USAGE                                                            \
    "usage:\n"                                                           \
    "  echoclient [options]\n"                                           \
    "options:\n"                                                         \
    "  -s                  Server (Default: localhost)\n"                \
    "  -p                  Port (Default: 10823)\n"                      \
    "  -m                  Message to send to server (Default: \"Hello " \
    "Summer.\")\n"                                                       \
    "  -h                  Show this help message\n"

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
    {"server", required_argument, NULL, 's'},
    {"port", required_argument, NULL, 'p'},
    {"message", required_argument, NULL, 'm'},
    {"help", no_argument, NULL, 'h'},
    {NULL, 0, NULL, 0}};

int isSocketSuccess(int socketFid);

/* Main ========================================================= */

int main(int argc, char **argv)
{
    unsigned short portno = 10823;
    int option_char = 0;
    char *message = "Hello Summer!!";
    char *hostname = "::ffff:127.0.0.1";

    // Parse and set command line arguments
    while ((option_char =
                getopt_long(argc, argv, "p:s:m:hx", gLongOptions, NULL)) != -1)
    {
        switch (option_char)
        {
        default:
            fprintf(stderr, "%s\n", USAGE);
            exit(1);
        case 's': // server
            hostname = optarg;
            break;
        case 'p': // listen-port
            portno = atoi(optarg);
            break;
        case 'h': // help
            fprintf(stdout, "%s\n", USAGE);
            exit(0);
            break;
        case 'm': // message
            message = optarg;
            break;
        }
    }

    setbuf(stdout, NULL); // disable buffering

    if ((portno < 1025) || (portno > 65535))
    {
        fprintf(stderr, "%s @ %d: invalid port number (%d)\n", __FILE__, __LINE__,
                portno);
        exit(1);
    }

    if (NULL == message)
    {
        fprintf(stderr, "%s @ %d: invalid message\n", __FILE__, __LINE__);
        exit(1);
    }

    if (NULL == hostname)
    {
        fprintf(stderr, "%s @ %d: invalid host name\n", __FILE__, __LINE__);
        exit(1);
    }

    /* Socket Code Here */
    int socketId = -1, receive;
    char buffer[BUFSIZE];

    int localhost=-1;
    int ipv4=-1;
    int ipv6=-1;
    int ipv4_6=-1;

    if(strcmp(hostname,"localhost")==0 && strlen(hostname)==9){
        localhost = 0;
        //printf("localhost \n");
    }else if( strchr(hostname, ':') && !strchr(hostname, '.') ){
        ipv6 = 0;
        //printf("ipv6 \n");
    }else if(strchr(hostname, '.') && !strchr(hostname, ':')){
        ipv4 = 0;
        //printf("ipv4 \n");
    }else if(strchr(hostname, '.') && strchr(hostname, ':')){
        ipv4_6 = 0;
        //printf("ipv4_6 \n");
    }else{
        fprintf(stderr, "%s @ %d: invalid host name\n", __FILE__, __LINE__);
        exit(1);
    }

    char strPort[6];
    sprintf(strPort, "%d", portno);
    
    //ipv4
    struct hostent *hostp;
    struct sockaddr_in serv_addr4;
        memset(&serv_addr4, 0, sizeof(serv_addr4));

    //ipv6
	struct sockaddr_in6 server_addr6;
        memset(&server_addr6, 0, sizeof(server_addr6));

    if(localhost == 0){
        serv_addr4.sin_family = AF_INET;
        serv_addr4.sin_port   = htons(portno);
        hostp =   gethostbyname(hostname);
        if (hostp == (struct hostent *)NULL){
            perror("Host not found");
            exit(1);
        }
        memcpy(&serv_addr4.sin_addr,hostp->h_addr,sizeof(serv_addr4.sin_addr));
        
        socketId = socket(AF_INET, SOCK_STREAM, 0);
        if(isSocketSuccess(socketId))
            if(isSocketSuccess(connect(socketId, (struct sockaddr*)&serv_addr4, sizeof(serv_addr4))))
                localhost = 0;
    }
    else if(ipv4 == 0){
        serv_addr4.sin_family = AF_INET;
        serv_addr4.sin_port   = htons(portno);
        if (inet_pton(AF_INET, hostname, &serv_addr4.sin_addr) <= 0) {
            perror("\nInvalid address/ Address v4 not supported \n");
            exit(1);
        }

        socketId = socket(AF_INET, SOCK_STREAM, 0);
        if(isSocketSuccess(socketId))
            if(isSocketSuccess(connect(socketId, (struct sockaddr*)&serv_addr4, sizeof(serv_addr4))))
                ipv4 = 0;
    }
    else if(ipv6 == 0 || ipv4_6 == 0){
	    server_addr6.sin6_family = AF_INET6;
        if (inet_pton(AF_INET6, hostname, &server_addr6.sin6_addr) <= 0) {
            perror("\nInvalid address/ Address v6 not supported \n");
            exit(1);
        }
	    server_addr6.sin6_port = htons(portno);

        socketId = socket(AF_INET6, SOCK_STREAM, 0);
        if(isSocketSuccess(socketId))
            if(isSocketSuccess(connect(socketId, (struct sockaddr*)&server_addr6, sizeof(server_addr6))))
                ipv6 = 0;
    }

    if(strlen(message)<BUFSIZE)
        receive = send(socketId, message, strlen(message), 0);
    else{
        char msg[16];
        memcpy(msg,&message[0],14);
        msg[15] = '\0';
        receive = send(socketId, msg, BUFSIZE, 0);
    }

    if (receive < 0)
        perror("send() failed");


    bzero(buffer, BUFSIZE);
    receive = recv(socketId, buffer, BUFSIZE, 0);
    if (receive == -1) {
        perror("recv() failed");
    }
    
    printf("%s", buffer);

    close(socketId);
}

int isSocketSuccess(int socketFid){
    if (socketFid < 0){
        perror("socket() failed");
        exit(1);
    }else
        return 1;
}