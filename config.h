#define MAX_LINE_SIZE 4096
#ifndef LETS_TALK_CONFIG
#define LETS_TALK_CONFIG
#include "list.h"

struct Config {
  List* _outgoing;
  List* _incoming;
  int localPort;
  int remotePort;
  char* remoteIP;
  int status;
};

#endif