
#ifndef VDECODER_H
#define VDECODER_H

#include "libve.h"
#include "os_adapter.h"
#include "fbm.h"
#include "vbv.h"

#include "vdecoder_config.h"

#define VDECODER_TAG_INF_SIZE 240

typedef struct VIDEO_DECODER video_decoder_t;

struct VIDEO_DECODER
{
    cedarv_decoder_t icedarv;

    Handle        	ve;
    u32				fbm_num;
    Handle        	fbm;
    Handle			minor_fbm;
    Handle        	vbv;
    u32 * 			index;

    u32				disable_3d;
    u32				vbv_num;
    Handle			minor_vbv;
    vstream_data_t	cur_minor_stream_part;


    s8            	display_already_begin;
    s8            	status;
    s8            	mode_switched;

    vstream_info_t	stream_info;
    vconfig_t     	config_info;
    vstream_data_t	cur_stream_part;
    u32           	remainMemorySize;
    u32           	max_vbv_buffer_size;
    s64				sys_time;
    u8              decinfTag[VDECODER_TAG_INF_SIZE];
    u32				drop_b_frame;
    s32             demuxType;

};

video_decoder_t* vdecoder_init(s32* return_value);
s32              vdecoder_exit(video_decoder_t* p);

#endif

