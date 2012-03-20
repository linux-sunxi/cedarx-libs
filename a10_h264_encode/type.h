

#ifndef __H264__ENCODE__LIB__TYPE__H__
#define __H264__ENCODE__LIB__TYPE__H__

#include "drv_display.h"

typedef struct VIDEO_ENCODE_FORMAT
{
    __u32       codec_type;         // video codec type, __herb_codec_t
    __s32		width;				//encoder frame width,����Ǻ���ȵ�����
	__s32		height;				//encoder frame height,����Ǻ��߶ȵ�����

    __u32       frame_rate;         // frame rate, ��ֵ�Ŵ�1000����
    __s32		color_format;       //__pixel_rgbfmt_t��__pixel_yuvfmt_t
	__s32		color_space;
    __s16		qp_max;				// 40
	__s16		qp_min;				// 20
    __s32       avg_bit_rate;       // average bit rate
//    __s32       max_bit_rate;       // maximum bit rate

	__s32		maxKeyInterval;			//keyframe Interval��ָ2���ؼ�֮֡���P֡���������
    //define private information for video decode
//    __u16       video_bs_src;       // video bitstream source
//    void        *private_inf;       // video bitstream private information pointer
//    __s16       priv_inf_len;       // video bitstream private information length

} __video_encode_format_t;

typedef enum
{
    BT601 = 0,
	BT709,
	YCC,
	VXYCC
}__cs_mode_t;

typedef enum __PIXEL_YUVFMT                         /* pixel format(yuv)                                            */
{
    PIXEL_YUV444 = 0x10,                            /* only used in scaler framebuffer                              */
    PIXEL_YUV422,
    PIXEL_YUV420,
    PIXEL_YUV411,
    PIXEL_CSIRGB,
    PIXEL_OTHERFMT
}__pixel_yuvfmt_t;

typedef enum
{
    M_ENCODE = 0,
    M_ISP_THUMB,
    M_ENCODE_ISP_THUMB,
    M_UNSUPPORT,
} __venc_output_mod_t;

typedef enum __COLOR_SPACE
{
#ifdef __CHIP_VERSION_F20
    COLOR_SPACE_BT601 = BT601,
    COLOR_SPACE_BT709 = BT709,
#else
    COLOR_SPACE_BT601 = DISP_BT601,
    COLOR_SPACE_BT709 = DISP_BT709,
#endif
    COLOR_SPACE_,
}__color_space_t;

typedef struct _media_file_inf_t
{
	// video
    __s32 nHeight;
	__s32 nWidth;
	__s32 uVideoFrmRate;
	__s32 create_time;
	__s32 maxKeyInterval;

	// audio
	int channels;
	int bits_per_sample;
	int frame_size;
	int sample_rate;
}_media_file_inf_t;

typedef struct V4L2BUF_t
{
	unsigned int	addrPhyY;		// physical Y address of this frame
	int 			index;			// DQUE id number
	__s64			timeStamp;		// time stamp of this frame
}V4L2BUF_t;

#endif	// __H264__ENCODE__LIB__TYPE__H__

