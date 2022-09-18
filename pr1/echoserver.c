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
#include <arpa/inet.h>

// A buffer large enough to contain the longest allowed string
#define BUFSIZE 16
#define FALSE 0

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
    {"help", no_argument, NULL, 'h'},
    {"port", required_argument, NULL, 'p'},
    {"maxnpending", required_argument, NULL, 'm'},
    {NULL, 0, NULL, 0}};

/* Main ========================================================= */
int main(int argc, char **argv)
{
    int maxnpending = 5;
    int option_char;
    //char *message = "!Hello Summer!";
    int portno = 10823; /* port to listen on */

    // Parse and set command line arguments
    while ((option_char =
                getopt_long(argc, argv, "hx:m:p:", gLongOptions, NULL)) != -1)
    {
        switch (option_char)
        {
        case 'm': // server
            maxnpending = atoi(optarg);
            break;
        case 'p': // listen-port
            portno = atoi(optarg);
            break;
        case 'h': // help
            fprintf(stdout, "%s \n", USAGE);
            exit(0);
            break;
        default:
            fprintf(stderr, "%s \n", USAGE);
            exit(1);
        }
    }

    setbuf(stdout, NULL); // disable buffering

    if ((portno < 1025) || (portno > 65535))
    {
        fprintf(stderr, "%s @ %d: invalid port number (%d)\n", __FILE__, __LINE__,
                portno);
        exit(1);
    }
    if (maxnpending < 1)
    {
        fprintf(stderr, "%s @ %d: invalid pending count (%d)\n", __FILE__, __LINE__,
                maxnpending);
        exit(1);
    }

    /* Socket Code Here */

    //  define ipv6 socket, so it takes both ipv4/6 conn

    int socketFid = -1, socketFidConn = -1;
    int response, v6only = 1;
    char buffer[BUFSIZE];
    struct sockaddr_in6 serveraddr, clientaddr;
    unsigned int addrlen = sizeof(clientaddr);
    char addStr[INET6_ADDRSTRLEN];

    // socket creation: conn domain, tcp, ip
    // ret: file descriptor socketFid
    if ((socketFid = socket(AF_INET6, SOCK_STREAM, 0)) < 0)
        perror("socket() creation failed");
    //printf("socketFid:%d\n", socketFid); // new line does matter

    // socket usage: sid, reuse on
    // ret: T or F
    if (setsockopt(socketFid, SOL_SOCKET, SO_REUSEADDR,
                   (char *)&v6only, sizeof(v6only)) < 0)
        perror("setsockopt() set reuse failed");

    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin6_family = AF_INET6;
    serveraddr.sin6_port = htons(portno);
    serveraddr.sin6_addr = in6addr_any;

    // socket binding: to addr/port
    if (bind(socketFid,
             (struct sockaddr *)&serveraddr,
             sizeof(serveraddr)) < 0)
        perror("bind() socket failed");

    // socket listen: numbner of pending
    if (listen(socketFid, maxnpending) < 0)
        perror("listen() threhold failed");

    //printf("socket conn is ready for client to connect().\n");
    //printf("\n");

    //keep-alive socket
    do{
        // socket accept: extract conn
        if ((socketFidConn = accept(socketFid, NULL, NULL)) < 0){
            perror("accept() a connection failed");
            close(socketFidConn);
            continue;
        }
        else
        {
            // look up client addr version
            getpeername(socketFidConn, (struct sockaddr *)&clientaddr, &addrlen);
            if (inet_ntop(AF_INET6, &clientaddr.sin6_addr, addStr, sizeof(addStr)))
            {
                //printf("Client address in current conn is %s\n", addStr);
                //printf("Client port in current conn is %d\n", ntohs(clientaddr.sin6_port));
                fflush(stdout);
            }
        }

        // recv a buff size payload
        bzero(buffer, BUFSIZE);
        response = recv(socketFidConn, buffer, BUFSIZE, 0);
        if (response < 0){
            perror("recv() failed");
            close(socketFidConn);
            continue;
        }

        //printf("%d bytes of data were received\n", response);
        //if (response == 0 || response < sizeof(buffer))
            //printf("conn disconncted before data transmitted\n");
        
        printf("%s", buffer);

        // send a buff size payload

        if(strlen(buffer)<BUFSIZE)
            response = send(socketFidConn, buffer, strlen(buffer), 0);
        else{
            char msg[16];
            memcpy(msg,&buffer[0],14);
            msg[15] = '\0';
            response = send(socketFidConn, msg, BUFSIZE, 0);
        }

        if (response < 0){
            perror("send() failed");
            close(socketFidConn);
            continue;
        }
        //printf("socketFid:%d\n", socketFid);
        //printf("socketFidConn:%d\n", socketFidConn);

        if (socketFid < 0)
            close(socketFid);
        close(socketFidConn);
        //printf("fetch next connection\n");
        //printf("\n");
    }while(1);
    return 0;
}