#ifndef SERVER_H
#define SERVER_H

#include "utils.h"

int credit(float amount);
int debit(float amount);
int balance();
int start();
int makeAccount(char* acctName);
int finish();
void sigHandler(int signo);
void printList();
int connectionHandler(char* server, char* port);
void createThreads(int socketDesc);
void* serverOut(void* out);
void* clientIn(void* in);
#endif



