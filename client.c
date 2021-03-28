#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <semaphore.h> 
#include <unistd.h> 
#include <sys/socket.h>
#include "config.h"
#include <arpa/inet.h>
#include "list.h"
#include <netinet/in.h>
#include <sys/time.h>

extern sem_t mutexOutgoing;

char* encryptMsg(char*);
int checkOnlineStatus(int, struct sockaddr_in *);

int *chatClient(void * args)
{
  struct Config *config;
  config = (struct Config *) args;
  List* outgoing = config->_outgoing;
	struct sockaddr_in myaddr, remaddr;
	int socket_fd;
  socklen_t slen=sizeof(remaddr);
	char *server = config->remoteIP;

  // create socket
	if ((socket_fd=socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		perror("Error: could not create socket\n");
    exit(EXIT_FAILURE);
  }



	memset((char *)&myaddr, 0, sizeof(myaddr));
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	myaddr.sin_port = htons(0);

	if (bind(socket_fd, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
		perror("bind failed");
		return 0;
	}       

	/* Remote address */

	memset((char *) &remaddr, 0, sizeof(remaddr));
	remaddr.sin_family = AF_INET;
	remaddr.sin_port = htons(config->remotePort);
	if (inet_aton(server, &remaddr.sin_addr)==0) {
		fprintf(stderr, "inet_aton() failed\n");
		exit(1);
	}

  while(1) {

    // wait to acquire mutex lock
    sem_wait(&mutexOutgoing);
    
    // send all messages in incoming list
    if(List_count(outgoing) != 0) {
      char* msg = (char *) List_first(outgoing);
      if (strcmp(msg, "!status") == 0) {
        if (sendto(socket_fd, msg, strlen(msg), 0, (struct sockaddr *)&remaddr, slen)< 0) {
          perror("Error: could not send message!");
          exit(EXIT_FAILURE);
        }
        char buf[MAX_LINE_SIZE];
        struct timeval tv;
        tv.tv_sec = 5;
        tv.tv_usec = 0;
        if (setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
            perror("Error");
        }
        int recvlen;
        if ((recvlen = recvfrom(socket_fd, buf, MAX_LINE_SIZE, 0, (struct sockaddr *)&remaddr, &slen)) < 0) {
          printf("Offline\n");
        } else {
          printf("Online\n");
        }

      } else {
        char* encryptedMsg = encryptMsg(msg);
        if (sendto(socket_fd, encryptedMsg, strlen(List_first(outgoing)), 0, (struct sockaddr *)&remaddr, slen)<0) {
          perror("Error: could not send message!");
          exit(EXIT_FAILURE);
        }
        free(encryptedMsg);
      }
      free(List_first(outgoing));
      List_remove(outgoing);
    }

    // release lock
    sem_post(&mutexOutgoing);
  }
	close(socket_fd);
	return 0;
}

char* encryptMsg(char* unencrypted) {
  int encryptionKey = 3;
  int len = strlen(unencrypted);
  char* encrypted = malloc(len + 5);
  memset(encrypted, '\0', len + 5);
  for(int i = 0; i < len; i++) {
    encrypted[i] = (char) (unencrypted[i] + encryptionKey);
  }
  return encrypted;
}
