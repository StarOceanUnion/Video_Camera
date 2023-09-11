#include "camera.h"

static int fd = -1;
static struct v4l2_buffer buf;
static void *frm_mem[4] = {NULL, NULL, NULL, NULL};

/**
 * 描述：打开摄像头设备
 * 参数：dev 设备名（包含路径）
 * 返回值：0 打开成功
 *         -1 打开失败
**/
int camera_open(char *dev)
{
	struct v4l2_capability capab;
	struct v4l2_fmtdesc fmtdesc;
	int open_success = 0;
	
	fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	
	if (dev != NULL) {    /*指定设备时，打开指定的设备*/
		fd = open(dev, O_RDWR);
		if (fd < 0) {
			perror("Error: open");
			return -1;
		}
		ioctl(fd, VIDIOC_QUERYCAP, &capab);
		if ((capab.capabilities & V4L2_CAP_VIDEO_CAPTURE) == 0) {
			fprintf(stderr, "Error: \"%s\" is not a video capture device! \n", dev);
			close(fd);
			return -1;
		}
		for (fmtdesc.index = 0; ioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc) == 0; fmtdesc.index++) {
			if (fmtdesc.pixelformat == V4L2_PIX_FMT_YUYV) {
				open_success = 1;
				printf("Device \"%s\" is opened successfully. \n", dev);
				break;
			}
		}
		if (open_success == 0) {
			fprintf(stderr, "Error: Device \"%s\" does not support MJPEG pixel format! \n", dev);
			close(fd);
			return -1;
		}
	}
	else {    /*未指定设备时，自动搜索并打开可用设备*/
		DIR *dirp;
		struct dirent *direntp;
		char device[261];
		
		dirp = opendir("/dev/");
		if (dirp == NULL) {
			perror("Error: opendir: \"/dev/\" \n");
			return -1;
		}
		printf("Searching for available devices... \n");
		while (open_success == 0 && (direntp = readdir(dirp)) != NULL) {
			if (strncmp(direntp->d_name, "video", 5) == 0) {
				snprintf(device, 261, "/dev/%s", direntp->d_name);
				fd = open(device, O_RDWR);
				if (fd < 0)
					continue;
				printf("Find device \"%s\". \n", device);
				ioctl(fd, VIDIOC_QUERYCAP, &capab);
				if ((capab.capabilities & V4L2_CAP_VIDEO_CAPTURE) == 0) {
					printf("Device \"%s\" is not a video capture device. \n", device);
					close(fd);
					continue;
				}
				for (fmtdesc.index = 0; ioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc) == 0; fmtdesc.index++) {
					if (fmtdesc.pixelformat == V4L2_PIX_FMT_YUYV) {
						open_success = 1;
						printf("Device \"%s\" is opened successfully. \n", device);
						break;
					}
				}
				if (open_success == 0) {
					printf("Device \"%s\" does not support MJPEG pixel format. \n", device);
					close(fd);
					continue;
				}
			}
		}
		closedir(dirp);
		if (open_success == 0) {
			fprintf(stderr, "Error: No available video capture device! \n");
			return -1;
		}
	}
	printf("\n");
	
	/*打印设备的驱动名字、设备名字、总线名字和版本信息*/
	printf("Device information: \n"
		   "driver  : %s \n"
		   "card    : %s \n"
		   "bus     : %s \n"
		   "version : %d \n", capab.driver, capab.card, capab.bus_info, capab.version);
	printf("\n");
	
	return 0;
}

/**
 * 描述：查看摄像头设备的参数（像素格式、分辨率、帧率）
 * 参数：无
 * 返回值：无
**/
void camera_view()
{
	struct v4l2_fmtdesc fmtdesc;
	struct v4l2_frmsizeenum frmsize;
	struct v4l2_frmivalenum frmival;
	
	/*列出摄像头所支持的所有像素格式以及描述信息*/
	fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	printf("Pixel formats supported by the device: \n");
	for (fmtdesc.index = 0; ioctl(fd, VIDIOC_ENUM_FMT, &fmtdesc) == 0; fmtdesc.index++)
		printf("0x%x (%s) \n", fmtdesc.pixelformat, fmtdesc.description);
	printf("\n");
	
	/*列出摄像头MJPG像素格式所支持的所有分辨率及其帧率*/
	frmsize.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	frmsize.pixel_format = V4L2_PIX_FMT_YUYV;
	frmival.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	frmival.pixel_format = V4L2_PIX_FMT_YUYV;
	printf("Frame sizes and their frame rates supported by the device's MJPG pixel format: \n");
	printf("%-11s  %-s \n", "Frame sizes", "Frame rates (fps)");
	for (frmsize.index = 0; ioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frmsize) == 0; frmsize.index++) {
		printf("%5d*%-5d  ", frmsize.discrete.width, frmsize.discrete.height);
		frmival.width = frmsize.discrete.width;
		frmival.height = frmsize.discrete.height;
		for (frmival.index = 0; ioctl(fd, VIDIOC_ENUM_FRAMEINTERVALS, &frmival) == 0; frmival.index++) {
			printf("%-3d  ", frmival.discrete.denominator / frmival.discrete.numerator);
		}
		printf("\n");
	}
	printf("\n");
	
	close(fd);
}

/**
 * 描述：摄像头设备初始化
 * 参数：width 图像宽度
 *       height 图像高度
 *       rate 帧率
 * 返回值：0 初始化成功
 *         -1 初始化失败
**/
int camera_init(int width, int height, int rate)
{
	int ret;
	struct v4l2_format format;
	struct v4l2_streamparm strmparm;
	
	/* 设置帧大小 */
	format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
	format.fmt.pix.width = width;
	format.fmt.pix.height = height;
	ret = ioctl(fd, VIDIOC_S_FMT, &format);
	if (ret < 0) {
		perror("Error: ioctl: VIDIOC_S_FMT");
		return -1;
	}
	
	/* 设置帧率 */
	strmparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	strmparm.parm.capture.timeperframe.numerator = 1;
	strmparm.parm.capture.timeperframe.denominator = rate;
	ret = ioctl(fd, VIDIOC_S_PARM, &strmparm);
	if (ret < 0)
		perror("Warning: ioctl: VIDIOC_S_PARM");
	
	/*显示使用的帧格式*/
	printf("Parameter selections: \n");
	ret = ioctl(fd, VIDIOC_G_FMT, &format);
	if (ret < 0)
		perror("Warning: ioctl: VIDIOC_G_FMT");
	printf("frame size : %d*%d \n", format.fmt.pix.width, format.fmt.pix.height);
	ret = ioctl(fd, VIDIOC_G_PARM, &strmparm);
	if (ret < 0)
		perror("Warning: ioctl: VIDIOC_G_PARM");
	printf("frame rate : %dfps \n", strmparm.parm.capture.timeperframe.denominator / strmparm.parm.capture.timeperframe.numerator);
	
	return 0;
}

/**
 * 描述：视频采集线程清理函数
 * 参数：globalp 全局变量结构体指针
 * 返回值：无
**/
void camera_cleanup(void *globalp)
{
	printf("The camera thread has exited or has been cancelled. \n");
	close(fd);
	for (buf.index = 0; buf.index < 4; buf.index++) {
		if (frm_mem[buf.index] != NULL) {
			ioctl(fd, VIDIOC_QUERYBUF, &buf);
			munmap(frm_mem[buf.index], buf.length);
		}
	}
	if (((global_t *)globalp)->frame != NULL)
		free(((global_t *)globalp)->frame);
	printf("The cleanup of camera thread has finished. \n");
}

/**
 * 描述：视频采集线程
 * 参数：globalp 全局变量结构体指针
 * 返回值：NULL
**/
void *camera_thread(void *globalp)
{
	int ret;
	sigset_t sigset;
	struct v4l2_requestbuffers reqbuf;
	enum v4l2_buf_type buftype = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	
	pthread_cleanup_push(camera_cleanup, globalp);
	
	/*屏蔽SIGINT信号*/
	sigemptyset(&sigset);
	sigaddset(&sigset, SIGINT);
	ret = pthread_sigmask(SIG_BLOCK, &sigset, NULL);
	if (ret < 0)
		perror("Warning: pthread_sigmask");
	
	/*申请帧缓冲*/
	reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	reqbuf.count = 4;
	reqbuf.memory = V4L2_MEMORY_MMAP;
	ret = ioctl(fd, VIDIOC_REQBUFS, &reqbuf);
	if (ret < 0) {
		perror("Error: ioctl: VIDIOC_REQBUFS");
		goto error;
	}
	
	/*建立内存映射*/
	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;
	for (buf.index = 0; buf.index < 4; buf.index++) {
		ioctl(fd, VIDIOC_QUERYBUF, &buf);
		frm_mem[buf.index] = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
		if (frm_mem[buf.index] == MAP_FAILED) {
			perror("Error: mmap");
			goto error;
		}
	}
	
	/*入队*/
	for (buf.index = 0; buf.index < 4; buf.index++) {
		ret = ioctl(fd, VIDIOC_QBUF, &buf);
		if (ret < 0) {
			perror("Error: ioctl: VIDIOC_QBUF");
			goto error;
		}
	}
	
	/*为帧数据分配内存*/
	((global_t *)globalp)->frame = (char *)malloc(buf.length);
	if (((global_t *)globalp)->frame == NULL) {
		fprintf(stderr, "Error: malloc() failed! \n");
		goto error;
	}
	
	/*开启视频采集*/
	ret = ioctl(fd, VIDIOC_STREAMON, &buftype);
	if (ret < 0) {
		perror("Error: ioctl: VIDIOC_STREAMON");
		goto error;
	}
	
	while (1) {
		/*出队*/
		memset(&buf, 0, sizeof(struct v4l2_buffer));
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		ret = ioctl(fd, VIDIOC_DQBUF, &buf);
		if(ret < 0) {
			perror("Error: ioctl: VIDIOC_DQBUF");
			goto error;
		}
		
		/*将图像复制到全局*/
		sem_wait(&((global_t *)globalp)->camera_sem);
		memcpy(((global_t *)globalp)->frame, frm_mem[buf.index], buf.bytesused);
		((global_t *)globalp)->frame_bytes = buf.bytesused;
		sem_post(&((global_t *)globalp)->net_sem);
		
		/*再次入队*/
		ret = ioctl(fd, VIDIOC_QBUF, &buf);
		if(ret < 0) {
			perror("Error: ioctl: VIDIOC_QBUF");
			goto error;
		}
	}
	
error:
	pthread_exit(NULL);
	pthread_cleanup_pop(0);
}

