
#ifndef VDECODER_H
#define VDECODER_H

#include "libve.h"
#include "os_adapter.h"

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

    vpicture_t		rotate_img[3];//dy_rot
    u32				rotate_angle;//dy_rot, 0: no rotate; 1: 90 degree; 2: 180 degree; 3: 270 degree; 4: horizon flip; 5: vertical flip;
    u32				rotate_ve_reset;//dy_rot
    u32				dy_rotate_flag; //mb32 dynamic rotate flag, 0: not mb32 or mb32 need not rotate, 1:mb32 need rotate
    u8*				rot_img_y_v[3];
    u8*				rot_img_c_v[3];
    u32				rot_img_y[3];
    u32				rot_img_c[3];
    u32				cur_img_flag;

    u32				dy_rotate_on;   //dynamic rotate flag
	
	s32             config_vbv_size;

    s32             mRotateFlag;    //when detect rotate different frame, set to 1
    s32             mRotateIgnoreFrameNum;  //calculate ignore frame number after frame rotate_flag turn normal.

};

video_decoder_t* vdecoder_init(s32* return_value);
s32              vdecoder_exit(video_decoder_t* p);

#endif

