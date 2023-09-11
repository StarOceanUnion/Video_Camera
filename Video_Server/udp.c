#include "udp.h"

static int udp_socket;

/**
 * 描述：UDP服务端初始化
 * 参数：port 使用的端口
 * 返回值：0 初始化成功
 *         -1 初始化失败
**/
int udp_init(int port)
{
	int ret;
	struct sockaddr_in server_socket_addr;
		
	/*创建套接字*/
    udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket < 0) {
        perror("Error: socket");
        return -1;
    }
    
	/*服务端填充sockaddr_in结构体*/
    server_socket_addr.sin_family = AF_INET;
    server_socket_addr.sin_port = htons(port);
    server_socket_addr.sin_addr.s_addr = INADDR_ANY;
    memset(server_socket_addr.sin_zero, 0, 8);
    
	/*绑定套接字*/
    ret = bind(udp_socket, (const struct sockaddr *)&server_socket_addr, sizeof(struct sockaddr));
    if (ret < 0) {
        perror("Error: bind");
        return -1;
    }
	
	/*显示使用的传输协议和端口*/
	printf("port       : %u \n", htons(server_socket_addr.sin_port));
	printf("protocol   : UDP \n");
	printf("\n");
	
	return 0;
}

/**
 * 描述：UDP传输线程清理函数
 * 参数：arg 无用
 * 返回值：无
**/
void udp_cleanup(void *arg)
{
	printf("The udp thread has exited or has been cancelled. \n");
	close(udp_socket);
	printf("The cleanup of udp thread has finished. \n");
}

/**
 * 描述：UDP发送数据。此函数为非阻塞式发送，忽略错误且不检验数据是否被完整发送
 * 参数：socketfd 套接字文件描述符
 *       data 待发送数据的首地址
 *       bytes 待发送数据的字节数
 *       socketaddr 接收端套接字地址
 *       addrlen 套接字地址长度
 * 返回值：无
**/
void udp_send(int socketfd, const char *data, unsigned int bytes, struct sockaddr *socketaddr, int addrlen)
{
	int ret;
	unsigned int sent_bytes = 0;
	
	while (bytes - sent_bytes > 65507) {
		ret = sendto(socketfd, data + sent_bytes, 65507, 0, socketaddr, addrlen);
		if (ret < 0)
			perror("Error: sendto");
		sent_bytes += 65507;
		usleep(5000);
	}
	ret = sendto(socketfd, data + sent_bytes, bytes - sent_bytes, 0, socketaddr, addrlen);
	if (ret < 0)
		perror("Error: sendto");
	usleep(5000);
}

/**
 * 描述：UDP传输线程
 * 参数：globalp 全局变量结构体指针
 * 返回值：无
**/
void *udp_thread(void *globalp)
{
	int ret;
	sigset_t sigset;
	struct sockaddr_in temp_socket_addr;
	struct sockaddr_in client_socket_addr;
	int addr_len = sizeof(struct sockaddr);
	int bytes;
	char udp_cmd[32] = {0};
	int connected_flag = 0;
	unsigned int net_frame_bytes;
	unsigned char header[6] = {0xA5, 0x5A};
	unsigned char tail[2] = {0x5A, 0xA5};
	
	pthread_cleanup_push(udp_cleanup, NULL);
	
	/*屏蔽SIGINT信号*/
	sigemptyset(&sigset);
	sigaddset(&sigset, SIGINT);
	ret = pthread_sigmask(SIG_BLOCK, &sigset, NULL);
	if (ret < 0)
		perror("Warning: pthread_sigmask");
	
	while (1) {
		/*接收UDP命令*/
        memset(udp_cmd, 0, sizeof(udp_cmd));
        bytes = recvfrom(udp_socket, udp_cmd, sizeof(udp_cmd), MSG_DONTWAIT, (struct sockaddr *)&temp_socket_addr, &addr_len);
		if (bytes > 0) {
			printf("Receive from %s:%d \n"
				   "Message is: \"%s\" \n", inet_ntoa(temp_socket_addr.sin_addr), ntohs(temp_socket_addr.sin_port), udp_cmd);
			/*UDP连接命令*/
			if (strcmp(udp_cmd, "Connect") == 0) {
				client_socket_addr = temp_socket_addr;
				sem_post(&((global_t *)globalp)->camera_sem);
				udp_send(udp_socket, "Connected", 10, (struct sockaddr *)&temp_socket_addr, addr_len);
				connected_flag = 1;
			}
			/*UDP断开命令*/
			else if (strcmp(udp_cmd, "Disconnect") == 0 && connected_flag == 1 
					 && strcmp(inet_ntoa(temp_socket_addr.sin_addr), inet_ntoa(client_socket_addr.sin_addr)) == 0
					 && temp_socket_addr.sin_port == client_socket_addr.sin_port) {
				connected_flag = 0;
			}
		}
		
		if (connected_flag == 1) {
			sem_wait(&((global_t *)globalp)->net_sem);
			
			/*发送该帧图像的帧头（包含该帧图像的字节数）*/
			net_frame_bytes = htonl(((global_t *)globalp)->frame_bytes);
			memcpy(header + 2, &net_frame_bytes, 4);
			udp_send(udp_socket, header, 6, (struct sockaddr *)&client_socket_addr, addr_len);


			/*发送该帧图像的数据内容*/
			udp_send(udp_socket, ((global_t *)globalp)->frame, ((global_t *)globalp)->frame_bytes, (struct sockaddr *)&client_socket_addr, addr_len);


			/*发送该帧图像的帧尾*/
			udp_send(udp_socket, tail, 2, (struct sockaddr *)&client_socket_addr, addr_len);

			
			/*打印该帧图像的字节数*/
			printf("frame bytes: %d \n", ((global_t *)globalp)->frame_bytes);
			
			sem_post(&((global_t *)globalp)->camera_sem);
		}
	}
	
	pthread_exit(NULL);
	pthread_cleanup_pop(0);
}


