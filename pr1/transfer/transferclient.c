#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFSIZE 512
#define FALSE 0
#define MAX_LENGTH_IPV6 128
int isSocketSuccess(int socketFid);
int receiveFileOverSocket();

#define USAGE                                                \
  "usage:\n"                                                 \
  "  transferclient [options]\n"                             \
  "options:\n"                                               \
  "  -h                  Show this help message\n"           \
  "  -p                  Port (Default: 10823)\n"            \
  "  -s                  Server (Default: localhost)\n"      \
  "  -o                  Output file (Default cs6200.txt)\n" 

/* OPTIONS DESCRIPTOR ====================================================== */
static struct option gLongOptions[] = {{"server", required_argument, NULL, 's'},
                                       {"output", required_argument, NULL, 'o'},
                                       {"port", required_argument, NULL, 'p'},
                                       {"help", no_argument, NULL, 'h'},
                                       {NULL, 0, NULL, 0}};

/* Main ========================================================= */
  unsigned short portno = 10823;
  int option_char = 0;
  char *hostname = "localhost";
  char *filename = "cs6200.txt";

  int socketId = -1, receive;
  char buffer[BUFSIZE];

  int localhost=-1;
  int ipv4=-1;
  int ipv6=-1;
  int ipv4_6=-1;

    
  //ipv4
  struct hostent *hostp;
  struct sockaddr_in serv_addr4;
    
  //ipv6
	struct sockaddr_in6 server_addr6;

int main(int argc, char **argv) {

  setbuf(stdout, NULL);

  /* Parse and set command line arguments */ 
  while ((option_char =
              getopt_long(argc, argv, "xp:s:h:o:", gLongOptions, NULL)) != -1) {
    switch (option_char) {
      case 's':  // server
        hostname = optarg;
        break;
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
      case 'o':  // filename
        filename = optarg;
        break;
    }
  }

  if (NULL == hostname) {
    fprintf(stderr, "%s @ %d: invalid host name\n", __FILE__, __LINE__);
    exit(1);
  }

  if (NULL == filename) {
    fprintf(stderr, "%s @ %d: invalid filename\n", __FILE__, __LINE__);
    exit(1);
  }

  if ((portno < 1025) || (portno > 65535)) {
    fprintf(stderr, "%s @ %d: invalid port number (%d)\n", __FILE__, __LINE__,
            portno);
    exit(1);
  }

  // Socket Code Here 
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

        memset(&serv_addr4, 0, sizeof(serv_addr4));

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

      if(receiveFileOverSocket()==-1)
          perror("receiveFileOverSocket() failed");
    
    close(socketId);
      
  return 0;
}

int isSocketSuccess(int socketFid){
    if (socketFid < 0){
        perror("socket() failed");
        exit(1);
    }else
        return 1;
}

int receiveFileOverSocket(){
  FILE *file_ptr;

  file_ptr = fopen(filename, "w+");
  if(file_ptr == NULL){
    perror("Error in open the file! \n");
    return -1;
	} 
int zeroRecvCount = 0;
  while(1) { 
      int response;
      bzero(buffer, BUFSIZE);
      int readIn = recv(socketId, buffer, BUFSIZE, 0);
      //printf("readIn%d",readIn);

      if (readIn < 0) {
          perror("Error in receive the data! \n");
          fclose(file_ptr);
          return -1;
      }
      if(readIn > 0){
        response = fwrite(buffer, 1, readIn, file_ptr);
        if (response < 0){
            perror("fwrite() failed");
            fclose(file_ptr);
            return -1;
        }
      }
      if(readIn == 0){
        zeroRecvCount += 1;
        if(zeroRecvCount >= 5){
          fclose(file_ptr);
          return 0;
        }
      }
  }
  return -1;
}