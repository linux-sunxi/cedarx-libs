
#ifndef VDECODER_CONFIG
#define VDECODER_CONFIG

#include "libcedarv.h"

#define BITSTREAM_FRAME_NUM         		(2000)
#define BITSTREAM_BUF_SIZE_IN_PREVIEW_MODE	(4*1024*1024)
#define BITSTREAM_FRAME_NUM_IN_PREVIEW_MODE (32)
//#define MAX_SUPPORTED_VIDEO_WIDTH   		3840
//#define MAX_SUPPORTED_VIDEO_HEIGHT  		2160

#ifdef MELIS

#include "mod_cedar_i.h"
#include "vdec_2_vdrv.h"
#include "vdrv_common_cfg.h"
#define DECODE_FREE_RUN             0
#define DISPLAY_FREE_RUN            1

#else

//#include <cedarv_osal_linux.h>
#define DECODE_FREE_RUN             1
#define DISPLAY_FREE_RUN            1

#endif


#endif
