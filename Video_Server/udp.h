#ifndef UDP_H
#define UDP_H

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>

#include "global.h"

int udp_init(int port);
void *udp_thread(void *globalp);

#endif    /* UDP_H */