#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "list.h"
#include <pthread.h>
#include <semaphore.h> 
#include <readline/readline.h>
#include <readline/history.h>
#include <regex.h>
#include "config.h"

int *chatServer(void *_incoming);
int *chatClient(void *_outgoing);

sem_t mutexIncoming;
sem_t mutexOutgoing;

void *getUserInput(void * args) {
  struct Config *config;
  config = (struct Config *) args;
  List* outgoing = config->_outgoing;
  int status = 0;
  printf("Welcome to Lets-Talk! Please type your messages now.\n");
  do {
    char *input;
    input = readline("");
    if (input == NULL || strlen(input) == 0) continue;
    if (strcmp(input, "!exit") == 0)
      config->status = 0;

    rl_clear_history();
    // wait to acquire mutex lock
    sem_wait(&mutexOutgoing);


    // add input to outgoing messages list
    char* toStore = strdup(input);
    List_append(outgoing, toStore);

    // release lock
    sem_post(&mutexOutgoing);
    free(input);
  } while (status != -1);
  pthread_exit(NULL);
  rl_clear_history();
}

void *printIncomingMessages(void * args) {
  struct Config *config;
  config = (struct Config *) args;
  List* incoming = config->_incoming;
  while (1) {
    // wait to acquire mutex lock
    sem_wait(&mutexIncoming);

    // print all messages in incoming list
    if(List_count(incoming) != 0) {
      printf("%s\n", (char *) List_first(incoming));
      fflush(stdout);
      free(List_first(incoming));
      List_remove(incoming);
    }

    // release lock
    sem_post(&mutexIncoming);
  }
  // return NULL;
}

int main(int argc, char **argv) {

  if (argc != 4) {
    printf("Execution format: ./lets-talk [local port number] [remote IP] [remote port number]\n");
    exit(EXIT_FAILURE);
  }

  int localPort, remotePort;
  char* remoteIP = argv[2];

  if (sscanf (argv[1], "%i", &localPort) != 1 || localPort > 49151 || localPort < 1024) {
    printf("Error: please provide a local port between 1024 - 49151\n");
    exit(EXIT_FAILURE);
  }

  if (sscanf (argv[3], "%i", &remotePort) != 1 || remotePort > 49151 || remotePort < 1024) {
    printf("Error: please provide a remote port between 1024 - 49151\n");
    exit(EXIT_FAILURE);
  }

  regex_t regex;
  int retVal;
  retVal = regcomp(&regex, 
            "^([0-9]|[1-9][0-9]|1([0-9][0-9])|2([0-4][0-9]|5[0-5]))."
            "([0-9]|[1-9][0-9]|1([0-9][0-9])|2([0-4][0-9]|5[0-5]))."
            "([0-9]|[1-9][0-9]|1([0-9][0-9])|2([0-4][0-9]|5[0-5]))."
            "([0-9]|[1-9][0-9]|1([0-9][0-9])|2([0-4][0-9]|5[0-5]))$", REG_EXTENDED);

  if (retVal != 0){
    printf("Error: could not compile regular express\n");
    exit(EXIT_FAILURE);
  }
  int val = regexec(&regex, remoteIP, 0, NULL, 0);
  if (val != 0) {
    printf("Error: invalid remote IP address. If using localhost, please enter: 127.0.0.1.\n");
    exit(EXIT_FAILURE);
  }
  regfree(&regex);

  List* incoming = List_create();
  List* outgoing = List_create();


  struct Config config;
  config.localPort = localPort;
  config.remotePort = remotePort;
  config.remoteIP = remoteIP;
  config._incoming = incoming;
  config._outgoing = outgoing;
  config.status = 1;

  sem_init(&mutexIncoming, 0, 1);
  sem_init(&mutexOutgoing, 0, 1);
  pthread_t serverThread;
  pthread_t clientThread;
  pthread_t inputThread;
  pthread_t printerThread;
  int serverThread_status, clientThread_status, inputThread_status, printerThread_status;

  serverThread_status = pthread_create(&serverThread, NULL, (void *) chatServer, &config);
  if (serverThread_status) {
    printf("Error: could not create server thread");
    exit(EXIT_FAILURE);
  }

  clientThread_status = pthread_create(&clientThread, NULL, (void *) chatClient, &config);
  if (clientThread_status) {
    printf("Error: could not create client thread");
    exit(EXIT_FAILURE);
  }

  inputThread_status = pthread_create(&inputThread, NULL, (void *) getUserInput, &config);
  if (inputThread_status) {
    printf("Error: could not create keyboard input thread");
    exit(EXIT_FAILURE);
  }

  printerThread_status = pthread_create(&printerThread, NULL, (void *) printIncomingMessages, &config);
  if (printerThread_status) {
    printf("Error: could not create keyboard input thread");
    exit(EXIT_FAILURE);
  }

  while(config.status) {

  }

  rl_clear_history();
  pthread_cancel(serverThread);
  pthread_cancel(clientThread);
  pthread_cancel(inputThread);
  pthread_cancel(printerThread);
  pthread_join(serverThread, NULL);
  pthread_join(clientThread, NULL);
  pthread_join(inputThread, NULL);
  pthread_join(printerThread, NULL);
  pthread_exit(0);
}