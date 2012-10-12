#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

// #include <asm/types.h>
#include <linux/videodev2.h>
#include <time.h>
#include <linux/fb.h>

#include "type.h"
#include "drv_display.h"
#include "capture.h"

#define DEV_NAME	"/dev/video0"

typedef struct buffer
{
	void * start;
	size_t length;
}buffer;

int disphd;
unsigned int hlay;
int sel = 0;//which screen 0/1
__disp_layer_info_t layer_para;
__disp_video_fb_t video_fb;
__u32 arg[4];

static int 			fd 		= NULL;
struct buffer 		*buffers	= NULL;
static unsigned int	n_buffers	= 0;

#define CLEAR(x) memset (&(x), 0, sizeof (x))

#define DISP_PREVIEW
#ifdef DISP_PREVIEW

int disp_int(int w, int h)
{
	/*display start*/
    unsigned int preview_left, preview_top, preview_h, preview_w;

	preview_left = 80;
	preview_top = 0;
	preview_h = h;
	preview_w = w;

	printf("w: %d, h: %d, preview_left: %d, preview_top: %d, preview_h: %d, preview_w : %d\n",
		w, h, preview_left, preview_top, preview_h, preview_w);

	if((disphd = open("/dev/disp",O_RDWR)) == -1)
	{
		printf("open file /dev/disp fail. \n");
		return 0;
	}

    arg[0] = 0;
    ioctl(disphd, DISP_CMD_LCD_ON, (void*)arg);

    //layer0
    arg[0] = 0;
    arg[1] = DISP_LAYER_WORK_MODE_SCALER;
    hlay = ioctl(disphd, DISP_CMD_LAYER_REQUEST, (void*)arg);
    if(hlay == 0)
    {
        printf("request layer0 fail\n");
        return 0;
    }
	printf("video layer hdl:%d\n", hlay);

    layer_para.mode = DISP_LAYER_WORK_MODE_SCALER;
    layer_para.pipe = 0;
    layer_para.fb.addr[0]       = 0;//your Y address,modify this
    layer_para.fb.addr[1]       = 0; //your C address,modify this
    layer_para.fb.addr[2]       = 0;
    layer_para.fb.size.width    = w;
    layer_para.fb.size.height   = h;
    layer_para.fb.mode          = DISP_MOD_NON_MB_UV_COMBINED;	//DISP_MOD_NON_MB_PLANAR;
    layer_para.fb.format        = DISP_FORMAT_YUV420;			//DISP_FORMAT_YUV422;
    layer_para.fb.br_swap       = 0;
    layer_para.fb.seq           = DISP_SEQ_UVUV;
    layer_para.ck_enable        = 0;
    layer_para.alpha_en         = 1;
    layer_para.alpha_val        = 0xff;
    layer_para.src_win.x        = 0;
    layer_para.src_win.y        = 0;
    layer_para.src_win.width    = w;
    layer_para.src_win.height   = h;
    layer_para.scn_win.x        = preview_left;
    layer_para.scn_win.y        = preview_top;
    layer_para.scn_win.width    = preview_w;
    layer_para.scn_win.height   = preview_h;
	arg[0] = sel;
    arg[1] = hlay;
    arg[2] = (__u32)&layer_para;
    ioctl(disphd,DISP_CMD_LAYER_SET_PARA,(void*)arg);
#if 0
    arg[0] = sel;
    arg[1] = hlay;
    ioctl(disphd,DISP_CMD_LAYER_TOP,(void*)arg);
#endif
    arg[0] = sel;
    arg[1] = hlay;
    ioctl(disphd,DISP_CMD_LAYER_OPEN,(void*)arg);

	printf("MODE: %d, format: %d\n", layer_para.fb.mode, layer_para.fb.format);

#if 1
	int fb_fd;
	unsigned long fb_layer;
	void *addr = NULL;
	fb_fd = open("/dev/fb0", O_RDWR);

	if (ioctl(fb_fd, FBIOGET_LAYER_HDL, &fb_layer) == -1) {
		printf("get fb layer handel\n");
	}

#if 0
	addr = malloc(800*480*3);
	memset(addr, 0xff, 800*480*3);
	write(fb_fd, addr, 800*480*3);
	//memset(addr, 0x12, 800*480*3);
#else
	addr = malloc(w*h*3);
	memset(addr, 0xff, w*h*3);
	write(fb_fd, addr, w*h*3);
	//memset(addr, 0x12, 800*480*3);
#endif

	printf("fb_layer hdl: %ld\n", fb_layer);
	close(fb_fd);

	arg[0] = 0;
	arg[1] = fb_layer;
	ioctl(disphd, DISP_CMD_LAYER_BOTTOM, (void *)arg);
#endif

	return 0;
}

void disp_start(void)
{
	arg[0] = sel;
    arg[1] = hlay;
    ioctl(disphd, DISP_CMD_VIDEO_START,  (void*)arg);
}

void disp_stop(void)
{
	arg[0] = sel;
    arg[1] = hlay;
    ioctl(disphd, DISP_CMD_VIDEO_STOP,  (void*)arg);
}

int disp_set_addr(int w,int h,int *addr)
{
#if 0
	layer_para.fb.addr[0]       = *addr;//your Y address,modify this
    layer_para.fb.addr[1]       = *addr+w*h; //your C address,modify this
    layer_para.fb.addr[2]       = *addr+w*h*3/2;

    arg[0] = sel;
    arg[1] = hlay;
    arg[2] = (__u32)&layer_para;
    ioctl(disphd,DISP_CMD_LAYER_SET_PARA,(void*)arg);
#endif
	__disp_video_fb_t  fb_addr;
	memset(&fb_addr, 0, sizeof(__disp_video_fb_t));

	fb_addr.interlace       = 0;
	fb_addr.top_field_first = 0;
	fb_addr.frame_rate      = 25;
	fb_addr.addr[0] = *addr;
	fb_addr.addr[1] = *addr + w * h;
	fb_addr.addr[2] = *addr + w*h*3/2;
	fb_addr.id = 0;  //TODO

    arg[0] = sel;
    arg[1] = hlay;
    arg[2] = (__u32)&fb_addr;
    ioctl(disphd, DISP_CMD_VIDEO_SET_FB, (void*)arg);

	return 0;
}

void disp_quit()
{
	__u32 arg[4];
	arg[0] = 0;
    ioctl(disphd, DISP_CMD_LCD_OFF, (void*)arg);

    arg[0] = sel;
    arg[1] = hlay;
    ioctl(disphd, DISP_CMD_LAYER_CLOSE,  (void*)arg);

    arg[0] = sel;
    arg[1] = hlay;
    ioctl(disphd, DISP_CMD_LAYER_RELEASE,  (void*)arg);
    close (disphd);
}

#endif // DISP_PREVIEW



int InitCapture()
{
	struct v4l2_capability cap;
	struct v4l2_format fmt;
	unsigned int i;

	fd = open (DEV_NAME, O_RDWR /* required */ | O_NONBLOCK, 0);
	if (fd == 0)
	{
		printf("open %s failed\n", DEV_NAME);
		return -1;
	}

	ioctl (fd, VIDIOC_QUERYCAP, &cap);

	CLEAR (fmt);
	fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width       = 640; //320;
	fmt.fmt.pix.height      = 480; //240;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_NV12;	//V4L2_PIX_FMT_YUV422P;	//
	fmt.fmt.pix.field       = V4L2_FIELD_NONE;
	ioctl (fd, VIDIOC_S_FMT, &fmt);

	struct v4l2_requestbuffers req;
	CLEAR (req);
	req.count               = 4;
	req.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory              = V4L2_MEMORY_MMAP;

	ioctl (fd, VIDIOC_REQBUFS, &req);

	buffers = calloc (req.count, sizeof(struct buffer));

	for (n_buffers = 0; n_buffers < req.count; ++n_buffers)
	{
	   struct v4l2_buffer buf;
	   CLEAR (buf);
	   buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	   buf.memory      = V4L2_MEMORY_MMAP;
	   buf.index       = n_buffers;

	   if (-1 == ioctl (fd, VIDIOC_QUERYBUF, &buf))
			printf ("VIDIOC_QUERYBUF error\n");

	   buffers[n_buffers].length = buf.length;
	   buffers[n_buffers].start  = mmap (NULL /* start anywhere */,
								         buf.length,
								         PROT_READ | PROT_WRITE /* required */,
								         MAP_SHARED /* recommended */,
								         fd, buf.m.offset);

	   if (MAP_FAILED == buffers[n_buffers].start)
			printf ("mmap failed\n");
	}

	for (i = 0; i < n_buffers; ++i)
	{
	   struct v4l2_buffer buf;
	   CLEAR (buf);

	   buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	   buf.memory      = V4L2_MEMORY_MMAP;
	   buf.index       = i;

	   if (-1 == ioctl (fd, VIDIOC_QBUF, &buf))
		printf ("VIDIOC_QBUF failed\n");
	}
#ifdef DISP_PREVIEW
	disp_int(640, 480);
	disp_start();
#endif // DISP_PREVIEW

	return 0;
}

void DeInitCapture()
{
	int i;
	enum v4l2_buf_type type;

	printf("DeInitCapture");

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (-1 == ioctl (fd, VIDIOC_STREAMOFF, &type))
		printf ("VIDIOC_STREAMOFF failed\n");
	else
		printf ("VIDIOC_STREAMOFF ok\n");

	for (i = 0; i < n_buffers; ++i) {
		if (-1 == munmap (buffers[i].start, buffers[i].length)) {
			printf ("munmap error\n");
		}
	}

#ifdef DISP_PREVIEW
	disp_stop();
	disp_quit();
#endif

	if (fd != 0)
	{
		close (fd);
		fd = 0;
	}

	printf("V4L2 close****************************\n");
}

int StartStreaming()
{
    int ret = -1;
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	printf("V4L2Camera::v4l2StartStreaming\n");

    ret = ioctl (fd, VIDIOC_STREAMON, &type);
    if (ret < 0)
	{
        printf("StartStreaming: Unable to start capture: %s\n", strerror(errno));
        return ret;
    }

	printf("V4L2Camera::v4l2StartStreaming OK\n");
    return 0;
}

void ReleaseFrame(int buf_id)
{
	struct v4l2_buffer v4l2_buf;
	int ret;
	static int index = -1;

	memset(&v4l2_buf, 0, sizeof(struct v4l2_buffer));
	v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	v4l2_buf.memory = V4L2_MEMORY_MMAP;
	v4l2_buf.index = buf_id;		// buffer index

	if (index == v4l2_buf.index)
	{
		printf("v4l2 should not release the same buffer twice continuous: index : %d\n", index);
		// return ;
	}
	index = v4l2_buf.index;

	ret = ioctl(fd, VIDIOC_QBUF, &v4l2_buf);
	if (ret < 0) {
		printf("VIDIOC_QBUF failed, id: %d\n", v4l2_buf.index);
		return ;
	}

	printf("VIDIOC_QBUF id: %d\n", v4l2_buf.index);
}

int WaitCamerReady()
{
	fd_set fds;
	struct timeval tv;
	int r;

	FD_ZERO(&fds);
	FD_SET(fd, &fds);

	/* Timeout */
	tv.tv_sec  = 2;
	tv.tv_usec = 0;

	r = select(fd + 1, &fds, NULL, NULL, &tv);
	if (r == -1)
	{
		printf("select err\n");
		return -1;
	}
	else if (r == 0)
	{
		printf("select timeout\n");
		return -1;
	}

	return 0;
}


int GetPreviewFrame(V4L2BUF_t *pBuf)	// DQ buffer for preview or encoder
{
	int ret = -1;
	struct v4l2_buffer buf;

	ret = WaitCamerReady();
	if (ret != 0)
	{
		printf("wait time out\n");
		return __LINE__;
	}

	memset(&buf, 0, sizeof(struct v4l2_buffer));
    buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    /* DQ */
    ret = ioctl(fd, VIDIOC_DQBUF, &buf);
    if (ret < 0)
	{
        printf("GetPreviewFrame: VIDIOC_DQBUF Failed\n");
        return __LINE__;
    }

	pBuf->addrPhyY	= buf.m.offset;
	pBuf->index 	= buf.index;
	pBuf->timeStamp = (int64_t)((int64_t)buf.timestamp.tv_usec + (((int64_t)buf.timestamp.tv_sec) * 1000000));

	printf("VIDIOC_DQBUF id: %d\n", buf.index);

#ifdef DISP_PREVIEW
	disp_set_addr(640, 480, &buf.m.offset);
#endif

	return 0;
}



