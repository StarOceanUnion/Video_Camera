#ifndef CAMERA_H 
#define CAMERA_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <sys/mman.h>
#include <signal.h>
#include <pthread.h>

#include "global.h"

int camera_open(char *dev);
void camera_view();
int camera_init(int width, int height, int rate);
void *camera_thread(void *globalp);

#endif    /* CAMERA_H */