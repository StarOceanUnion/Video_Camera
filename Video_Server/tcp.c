#include "tcp.h"

static int tcp_socket;

/**
 * 描述：TCP服务端初始化
 * 参数：port 使用的端口
 * 返回值：0 初始化成功
 *         -1 初始化失败
**/
int tcp_init(int port)
{
	int ret;
	struct sockaddr_in server_socket_addr;
	
	/*创建套接字文件*/
    tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_socket < 0) {
        perror("Error: socket");
        return -1;
    }
    
	/*服务端填充sockaddr_in结构体*/
    server_socket_addr.sin_family = AF_INET;
    server_socket_addr.sin_port = htons(port);
    server_socket_addr.sin_addr.s_addr = INADDR_ANY;
    memset(server_socket_addr.sin_zero, 0, 8);
    
	/*绑定套接字*/
    ret = bind(tcp_socket, (const struct sockaddr *)&server_socket_addr, sizeof(struct sockaddr));
    if (ret < 0) {
        perror("Error: bind");
        return -1;
    }
	
	/*监听*/
	ret = listen(tcp_socket, 1);
	if (ret < 0) {
		perror("Error: listen");
		return -1;
	}
	
	/*显示使用的传输协议和端口*/
	printf("port       : %u \n", htons(server_socket_addr.sin_port));
	printf("protocol   : TCP \n");
	printf("\n");
	
	return 0;
}

/**
 * 描述：TCP传输线程清理函数
 * 参数：arg 无用
 * 返回值：无
**/
void tcp_cleanup(void *arg)
{
	printf("The tcp thread has exited or has been cancelled. \n");
	close(tcp_socket);
	printf("The cleanup of tcp thread has finished. \n");
}

/**
 * 描述：TCP发送数据。此函数为阻塞式发送（除非TCP连接已断开），返回“0”则保证数据被完整发送
 * 参数：socketfd 套接字文件描述符
 *       data 待发送数据的首地址
 *       bytes 待发送数据的字节数
 * 返回值：0 发送成功
 *         -1 套接字文件已关闭（TCP连接已断开）
**/
int tcp_send(int socketfd, const char *data, unsigned int bytes)
{
	int ret;
	unsigned int sent_bytes = 0;
	
    while (sent_bytes < bytes) {
        ret = send(socketfd, data + sent_bytes, bytes - sent_bytes, MSG_NOSIGNAL);
        if (ret < 0) {
			if (errno == EPIPE)
				return -1;
            perror("Error: send");
			continue;
		}
        sent_bytes += ret;
    }
	
	return 0;
}

/**
 * 描述：TCP传输线程
 * 参数：globalp 全局变量结构体指针
 * 返回值：NULL
**/
void *tcp_thread(void *globalp)
{
	int ret;
	sigset_t sigset;
	int client_fd;
	struct sockaddr_in client_socket_addr;
	int addr_len = sizeof(struct sockaddr);
	int connected_flag = 0;
	unsigned int net_frame_bytes;
	
	pthread_cleanup_push(tcp_cleanup, NULL);
	
	/*屏蔽SIGINT信号*/
	sigemptyset(&sigset);
	sigaddset(&sigset, SIGINT);
	ret = pthread_sigmask(SIG_BLOCK, &sigset, NULL);
	if (ret < 0)
		perror("Warning: pthread_sigmask");
	
	while (1) {
		if (connected_flag == 0) {
			/*接受连接请求*/
			client_fd = accept(tcp_socket, (struct sockaddr *)&client_socket_addr, &addr_len);
			if (client_fd != -1) {
				/* TCP已连接 */
				printf("Get connection from %s:%d \n", inet_ntoa(client_socket_addr.sin_addr), ntohs(client_socket_addr.sin_port));
				sem_post(&((global_t *)globalp)->camera_sem);
				connected_flag = 1;
			}
		}
		else {
			sem_wait(&((global_t *)globalp)->net_sem);
			
			/*发送该帧图像的字节数*/
			net_frame_bytes = htonl(((global_t *)globalp)->frame_bytes);
			ret = tcp_send(client_fd, (char *)&net_frame_bytes, 4);
			if (ret < 0) {
				/* TCP已断开 */
				connected_flag = 0;
				continue;
			}
			/*发送该帧图像的数据内容*/
			ret = tcp_send(client_fd, ((global_t *)globalp)->frame, ((global_t *)globalp)->frame_bytes);
			if (ret < 0) {
				/* TCP已断开 */
				connected_flag = 0;
				continue;
			}
			/*打印该帧图像的字节数*/
			printf("frame bytes: %d \n", ((global_t *)globalp)->frame_bytes);
			
			sem_post(&((global_t *)globalp)->camera_sem);
		}
	}
	
	pthread_exit(NULL);
	pthread_cleanup_pop(0);
}


