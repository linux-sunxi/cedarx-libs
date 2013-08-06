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

#include <asm/types.h>       
#include <linux/videodev.h>
#include <linux/videodev2.h>
#include <time.h>
#include <linux/fb.h>

#if 1
#include "encoder_type.h"
#else
#include "type.h"
#endif

#include "drv_display.h"
#include "capture.h"

#define LOG_NDEBUG 0

#ifndef __OS_LINUX
#include <utils/Log.h>
#endif

#define DEV_NAME	"/dev/video0"		
#define NB_BUFFER 5

#define CAMERA_DRIV_UPDATE

typedef struct buffer {
	void * start;
	size_t length;
} buffer;

int disphd;
unsigned int hlay;
int sel = 0;//which screen 0/1
__disp_layer_info_t layer_para;
__disp_video_fb_t video_fb;
__u32 arg[4];

static int mCamFd = NULL;
struct buffer *buffers = NULL;
static unsigned int n_buffers = 0;

int mLayer = 0;
int mDispHandle = 0;
int mScreenWidth = 0;
int mScreenHeight = 0;
int mFirstFrame = 1;
int mFrameId = 0;

static int mCaptureFormat = 0;
int buf_vir_addr[NB_BUFFER];
int buf_phy_addr[NB_BUFFER];

#define CLEAR(x) memset (&(x), 0, sizeof (x))

int tryFmt(int format) {
	struct v4l2_fmtdesc fmtdesc;
	int i;
	fmtdesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	for (i = 0; i < 12; i++) {
		fmtdesc.index = i;
		if (-1 == ioctl(mCamFd, VIDIOC_ENUM_FMT, &fmtdesc)) {
			break;
		}
		printf("format index = %d, name = %s, v4l2 pixel format = %x \n", i, fmtdesc.description,
				fmtdesc.pixelformat);

		if (fmtdesc.pixelformat == format) {
			return 0;
		}
	}

	return -1;
}

static void YUYVToNV12(const void* yuyv, void *nv12, int width, int height) {
	int i, j;
	uint8_t* Y = (uint8_t*) nv12;
	uint8_t* UV = (uint8_t*) Y + width * height;

	for (i = 0; i < height; i += 2) {
		for (j = 0; j < width; j++) {
			*(uint8_t*) ((uint8_t*) Y + i * width + j) = *(uint8_t*) ((uint8_t*) yuyv + i * width
					* 2 + j * 2);
			*(uint8_t*) ((uint8_t*) Y + (i + 1) * width + j) = *(uint8_t*) ((uint8_t*) yuyv + (i
					+ 1) * width * 2 + j * 2);
			*(uint8_t*) ((uint8_t*) UV + ((i * width) >> 1) + j) = *(uint8_t*) ((uint8_t*) yuyv + i
					* width * 2 + j * 2 + 1);
		}
	}
}

static void YUYVToNV21(const void* yuyv, void *nv21, int width, int height) {
	int i, j;
	uint8_t* Y = (uint8_t*) nv21;
	uint8_t* VU = (uint8_t*) Y + width * height;

	for (i = 0; i < height; i += 2) {
		for (j = 0; j < width; j++) {
			*(uint8_t*) ((uint8_t*) Y + i * width + j) = *(uint8_t*) ((uint8_t*) yuyv + i * width
					* 2 + j * 2);
			*(uint8_t*) ((uint8_t*) Y + (i + 1) * width + j) = *(uint8_t*) ((uint8_t*) yuyv + (i
					+ 1) * width * 2 + j * 2);

			if (j % 2) {
				if (j < width - 1) {
					*(uint8_t*) ((uint8_t*) VU + ((i * width) >> 1) + j)
							= *(uint8_t*) ((uint8_t*) yuyv + i * width * 2 + (j + 1) * 2 + 1);
				}
			} else {
				if (j > 1) {
					*(uint8_t*) ((uint8_t*) VU + ((i * width) >> 1) + j)
							= *(uint8_t*) ((uint8_t*) yuyv + i * width * 2 + (j - 1) * 2 + 1);
				}
			}
		}
	}
}

int dispInit() {
	int error = 0;

	mFirstFrame = 1;

	// open disp driver
	mDispHandle = open("/dev/disp", O_RDWR);
	if (mDispHandle <= 0) {
		printf("open display driver failed, %s", strerror(errno));
		goto ERROR;
	}
	printf("open display driver ok");

	// request a video layer
	unsigned long args[4];
	memset(&args, 0, sizeof(args));
	args[0] = 0;
	args[1] = DISP_LAYER_WORK_MODE_SCALER;
	mLayer = ioctl(mDispHandle, DISP_CMD_LAYER_REQUEST, (void*) args);
	if (mLayer == 0) {
		printf("request layer failed, %s", strerror(errno));
		goto ERROR;
	}
	printf("request layer ok");

	mScreenWidth = ioctl(mDispHandle, DISP_CMD_SCN_GET_WIDTH, args);
	mScreenHeight = ioctl(mDispHandle, DISP_CMD_SCN_GET_HEIGHT, args);
	printf("get screen width: %d, height: %d", mScreenWidth, mScreenHeight);

	// set layer information
	__disp_layer_info_t layer_para;
	memset(&args, 0, sizeof(args));
	args[0] = 0;
	args[1] = mLayer;
	args[2] = (unsigned long) (&layer_para);
	args[3] = 0;

	memset(&layer_para, 0, sizeof(layer_para));
	error = ioctl(mDispHandle, DISP_CMD_LAYER_GET_PARA, &args);
	if (error != 0) {
		printf("get layer information failed, %s", strerror(errno));
		goto ERROR;
	}
	printf("get layer information ok");

	layer_para.mode = DISP_LAYER_WORK_MODE_SCALER;
	layer_para.pipe = 0;
	layer_para.fb.addr[0] = 0;
	layer_para.fb.addr[1] = 0;
	layer_para.fb.addr[2] = 0;
	layer_para.fb.size.width = mVideoWidth;
	layer_para.fb.size.height = mVideoHeight;
	layer_para.fb.mode = DISP_MOD_NON_MB_UV_COMBINED;///DISP_MOD_INTERLEAVED;//DISP_MOD_NON_MB_PLANAR;//DISP_MOD_NON_MB_PLANAR;//DISP_MOD_NON_MB_UV_COMBINED;
	layer_para.fb.format = DISP_FORMAT_YUV420;//DISP_FORMAT_YUV420;//DISP_FORMAT_YUV422;//DISP_FORMAT_YUV420;
	layer_para.fb.br_swap = 0;
	layer_para.fb.seq = DISP_SEQ_UVUV;//DISP_SEQ_UVUV;//DISP_SEQ_YUYV;//DISP_SEQ_YVYU;//DISP_SEQ_UYVY;//DISP_SEQ_VYUY//DISP_SEQ_UVUV
	layer_para.ck_enable = 0;
	layer_para.alpha_en = 1;
	layer_para.alpha_val = 0xff;
	layer_para.src_win.x = 0;
	layer_para.src_win.y = 0;
	layer_para.src_win.width = mVideoWidth;
	layer_para.src_win.height = mVideoHeight;
	layer_para.scn_win.x = 0;
	layer_para.scn_win.y = 0;
	layer_para.scn_win.width = mScreenWidth;
	layer_para.scn_win.height = mScreenHeight;

	printf("video : %dx%d, scn: %dx%d", mVideoWidth, mVideoHeight, mScreenWidth, mScreenHeight);

	error = ioctl(mDispHandle, DISP_CMD_LAYER_SET_PARA, &args);
	if (error != 0) {
		printf("set layer information failed, %s", strerror(errno));
		goto ERROR;
	}

	printf("set video layer top");
	args[0] = 0;
	args[1] = mLayer;
	error = ioctl(mDispHandle, DISP_CMD_LAYER_TOP, (void *) args);
	if (error != 0) {
		printf("set layer top failed, %s", strerror(errno));
		goto ERROR;
	}

	ERROR: return error;
}

void dispDeInit() {
	unsigned int args[4];

	if (mLayer != 0) {
		args[0] = 0;
		args[1] = mLayer;
		args[2] = 0;
		args[3] = 0;
		ioctl(mDispHandle, DISP_CMD_VIDEO_STOP, (void*) args);

		printf("close video layer");
		ioctl(mDispHandle, DISP_CMD_LAYER_CLOSE, (void*) args);

		printf("release video layer");
		ioctl(mDispHandle, DISP_CMD_LAYER_RELEASE, (void*) args);
	}

	if (mDispHandle != 0) {
		close(mDispHandle);
		mDispHandle = 0;
	}

	mFirstFrame = 0;
}

int setLayerAddr(struct v4l2_buffer * buf) {
	unsigned int args[4];
	__disp_video_fb_t fb_addr;
	memset(&fb_addr, 0, sizeof(__disp_video_fb_t ));

	if (mFirstFrame) {
		printf("first frame");

		args[0] = 0;
		args[1] = (unsigned long) mLayer;
		args[2] = 0;
		args[3] = 0;
		ioctl(mDispHandle, DISP_CMD_VIDEO_START, args);

		fb_addr.interlace = 0;
		fb_addr.top_field_first = 0;
		fb_addr.frame_rate = 25;
		fb_addr.addr[0] = buf->m.offset;
		fb_addr.addr[1] = buf->m.offset + mVideoWidth * mVideoHeight;
		fb_addr.addr[2] = 0;
		fb_addr.id = mFrameId++;

		args[0] = 0;
		args[1] = mLayer;
		args[2] = (__u32) & fb_addr;
		ioctl(mDispHandle, DISP_CMD_VIDEO_SET_FB, (void*) args);

		GET_FRAME_ID: args[0] = 0;
		args[1] = mLayer;
		int tmpId = ioctl(mDispHandle, DISP_CMD_VIDEO_GET_FRAME_ID, (void*) args);
		if (tmpId != fb_addr.id) {
			printf("WAIT...., tmpId: %d, fb_addr.id: %d", tmpId, fb_addr.id);
			usleep(1000);
			goto GET_FRAME_ID;
		}

		args[0] = 0;
		args[1] = (unsigned long) mLayer;
		args[2] = 0;
		args[3] = 0;
		ioctl(mDispHandle, DISP_CMD_LAYER_OPEN, args);

		mFirstFrame = 0;
	} else {
		fb_addr.interlace = 0;
		fb_addr.top_field_first = 0;
		fb_addr.frame_rate = 25;
		fb_addr.addr[0] = buf->m.offset;
		fb_addr.addr[1] = buf->m.offset + mVideoWidth * mVideoHeight;
		fb_addr.addr[2] = 0;
		fb_addr.id = mFrameId++;

		args[0] = 0;
		args[1] = mLayer;
		args[2] = (__u32) & fb_addr;
		ioctl(mDispHandle, DISP_CMD_VIDEO_SET_FB, (void*) args);
	}

	return 0;
}

int InitCapture() {
	struct v4l2_capability cap;
	struct v4l2_format fmt;
	unsigned int i;

	mCamFd = open(DEV_NAME, O_RDWR /* required */| O_NONBLOCK, 0);
	if (mCamFd == 0) {
		printf("open %s failed\n", DEV_NAME);
		return -1;
	}

#ifdef CAMERA_DRIV_UPDATE

	struct v4l2_input inp;
	int ret;
	inp.index = 0;
	if (-1 == ioctl(mCamFd, VIDIOC_S_INPUT, &inp)) {
		printf("VIDIOC_S_INPUT error!\n");
		return -1;
	}

#endif	

	ret = ioctl(mCamFd, VIDIOC_QUERYCAP, &cap);
	if (ret < 0) {
		printf("Error opening device: unable to query device. \n");
		return -1;
	}

	if ((cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) == 0) {
		printf("Error opening device: video capture not supported. \n");
		return -1;
	}

	if ((cap.capabilities & V4L2_CAP_STREAMING) == 0) {
		printf("Capture device does not support streaming i/o  \n");
		return -1;
	}

	// try to support this format: NV21, YUYV
	// we do not support mjpeg camera now
	if (!tryFmt(V4L2_PIX_FMT_NV21)) {
		mCaptureFormat = V4L2_PIX_FMT_NV21;
		printf("capture format: V4L2_PIX_FMT_NV21  \n");
	} else if (!tryFmt(V4L2_PIX_FMT_NV12)) {
		mCaptureFormat = V4L2_PIX_FMT_NV12;
		printf("capture format: V4L2_PIX_FMT_NV12  \n");
	} else if (!tryFmt(V4L2_PIX_FMT_YUYV)) {
		mCaptureFormat = V4L2_PIX_FMT_YUYV; // maybe usb camera
		printf("capture format: V4L2_PIX_FMT_YUYV  \n");
	} else {
		printf("driver should surpport NV21/NV12 or YUYV format, but it not!  \n");
		return -1;
	}

	CLEAR (fmt);
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = mVideoWidth; //3
	fmt.fmt.pix.height = mVideoHeight;
	fmt.fmt.pix.pixelformat = mCaptureFormat; //V4L2_PIX_FMT_YUV422P;	//
	fmt.fmt.pix.field = V4L2_FIELD_NONE;
	ioctl(mCamFd, VIDIOC_S_FMT, &fmt);

	struct v4l2_requestbuffers req;
	CLEAR (req);
	req.count = 4;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;

	ioctl(mCamFd, VIDIOC_REQBUFS, &req);

	buffers = calloc(req.count, sizeof(struct buffer));

	for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
		struct v4l2_buffer buf;
		CLEAR (buf);
		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = n_buffers;

		if (-1 == ioctl(mCamFd, VIDIOC_QUERYBUF, &buf))
			printf("VIDIOC_QUERYBUF error\n");

		buffers[n_buffers].length = buf.length;
		buffers[n_buffers].start = mmap(NULL /* start anywhere */, buf.length,
				PROT_READ | PROT_WRITE /* required */, MAP_SHARED /* recommended */, mCamFd,
				buf.m.offset);

		if (MAP_FAILED == buffers[n_buffers].start)
			printf("mmap failed\n");
	}

	for (i = 0; i < n_buffers; ++i) {
		struct v4l2_buffer buf;
		CLEAR (buf);

		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;
		buf.index = i;

		if (-1 == ioctl(mCamFd, VIDIOC_QBUF, &buf)) {
			printf("VIDIOC_QBUF failed\n");
			return -1;
		}

		int buffer_len = mVideoWidth * mVideoHeight * 3 / 2;
		buf_vir_addr[i] = (int) cedara_phymalloc_map(buffer_len, 1024);
		buf_phy_addr[i] = cedarv_address_vir2phy((void*) buf_vir_addr[i]);
		buf_phy_addr[i] |= 0x40000000;
		printf("video buffer: index: %d, vir: %x, phy: %x, len: %x \n", i, buf_vir_addr[i],
				buf_phy_addr[i], buffer_len);

		memset((void*) buf_vir_addr[i], 0x10, mVideoWidth * mVideoHeight);
		memset((void*) buf_vir_addr[i] + mVideoWidth * mVideoHeight, 0x80,
				mVideoWidth * mVideoHeight / 2);
	}

	return 0;
}

void DeInitCapture() {
	int i;
	enum v4l2_buf_type type;

	printf("DeInitCapture");

	type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (-1 == ioctl(mCamFd, VIDIOC_STREAMOFF, &type))
		printf("VIDIOC_STREAMOFF failed\n");
	else
		printf("VIDIOC_STREAMOFF ok\n");

	for (i = 0; i < n_buffers; ++i) {
		if (-1 == munmap(buffers[i].start, buffers[i].length)) {
			printf("munmap error\n");
		}
		cedara_phyfree_map((void*) buf_vir_addr[i]);
		buf_phy_addr[i] = 0;
	}

	if (mCamFd != 0) {
		close(mCamFd);
		mCamFd = 0;
	}

	printf("V4L2 close****************************\n");
}

int StartStreaming() {
	int ret = -1;
	enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	printf("V4L2Camera::v4l2StartStreaming\n");

	ret = ioctl(mCamFd, VIDIOC_STREAMON, &type);
	if (ret < 0) {
		printf("StartStreaming: Unable to start capture: %s\n", strerror(errno));
		return ret;
	}

	printf("V4L2Camera::v4l2StartStreaming OK\n");
	return 0;
}

void ReleaseFrame(int buf_id) {
	struct v4l2_buffer v4l2_buf;
	int ret;
	static int index = -1;

	memset(&v4l2_buf, 0, sizeof(struct v4l2_buffer));
	v4l2_buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	v4l2_buf.memory = V4L2_MEMORY_MMAP;
	v4l2_buf.index = buf_id; // buffer index

	if (index == v4l2_buf.index) {
		printf("v4l2 should not release the same buffer twice continuous: index : %d\n", index);
		// return ;
	}
	index = v4l2_buf.index;

	ret = ioctl(mCamFd, VIDIOC_QBUF, &v4l2_buf);
	if (ret < 0) {
		printf("VIDIOC_QBUF failed, id: %d\n", v4l2_buf.index);
		return;
	}

}

int WaitCamerReady() {
	fd_set fds;
	struct timeval tv;
	int r;

	FD_ZERO(&fds);
	FD_SET(mCamFd, &fds);

	/* Timeout */
	tv.tv_sec = 2;
	tv.tv_usec = 0;

	r = select(mCamFd + 1, &fds, NULL, NULL, &tv);
	if (r == -1) {
		printf("select err\n");
		return -1;
	} else if (r == 0) {
		printf("select timeout\n");
		return -1;
	}

	return 0;
}

int GetPreviewFrame(V4L2BUF_t *pBuf) // DQ buffer for preview or encoder
{
	int ret = -1;
	struct v4l2_buffer buf;

	ret = WaitCamerReady();
	if (ret != 0) {
		printf("wait time out\n");
		return __LINE__;
	}

	memset(&buf, 0, sizeof(struct v4l2_buffer));
	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;

	/* DQ */
	ret = ioctl(mCamFd, VIDIOC_DQBUF, &buf);
	if (ret < 0) {
		printf("GetPreviewFrame: VIDIOC_DQBUF Failed\n");
		return __LINE__;
	}

	if (mCaptureFormat == V4L2_PIX_FMT_YUYV) {
		YUYVToNV12(buffers[buf.index].start, (void*) buf_vir_addr[buf.index], mVideoWidth,
				mVideoHeight);
	}

	V4L2BUF_t v4l2_buf;
	if (mCaptureFormat == V4L2_PIX_FMT_YUYV) {
		pBuf->addrPhyY = buf_phy_addr[buf.index];
		pBuf->addrVirY = buf_vir_addr[buf.index];
	} else {
		pBuf->addrPhyY = buf.m.offset;
		pBuf->addrVirY = (unsigned int) buffers[buf.index].start;
	}

	pBuf->index = buf.index;
	pBuf->timeStamp = (int64_t)(
			(int64_t) buf.timestamp.tv_usec + (((int64_t) buf.timestamp.tv_sec) * 1000000));

	//printf("VIDIOC_DQBUF id: %d\n", buf.index);

	return 0;
}

