#include <sys/socket.h>
#include <sys/types.h>
#include <errno.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFSIZE 512
int sendFileOverSocket();
void callSocket();

#define USAGE                                            \
  "usage:\n"                                             \
  "  transferserver [options]\n"                         \
  "options:\n"                                           \
  "  -h                  Show this help message\n"       \
  "  -f                  Filename (Default: 6200.txt)\n" \
  "  -p                  Port (Default: 10823)\n"

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {
    {"help", no_argument, NULL, 'h'},
    {"port", required_argument, NULL, 'p'},
    {"filename", required_argument, NULL, 'f'},
    {NULL, 0, NULL, 0}};

// const
    char *filename = "6200.txt"; // file to transfer 
    int portno = 10823;          // port to listen on 
    int option_char;

//  define ipv6 socket, so it takes both ipv4/6 conn
    int socketFid = -1;
    int socketFidConn = -1;
    int response, v6only = 1;
    char buffer[BUFSIZE];
    struct sockaddr_in6 serveraddr, clientaddr;
    unsigned int addrlen = sizeof(clientaddr);
    char addStr[INET6_ADDRSTRLEN];

void callSocket(){
    // socket creation: conn domain, tcp, ip
    // ret: file descriptor socketFid
    if ((socketFid = socket(AF_INET6, SOCK_STREAM, 0)) < 0)
        perror("socket() creation failed");
    //printf("socketFid:%d\n", socketFid); // new line does matter

    // socket usage: sid, reuse on
    // ret: T or F
    if (setsockopt(socketFid, SOL_SOCKET,  SO_REUSEADDR, (char *)&v6only, sizeof(v6only)) < 0)
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
    if (listen(socketFid, 10) < 0)
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
        }else{
            // look up client addr version
            getpeername(socketFidConn, (struct sockaddr *)&clientaddr, &addrlen);
            if (inet_ntop(AF_INET6, &clientaddr.sin6_addr, addStr, sizeof(addStr)))
            {
                //printf("Client address in current conn is %s\n", addStr);
                //printf("Client port in current conn is %d\n", ntohs(clientaddr.sin6_port));
                fflush(stdout);
            }
        }


        // send a buff size payload
        if(sendFileOverSocket()==-1){
            perror("sendFileOverSocket() failed");
            close(socketFidConn);
            //continue;
        }else{
          close(socketFidConn);
        }
        
        //printf("fetch next connection\n");
        //printf("\n");
    }while(1);
}   

int sendFileOverSocket(){
  FILE *file_ptr;
  long totalSize;
  long sendSize = 0;

  file_ptr = fopen(filename, "r");
  if(file_ptr == NULL){
    perror("Error in open the file! \n");
    return -1;
	} 

  // obtain file size:
  fseek (file_ptr , 0 , SEEK_END);
  totalSize = ftell (file_ptr);
  rewind (file_ptr);

  while(1) {
      bzero(buffer, BUFSIZE);
      int readIn = fread(buffer, 1, BUFSIZE, file_ptr);
      //printf("readIn%d",readIn);
      sendSize += readIn;
      if (readIn < 0) {
          perror("Error in read the file! \n");
          return -1;
      }
      if(readIn > 0){
        response = send(socketFidConn, buffer, readIn, 0);
        if (response < 0){
            perror("send() failed");
            return -1;
        }
      }
      if(sendSize == totalSize){
        return 0;
      }
  }
  return -1;
}
int main(int argc, char **argv) {

  setbuf(stdout, NULL);  // disable buffering

  // Parse and set command line arguments
  while ((option_char =
              getopt_long(argc, argv, "hf:xp:", gLongOptions, NULL)) != -1) {
    switch (option_char) {
      case 'p':  // listen-port
        portno = atoi(optarg);
        break;
      case 'h':  // help
        fprintf(stdout, "%s", USAGE);
        exit(0);
        break;
      default:
        fprintf(stderr, "%s", USAGE);
        exit(1);
      case 'f':  // file to transfer
        filename = optarg;
        break;
    }
  }

  if ((portno < 1025) || (portno > 65535)) {
    fprintf(stderr, "%s @ %d: invalid port number (%d)\n", __FILE__, __LINE__,
            portno);
    exit(1);
  }

  if (NULL == filename) {
    fprintf(stderr, "%s @ %d: invalid filename\n", __FILE__, __LINE__);
    exit(1);
  }

  /* Socket Code Here */
  callSocket();

  return 0;

}
