#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <semaphore.h> 
#include <arpa/inet.h>
#include "config.h"
#include <netinet/in.h>
#include <unistd.h> 
#include "list.h"
#include <string.h>


extern sem_t mutexIncoming;

char* decryptMsg(char*);

int *chatServer(void * args)
{
  struct Config *config;
  config = (struct Config *) args;
  List* incoming = config->_incoming;
  // local address
	struct sockaddr_in locAddr;
  // remote address
	struct sockaddr_in remAddr;

  // len of remote address
	socklen_t addrlen = sizeof(remAddr);
	int recvlen;		
	int socket_fd;				


  // create UDP socket
	if ((socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("Error: could not create socket\n");
		exit(EXIT_FAILURE);
	}

	memset((char *)&locAddr, 0, sizeof(locAddr));
	locAddr.sin_family = AF_INET;
	locAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	locAddr.sin_port = htons(config->localPort);

	if (bind(socket_fd, (struct sockaddr *)&locAddr, sizeof(locAddr)) < 0) {
		perror("Error: could not bind the socket");
		exit(EXIT_FAILURE);
	}
	  

	while (1) {

    char buf[MAX_LINE_SIZE];
    memset(buf, '0', sizeof(buf));
		recvlen = recvfrom(socket_fd, buf, MAX_LINE_SIZE, 0, (struct sockaddr *)&remAddr, &addrlen);
		if (recvlen > 0) {
			buf[recvlen] = '\0';

      if (strcmp(buf, "!status") == 0) {
        char *pingMsg = "PING";
        if (sendto(socket_fd, pingMsg, strlen(pingMsg), 0, (struct sockaddr *)&remAddr, addrlen) < 0)
			    perror("sendto");
      } else {
        // wait to acquire mutex lock
        sem_wait(&mutexIncoming);
        // add input to incoming messages list
        char* store = strdup(buf);
        char* decryptedMsg = decryptMsg(store);
        if (strcmp(decryptedMsg, "!exit") == 0)
          config->status = 0;
        List_append(incoming, decryptedMsg);
        free(store);
        // release lock
        sem_post(&mutexIncoming);
      }

		}	else {
			printf("Error: could not understand incoming message\n");
    }
	}
}

char* decryptMsg(char* encrypted) {
  int encryptionKey = 3;
  int len = strlen(encrypted);
  char* decrypted = malloc(len + 5);
  memset(decrypted, '\0', len + 5);
  for(int i = 0; i < len; i++) {
    decrypted[i] = (char) (encrypted[i] - encryptionKey);
  }
  return decrypted;
}