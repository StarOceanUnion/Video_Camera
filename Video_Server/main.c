#include "main.h"

static global_t global = {NULL, 0};
static int tcp_flag = 0;
static pthread_t camera_tid;
static pthread_t udp_tid;
static pthread_t tcp_tid;

/**
 * 描述：打印帮助信息
 * 参数：无
 * 返回值：无
**/
void help()
{
	printf("------------------------------------------------------------------\n"
		   "Format: \n"
		   "Video_Server [option] [argument] \n"
		   "------------------------------------------------------------------\n"
		   "Options: \n"
		   "-d, --device  : Specify a video capture device \n"
		   "-v, --view    : View the parameters of the video device \n"
		   "-s, --size    : Specify frame size \n"
		   "-r, --rate    : Specify frame rate \n"
		   "-p, --port    : Specify the port for network transport \n"
		   "-t, --tcp     : Using the TCP transport protocol (UDP by default) \n"
		   "-h, --help    : Show help information \n"
		   "------------------------------------------------------------------\n"
		   "Examples: \n"
		   "-d, --device  : Video_Server -d /dev/video0 \n"
		   "-v, --view    : Video_Server -v \n"
		   "-s, --size    : Video_Server -s 640*480 \n"
		   "-r, --rate    : Video_Server -r 30 \n"
		   "-p, --port    : Video_Server -p 2000 \n"
		   "-t, --tcp     : Video_Server -t \n"
		   "-h, --help    : Video_Server -h \n"
		   "------------------------------------------------------------------\n");
}

/**
 * 描述：SIGINT信号处理函数
 * 参数：sig 信号编号
 * 返回值：无
**/
void sig_handler(int sig)
{
	int ret;
	
	printf("\n");
	ret = pthread_cancel(camera_tid);
	if (ret != 0)
		fprintf(stderr, "Error: pthread_cancel: camera_tid: %s \n", strerror(ret));
	if (tcp_flag == 0) {
		ret = pthread_cancel(udp_tid);
		if (ret != 0)
			fprintf(stderr, "Error: pthread_cancel: udp_tid: %s \n", strerror(ret));
	}
	else {
		ret = pthread_cancel(tcp_tid);
		if (ret != 0)
			fprintf(stderr, "Error: pthread_cancel: tcp_tid: %s \n", strerror(ret));
	}
}

/**
 * 描述：主函数（主线程）
 * 参数：argc 命令行参数的个数
 *       argv 命令行参数的值
 * 返回值：0 线程正常终止
 *         -1 线程异常终止
**/
int main(int argc, char *argv[])
{
	int ret;
	int c = 0;
	struct option long_opts[] = {
		{"device", required_argument, NULL, 'd'},
		{"view", no_argument, NULL, 'v'},
		{"size", required_argument, NULL, 's'},
		{"rate", required_argument, NULL, 'r'},
		{"port", required_argument, NULL, 'p'},
		{"tcp", no_argument, NULL, 't'},
		{"help", no_argument, NULL, 'h'}
	};
	char *device = NULL;
	int view_flag = 0;
	char *mul;
	int frm_width = 640;
	int frm_height = 480;
	int frm_rate = 30;
	int port = 2000;
	
	/* 命令行参数解析 */
    while ((c = getopt_long(argc, argv, "d:vs:r:p:th", long_opts, NULL)) > 0) {
		switch (c) {
		case 'd':
			device = strdup(optarg);
			break;
		case 'v':
			view_flag = 1;
			break;
		case 's':
			frm_width = strtol(optarg, &mul, 10);
			if (*mul == '*')
				frm_height = strtol(mul + 1, NULL, 10);
			break;
		case 'r':
			frm_rate = atoi(optarg);
			break;
		case 'p':
			port = atoi(optarg);
			break;
		case 't':
			tcp_flag = 1;
			break;
		case 'h':
			help();
			return 0;
			break;
		default:
			help();
			return 0;
			break;
		}
	}
	
	/*打开摄像头*/
	ret = camera_open(device);
	if (ret < 0)
		return -1;
	
	if (view_flag == 1) {
		/*查看摄像头的参数*/
		camera_view();
		return 0;
	}
	
	/*摄像头初始化*/
	ret = camera_init(frm_width, frm_height, frm_rate);
	if (ret < 0)
		return -1;
	
	if (tcp_flag == 0) {
		/*UDP服务端初始化*/
		ret = udp_init(port);
		if (ret < 0)
			return -1;
	}
	else {
		/*TCP服务端初始化*/
		ret = tcp_init(port);
		if (ret < 0)
			return -1;
	}
	
	/*摄像头信号量初始化*/
	ret = sem_init(&global.camera_sem, 0, 0);
	if (ret < 0) {
		perror("Error: sem_init: camera_sem");
		return -1;
	}
	
	/*网络传输信号量初始化*/
	ret = sem_init(&global.net_sem, 0, 0);
	if (ret < 0) {
		perror("Error: sem_init: net_sem");
		return -1;
	}
	
	/*创建视频采集线程*/
	ret = pthread_create(&camera_tid, NULL, camera_thread, &global);
	if(ret != 0) {
		fprintf(stderr, "Error: pthread_create: camera_thread: %s \n", strerror(ret));
		return -1;
	}
	
	if (tcp_flag == 0) {
		/*创建UDP传输线程*/
		ret = pthread_create(&udp_tid, NULL, udp_thread, &global);
		if(ret != 0) {
			fprintf(stderr, "Error: pthread_create: udp_thread: %s \n", strerror(ret));
			return -1;
		}
	}
	else {
		/*创建TCP传输线程*/
		ret = pthread_create(&tcp_tid, NULL, tcp_thread, &global);
		if(ret != 0) {
			fprintf(stderr, "Error: pthread_create: tcp_thread: %s \n", strerror(ret));
			return -1;
		}
	}
	
	/*等待SIGINT信号*/
	if (signal(SIGINT, sig_handler) == SIG_ERR)
		fprintf(stderr, "Warning: signal failed! \n");
	pause();
	
	/*回收子线程*/
	pthread_join(camera_tid, NULL);
	if (tcp_flag == 0)
		pthread_join(udp_tid, NULL);
	else
		pthread_join(tcp_tid, NULL);
	
	return 0;
}


