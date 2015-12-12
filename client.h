#ifndef CLIENT_H
#define CLIENT_H
#include "utils.h"


void* serverOut(void* output);
void* clientIn(void* input);
void createThreads(int sockInfo);
int connectionHandler(char* server, char* port);

#endif
