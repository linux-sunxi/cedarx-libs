
#ifndef VDECODER_H
#define VDECODER_H

#include "libve.h"
#include "os_adapter.h"
#include "fbm.h"
#include "vbv.h"

#include "vdecoder_config.h"

typedef struct VIDEO_DECODER video_decoder_t;

struct VIDEO_DECODER
{
    cedarv_decoder_t icedarv;

    Handle        	ve;
    Handle        	fbm;
    Handle        	vbv;

    s8            	display_already_begin;
    u8            	last_frame_index;
    s8            	status;
    s8            	mode_switched;

    vstream_info_t	stream_info;
    vconfig_t     	config_info;
    vstream_data_t	cur_stream_part;
    u32           	remainMemorySize;
    u32           	max_vbv_buffer_size;

};

video_decoder_t* vdecoder_init(s32* return_value);
s32              vdecoder_exit(video_decoder_t* p);

#endif

