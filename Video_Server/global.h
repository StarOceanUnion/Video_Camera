#ifndef GLOBAL_H
#define GLOBAL_H

#include <semaphore.h>

typedef struct _global_t global_t;

struct _global_t {    /* 全局变量结构体 */
	char *frame;    /* 帧数据首地址 */
	unsigned int frame_bytes;    /* 帧数据字节数 */
	sem_t camera_sem;    /* 摄像头信号量 */
	sem_t net_sem;    /* 网络传输信号量 */
};

#endif    /* GLOBAL_H */