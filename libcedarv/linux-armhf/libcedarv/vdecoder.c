
#include "vdecoder.h"
#include "awprintf.h"
static s32 vdecoder_open(cedarv_decoder_t* p);
static s32 vdecoder_close(cedarv_decoder_t* p);
static s32 vdecoder_decode(cedarv_decoder_t* p);
static s32 vdecoder_io_ctrl(cedarv_decoder_t* p, u32 cmd, u32 param);
static s32 vdecoder_request_write(cedarv_decoder_t*p, u32 require_size, u8** buf0, u32* size0, u8** buf1, u32* size1);
static s32 vdecoder_update_data(cedarv_decoder_t* p, cedarv_stream_data_info_t* data_info);
static s32 vdeocder_display_request(cedarv_decoder_t* p, cedarv_picture_t* picture);
static s32 vdecoder_display_release(cedarv_decoder_t* p, u32 frame_index);
static s32 vdecoder_set_video_bitstream_info(cedarv_decoder_t* p, cedarv_stream_info_t* info);
static s32 vdecoder_query_quality(cedarv_decoder_t* p, cedarv_quality_t* vq);

static void set_format(vstream_info_t* vstream_info, cedarv_stream_info_t* cedarv_stream_info);
static void vdecoder_set_vbv_vedec_memory(video_decoder_t* p);

extern void ve_init_clock(void);
extern void ve_release_clock(void);

extern s32 cedardev_init(void);
extern s32 cedardev_exit(void);


extern IVEControl_t gIVE;
extern IOS_t        gIOS;
extern IFBM_t       gIFBM;
extern IVBV_t       gIVBV;

//#define SAVE_DATA
#ifdef SAVE_DATA
static FILE * fpStream = NULL;
const char * file_path = "/mnt/video/tmp.dat";
#endif


void libcedarv_free_vbs_buffer_sem(void* vdecoder)
{
	video_decoder_t* p = (video_decoder_t*)vdecoder;
	if(p && p->icedarv.free_vbs_buffer_sem)
		p->icedarv.free_vbs_buffer_sem(p->icedarv.cedarx_cookie);
}
cedarv_decoder_t* libcedarv_init(s32* return_value)
{
    video_decoder_t* p;

    *return_value = CEDARV_RESULT_ERR_FAIL;

    p = (video_decoder_t*)mem_alloc(sizeof(video_decoder_t));
    if (p == NULL)
    {
        *return_value = CEDARV_RESULT_ERR_NO_MEMORY;
        return NULL;
    }
    mem_set(p, 0, sizeof(video_decoder_t));

    p->icedarv.open             = vdecoder_open;
    p->icedarv.close            = vdecoder_close;
    p->icedarv.decode           = vdecoder_decode;
    p->icedarv.ioctrl           = vdecoder_io_ctrl;
    p->icedarv.request_write    = vdecoder_request_write;
    p->icedarv.update_data      = vdecoder_update_data;
    p->icedarv.display_request  = vdeocder_display_request;
    p->icedarv.display_release  = vdecoder_display_release;
    p->icedarv.set_vstream_info = vdecoder_set_video_bitstream_info;
    p->icedarv.query_quality    = vdecoder_query_quality;

    p->config_info.max_video_width  = MAX_SUPPORTED_VIDEO_WIDTH;
    p->config_info.max_video_height = MAX_SUPPORTED_VIDEO_HEIGHT;

    cedardev_init();
	ve_init_clock();

    *return_value = CEDARV_RESULT_OK;

    return (cedarv_decoder_t*)p;
}

s32 libcedarv_exit(cedarv_decoder_t* p)
{
    video_decoder_t* decoder;

    if(p == NULL)
        return CEDARV_RESULT_ERR_INVALID_PARAM;

    decoder = (video_decoder_t*)p;

    if(decoder->stream_info.init_data != NULL)
    {
        mem_free(decoder->stream_info.init_data);
        decoder->stream_info.init_data = NULL;
    }

    if(decoder->vbv != NULL)
    {
        vbv_release(decoder->vbv);
        decoder->vbv = NULL;
    }

    if(decoder->ve != NULL)
    {
        libve_close(0, decoder->ve);
        decoder->ve = NULL;
        decoder->fbm = NULL;
    }

    mem_free(decoder);

	ve_release_clock();
	cedardev_exit();
    return CEDARV_RESULT_OK;
}


static s32 vdecoder_open(cedarv_decoder_t* p)
{
	video_decoder_t* decoder;
    if(p == NULL)
        return CEDARV_RESULT_ERR_INVALID_PARAM;
    Log("xxxxxxxxxxxxxxxxxx");
    decoder = (video_decoder_t*)p;
    if(decoder->stream_info.format == STREAM_FORMAT_UNKNOW)
        return CEDARV_RESULT_ERR_UNSUPPORTED;

    if(decoder->status == CEDARV_STATUS_PREVIEW)
    {
    	decoder->vbv = vbv_init(BITSTREAM_BUF_SIZE_IN_PREVIEW_MODE, BITSTREAM_FRAME_NUM_IN_PREVIEW_MODE);
    }
    else
    {
    	vdecoder_set_vbv_vedec_memory(decoder);
    	decoder->vbv = vbv_init(decoder->max_vbv_buffer_size, BITSTREAM_FRAME_NUM);
    }

    if(decoder->vbv == NULL)
    {
        return CEDARV_RESULT_ERR_NO_MEMORY;
    }

    //* set method pointers first.
    libve_set_ive(&gIVE);
    libve_set_ios(&gIOS);
    libve_set_ifbm(&gIFBM);
    libve_set_ivbv(&gIVBV);

    decoder->ve = libve_open(&decoder->config_info, &decoder->stream_info, (void*)decoder);
    if(decoder->ve == NULL)
    {
        vbv_release(decoder->vbv);
        decoder->vbv = NULL;
        return CEDARV_RESULT_ERR_FAIL;
    }
    libve_set_vbv(decoder->vbv, decoder->ve);
    decoder->fbm = libve_get_fbm(decoder->ve);
#ifdef SAVE_DATA
    fpStream = fopen(file_path, "wb");
#endif

    return CEDARV_RESULT_OK;
}


static s32 vdecoder_close(cedarv_decoder_t* p)
{
    video_decoder_t* decoder;

    if(p == NULL)
        return CEDARV_RESULT_ERR_INVALID_PARAM;

    decoder = (video_decoder_t*)p;

    libve_reset(0, decoder->ve);

    if(decoder->stream_info.init_data != NULL)
    {
        mem_free(decoder->stream_info.init_data);
        decoder->stream_info.init_data = NULL;
    }

    if(decoder->ve != NULL)
    {
        libve_close(0, decoder->ve);
        decoder->ve = NULL;
        decoder->fbm = NULL;
    }

    if(decoder->vbv != NULL)
    {
        vbv_release(decoder->vbv);
        decoder->vbv = NULL;
    }
#ifdef SAVE_DATA
    if(fpStream)
    {
    	fclose(fpStream);
    	fpStream = NULL;
    }
#endif
    return CEDARV_RESULT_OK;
}


static s32 vdecoder_decode(cedarv_decoder_t* p)
{
    vresult_e        vresult;

#if (DECODE_FREE_RUN == 0)
    u64              video_time;
#endif

    video_decoder_t* decoder;

    if(p == NULL)
        return CEDARV_RESULT_ERR_INVALID_PARAM;

    decoder = (video_decoder_t*)p;
    if(decoder->mode_switched)
    {
    	libve_reset(0, decoder->ve);

        vbv_reset(decoder->vbv);
        if(p->free_vbs_buffer_sem != NULL)
            p->free_vbs_buffer_sem(p->cedarx_cookie);

        if(decoder->fbm != NULL)
        {
			fbm_flush(decoder->fbm);
            if(p->release_frame_buffer_sem != NULL)
                p->release_frame_buffer_sem(p->cedarx_cookie);
        }

    	decoder->mode_switched = 0;

    	return CEDARV_RESULT_NO_BITSTREAM;
    }
    if(decoder->status == CEDARV_STATUS_BACKWARD || decoder->status == CEDARV_STATUS_FORWARD)
    {
    	vresult = libve_decode(1, 0, 0, decoder->ve);
    }
    else
    {
    	if(decoder->stream_info.format == STREAM_FORMAT_H264 && decoder->display_already_begin == 0)
    	{
    		//* for H264, when decoding the first data unit containing PPS, we have to promise
    		//* there is more than one bitstream frame for decoding.
    		//* that is because the decoder uses the start code of the second bitstream frame
    		//* to judge PPS end.
    		if(vbv_get_stream_num(decoder->vbv) < 2)
    			return CEDARV_RESULT_NO_BITSTREAM;
    	}

    	if(decoder->status == CEDARV_STATUS_PREVIEW)
    	{
    		vresult = libve_decode(1, 0, 0, decoder->ve);
    	}
    	else
    	{
#if (DECODE_FREE_RUN == 0)
    	video_time = esMODS_MIoctrl(vdrv_com->avsync, DRV_AVS_CMD_GET_VID_TIME, DRV_AVS_TIME_TOTAL, 0);
    	video_time *= 1000;
    	vresult = libve_decode(0, 1, video_time, decoder->ve);
#else
    	vresult = libve_decode(0, 0, 0, decoder->ve);
#endif
    	}
    }

    if(vresult == VRESULT_OK)
    {
        return CEDARV_RESULT_OK;
    }
    else if(vresult == VRESULT_FRAME_DECODED)
    {
    	return CEDARV_RESULT_FRAME_DECODED;
    }
    else if(vresult == VRESULT_KEYFRAME_DECODED)
    {
    	if(decoder->status == CEDARV_STATUS_BACKWARD || decoder->status == CEDARV_STATUS_FORWARD || decoder->status == CEDARV_STATUS_PREVIEW)
    	{
    		libve_reset(1, decoder->ve);
    	}
        return CEDARV_RESULT_KEYFRAME_DECODED;
    }
    else if(vresult == VRESULT_NO_FRAME_BUFFER)
    {
        return CEDARV_RESULT_NO_FRAME_BUFFER;
    }
    else if(vresult == VRESULT_NO_BITSTREAM)
    {
        return CEDARV_RESULT_NO_BITSTREAM;
    }
    else if(vresult == VRESULT_ERR_NO_MEMORY)
    {
        return CEDARV_RESULT_ERR_NO_MEMORY;
    }
    else if( vresult == VRESULT_ERR_UNSUPPORTED)
    {
        return CEDARV_RESULT_ERR_UNSUPPORTED;
    }
    else if(vresult == VRESULT_ERR_LIBRARY_NOT_OPEN)
    {
        return CEDARV_RESULT_ERR_FAIL;
    }
    else
    {
        return CEDARV_RESULT_OK;
    }
}

static s32 vdecoder_io_ctrl(cedarv_decoder_t* p, u32 cmd, u32 param)
{
    video_decoder_t* decoder;

    if(p == NULL)
        return CEDARV_RESULT_ERR_INVALID_PARAM;

    decoder = (video_decoder_t*)p;

    if(p == NULL)
        return CEDARV_RESULT_ERR_INVALID_PARAM;

    switch(cmd)
    {
        case CEDARV_COMMAND_PLAY:
        {
            if(decoder->status != CEDARV_STATUS_PLAY && decoder->status != CEDARV_STATUS_STOP)
                decoder->mode_switched = 1;

            decoder->status = CEDARV_STATUS_PLAY;

            return CEDARV_RESULT_OK;
        }

        case CEDARV_COMMAND_FORWARD:
        {
            if(decoder->status != CEDARV_STATUS_PLAY)
                return CEDARV_RESULT_ERR_FAIL;

            decoder->status = CEDARV_STATUS_FORWARD;
            decoder->mode_switched = 1;

            return CEDARV_RESULT_OK;
        }

        case CEDARV_COMMAND_BACKWARD:
        {
            if(decoder->status != CEDARV_STATUS_PLAY)
                return CEDARV_RESULT_ERR_FAIL;

            decoder->status = CEDARV_STATUS_BACKWARD;
            decoder->mode_switched = 1;

            return CEDARV_RESULT_OK;
        }

        case CEDARV_COMMAND_STOP:
        {
            if(decoder->status != CEDARV_STATUS_PLAY && decoder->status != CEDARV_STATUS_FORWARD && decoder->status != CEDARV_STATUS_BACKWARD)
                return CEDARV_RESULT_ERR_FAIL;

            decoder->status = CEDARV_STATUS_STOP;
            decoder->mode_switched = 1;

            return CEDARV_RESULT_OK;
        }

        case CEDARV_COMMAND_ROTATE:
        {
            if(param > 5)
                return CEDARV_RESULT_ERR_FAIL;

            decoder->config_info.rotate_angle  = param;
            if(param != 0)
                decoder->config_info.rotate_enable = 1;

            return CEDARV_RESULT_OK;
        }

        case CEDARV_COMMAND_JUMP:
        {
        	libve_reset(0, decoder->ve);

			mem_set(&decoder->cur_stream_part, 0, sizeof(vstream_data_t));
            vbv_reset(decoder->vbv);
            if(p->free_vbs_buffer_sem != NULL)
                p->free_vbs_buffer_sem(p->cedarx_cookie);

            if(decoder->fbm != NULL)
			{
				fbm_flush(decoder->fbm);

                if(p->release_frame_buffer_sem != NULL)
                    p->release_frame_buffer_sem(p->cedarx_cookie);
            }

        	return CEDARV_RESULT_OK;
        }

//        case VDEC_QUERY_VBSBUFFER_USAGE_CMD:
//        {
//            s32 usage1, usage2;
//
//            if(p->vbv != NULL && vbv_get_buffer_size(p->vbv) != 0)
//            {
//                usage1 = vbv_get_valid_data_size(p->vbv) * 100 / vbv_get_buffer_size(p->vbv);
//                usage2 = vbv_get_valid_frame_num(p->vbv) * 100 / vbv_get_max_stream_frame_num(p->vbv);
//
//                return (usage1 > usage2) ? usage1 : usage2;
//            }
//
//            return 0;
//        }

        case CEDARV_COMMAND_SET_TOTALMEMSIZE:
        {
            decoder->remainMemorySize = param;
            return CEDARV_RESULT_OK;
        }


		case CEDARV_COMMAND_PREVIEW_MODE:
		{
			decoder->status = CEDARV_STATUS_PREVIEW;
			return CEDARV_RESULT_OK;
		}

        default:
            return CEDARV_RESULT_ERR_FAIL;
    }
}


static s32 vdecoder_request_write(cedarv_decoder_t*p, u32 require_size, u8** buf0, u32* size0, u8** buf1, u32* size1)
{
	u8*              start = NULL;
	u32              free_size = 0;
	u8*              vbv_end = NULL;
	video_decoder_t* decoder = NULL;

    if(p == NULL)
        return CEDARV_RESULT_ERR_INVALID_PARAM;

    decoder = (video_decoder_t*)p;

    if(p == NULL || buf0 == NULL || buf1 == NULL || size0 == NULL || size1 == NULL)
        return CEDARV_RESULT_ERR_INVALID_PARAM;

    *buf0  = NULL;
    *size0 = 0;
    *buf1  = NULL;
    *size1 = 0;

    if(decoder->vbv == NULL)
    {
        return CEDARV_RESULT_ERR_FAIL;
    }

    //* do not receive bitstream when decoding not started.
    if(decoder->status == CEDARV_STATUS_STOP)
    {
        return CEDARV_RESULT_ERR_FAIL;
    }

    //* do not receive bitstream when swithing play mode.
   // if(decoder->mode_switched != 0)
   //{
   //    return CEDARV_RESULT_ERR_FAIL;
   // }

    //* only receive one frame of bitstream when not in normal play mode.
    if(decoder->status == CEDARV_STATUS_FORWARD || decoder->status == CEDARV_STATUS_BACKWARD)
    {
        if(vbv_get_stream_num(decoder->vbv) > 1)
        {
            return CEDARV_RESULT_ERR_FAIL;
        }
    }

	if(require_size == 0)
	{
		require_size += 4;
	}

    if(vbv_request_buffer(&start, &free_size, require_size + decoder->cur_stream_part.length, decoder->vbv) != 0)
    {
        return CEDARV_RESULT_ERR_FAIL;
    }

    if(free_size <= decoder->cur_stream_part.length)
    {
        return CEDARV_RESULT_ERR_FAIL;
    }

    vbv_end  = vbv_get_base_addr(decoder->vbv) + vbv_get_buffer_size(decoder->vbv);
    start    = start + decoder->cur_stream_part.length;

    if(start >= vbv_end)
    {
        start -= vbv_get_buffer_size(decoder->vbv);
    }

    free_size -= decoder->cur_stream_part.length;

#if 1
	//* give ring buffer.
    if((start + free_size) > vbv_end)
    {
    	*buf0  = start;
    	*size0 = vbv_end - start;
    	*buf1  = vbv_get_base_addr(decoder->vbv);
    	*size1 = free_size - *size0;
    }
    else
    {
    	*buf0  = start;
    	*size0 = free_size;
    }
#else
	//* do not give ring buffer.
    if((start + free_size) > vbv_end)
    {
    	free_size = vbv_end - start;
    }

    *buf0 = start;
    *size0 = free_size;
#endif

    return 0;
}


static s32 vdecoder_update_data(cedarv_decoder_t* p, cedarv_stream_data_info_t* data_info)
{
	Handle           vbv = NULL;
	vstream_data_t*  stream= NULL;
	video_decoder_t* decoder = NULL;

    if(p == NULL)
        return CEDARV_RESULT_ERR_INVALID_PARAM;

    decoder = (video_decoder_t*)p;

    if(data_info == NULL)
        return CEDARV_RESULT_ERR_INVALID_PARAM;

    vbv    = decoder->vbv;
    stream = &decoder->cur_stream_part;

    if(data_info->flags & CEDARV_FLAG_FIRST_PART)
    {
        if(stream->length != 0)
        {
            stream->valid = 0;
            vbv_add_stream(stream, vbv);
        }

        stream->data   = vbv_get_current_write_addr(vbv);
        stream->length = data_info->lengh;
        if(data_info->flags & CEDARV_FLAG_PTS_VALID)
        {
            stream->pts = data_info->pts;
        }
        else
        {
            stream->pts = (u64)(-1);
        }
    }
    else
    {
        stream->length += data_info->lengh;
    }

    if(data_info->flags & CEDARV_FLAG_LAST_PART)
    {
    	if(data_info->flags & CEDARV_FLAG_DATA_INVALID)
    	{
        	stream->valid = 0;
        }
        else
        {
        	stream->valid = 1;
        }
        vbv_add_stream(stream, vbv);
#ifdef SAVE_DATA
        if(fpStream)
        {
			u8* vbv_end;
			u32 tmp_length;
			printf("writing data.........., stream length %x\n", stream->length);
			vbv_end  = vbv_get_base_addr(decoder->vbv) + vbv_get_buffer_size(decoder->vbv);
			if(stream->data + stream->length > vbv_end)
			{
				tmp_length = vbv_end - stream->data;
				fwrite(stream->data, 1, tmp_length, fpStream);
				fwrite(vbv_get_base_addr(decoder->vbv), 1, stream->length - tmp_length, fpStream);
			}
			else
			{
				fwrite(stream->data, 1, stream->length, fpStream);
			}
        }
#endif
    	stream->data   = NULL;
        stream->length = 0;
        stream->pts    = 0;
        stream->valid  = 0;
    }

    return 0;
}

static s32 vdeocder_display_request(cedarv_decoder_t* p, cedarv_picture_t* picture)
{
#if (DISPLAY_FREE_RUN == 0)
    u32              video_time;
#endif

	vpicture_t*      frame = NULL;
	video_decoder_t* decoder = NULL;

    if(p == NULL)
        return CEDARV_RESULT_ERR_INVALID_PARAM;

    decoder = (video_decoder_t*)p;

    if(decoder->fbm == NULL)
    {
        if(decoder->ve == NULL)
            return CEDARV_RESULT_ERR_FAIL;

        decoder->fbm = libve_get_fbm(decoder->ve);

        if(decoder->fbm == NULL)
            return CEDARV_RESULT_ERR_FAIL;
    }

    if(decoder->status == CEDARV_STATUS_PREVIEW)
    	frame = fbm_display_request_frame(decoder->fbm);
    else
    {
#if (DISPLAY_FREE_RUN == 1)
	    frame = fbm_display_request_frame(decoder->fbm);
#else
	    if(decoder->display_already_begin == 1)
	    {
get_frame_again:
	        frame = fbm_display_pick_frame(decoder->fbm);
	        if(frame != NULL)
	        {
	            if(frame->pts != (u64)(-1))
	            {
	                video_time = esMODS_MIoctrl(vdrv_com->avsync, DRV_AVS_CMD_GET_VID_TIME, DRV_AVS_TIME_TOTAL, 0); //* get system time.

	                if(decoder->status == CEDARV_STATUS_BACKWARD)
	                {
	                    if(video_time > (frame->pts/1000) + DISPLAY_TIME_WINDOW_WIDTH)
	                        return -1;  //* too early to display.
	                }
	                else
	                {
	                    if(video_time + DISPLAY_TIME_WINDOW_WIDTH < (frame->pts/1000))
	                        return -1;  //* too early to display.
	                }

	                frame = fbm_display_request_frame(decoder->fbm);

	                if(decoder->status == CEDARV_STATUS_PLAY)
	                {
	                	if((frame->pts/1000) + DISPLAY_TIME_WINDOW_WIDTH < video_time)
	                	{
	                    	if(fbm_display_pick_frame(decoder->fbm) != NULL)
	                    	{
	                        	fbm_display_return_frame(frame, decoder->fbm);
	                        	if(p->release_frame_buffer_sem != NULL)
	                            	p->release_frame_buffer_sem(p->cedarx_cookie);
	                        	goto get_frame_again;
	                    	}
	                    }
	                }
	            }
	            else
	            {
	                frame = fbm_display_request_frame(decoder->fbm);
	            }
	        }
	    }
	    else
	    {
	        frame = fbm_display_request_frame(decoder->fbm);
	    }
#endif
    }


    if(frame == NULL)
        return CEDARV_RESULT_ERR_FAIL;

    picture->id                     = frame->id;
    picture->width                  = frame->width;
    picture->height                 = frame->height;
    picture->top_offset				= frame->top_offset;
    picture->left_offset			= frame->left_offset;
    picture->display_width			= frame->display_width;
    picture->display_height			= frame->display_height;
    picture->rotate_angle           = frame->rotate_angle;
    picture->horizontal_scale_ratio = frame->horizontal_scale_ratio;
    picture->vertical_scale_ratio   = frame->vertical_scale_ratio;
    picture->store_width            = frame->store_width;
    picture->store_height           = frame->store_height;
    picture->frame_rate             = frame->frame_rate;
    picture->aspect_ratio           = frame->aspect_ratio;
    picture->is_progressive         = frame->is_progressive;
    picture->top_field_first        = frame->top_field_first;
    picture->repeat_top_field       = frame->repeat_top_field;
    picture->repeat_bottom_field    = frame->repeat_bottom_field;
    picture->size_y					= frame->size_y;
    picture->size_u					= frame->size_u;
    picture->size_v					= frame->size_v;

    if(frame->pixel_format == PIXEL_FORMAT_AW_YUV422)
    	picture->pixel_format = CEDARV_PIXEL_FORMAT_AW_YUV422;
    else if(frame->pixel_format == PIXEL_FORMAT_AW_YUV411)
    	picture->pixel_format = CEDARV_PIXEL_FORMAT_AW_YUV411;
    else
    	picture->pixel_format = CEDARV_PIXEL_FORMAT_AW_YUV420;

    picture->pts                    = frame->pts;
    picture->pcr                    = frame->pcr;

	//* the CedarX player need we return physic address of display frame.
#if 0
    picture->y                      = (unsigned char*)mem_get_phy_addr((unsigned int)frame->y);
    picture->u                      = (unsigned char*)mem_get_phy_addr((unsigned int)frame->u);
#else
    picture->y                      = (unsigned char*)frame->y;
    picture->u                      = (unsigned char*)frame->u;
#endif

    picture->v                      = NULL;
    picture->alpha                  = NULL;

    if(decoder->display_already_begin == 0)
        decoder->display_already_begin = 1;

    return CEDARV_RESULT_OK;
}


static s32 vdecoder_display_release(cedarv_decoder_t* p, u32 frame_index)
{
    vpicture_t*      frame;
    video_decoder_t* decoder;

    if(p == NULL)
        return CEDARV_RESULT_ERR_INVALID_PARAM;

    decoder = (video_decoder_t*)p;

    if(decoder == NULL || decoder->fbm == NULL)
        return CEDARV_RESULT_ERR_FAIL;

    frame = fbm_index_to_pointer(frame_index, decoder->fbm);
    if(frame == NULL)
        return CEDARV_RESULT_ERR_FAIL;

    fbm_display_return_frame(frame, decoder->fbm);
    if(p->release_frame_buffer_sem != NULL)
        p->release_frame_buffer_sem(p->cedarx_cookie);

    return CEDARV_RESULT_OK;
}


static s32 vdecoder_set_video_bitstream_info(cedarv_decoder_t* p, cedarv_stream_info_t* info)
{
    video_decoder_t*      decoder;

    decoder = (video_decoder_t*)p;

    if(p == NULL || info == NULL)
    {
        return CEDARV_RESULT_ERR_INVALID_PARAM;
    }


    set_format(&decoder->stream_info, info);

    decoder->stream_info.video_width      = info->video_width;
    decoder->stream_info.video_height     = info->video_height;
    decoder->stream_info.frame_rate       = info->frame_rate;
    decoder->stream_info.frame_duration   = info->frame_duration;
    decoder->stream_info.aspec_ratio      = info->aspect_ratio;
    decoder->stream_info.init_data_len    = info->init_data_len;


    if(info->init_data_len > 0)
    {
    	decoder->stream_info.init_data = (u8*)mem_alloc(decoder->stream_info.init_data_len);
    	if(decoder->stream_info.init_data == NULL)
    	{
        	return CEDARV_RESULT_ERR_NO_MEMORY;
        }

    	mem_cpy(decoder->stream_info.init_data, info->init_data, decoder->stream_info.init_data_len);
    }
    else
    	decoder->stream_info.init_data = NULL;

    return CEDARV_RESULT_OK;
}


static s32 vdecoder_query_quality(cedarv_decoder_t* p, cedarv_quality_t* vq)
{
    video_decoder_t*      decoder;

    decoder = (video_decoder_t*)p;

    if(p == NULL || vq == NULL)
        return CEDARV_RESULT_ERR_INVALID_PARAM;

	vq->vbv_buffer_usage = vbv_get_valid_data_size(decoder->vbv) * 100 / vbv_get_buffer_size(decoder->vbv);
    vq->frame_num_in_vbv = vbv_get_valid_frame_num(decoder->vbv);

	return CEDARV_RESULT_OK;
}


static void set_format(vstream_info_t* vstream_info, cedarv_stream_info_t* cedarv_stream_info)
{
	if(cedarv_stream_info->format == CEDARV_STREAM_FORMAT_MPEG2)
	{
		vstream_info->format = STREAM_FORMAT_MPEG2;
	}
	else if(cedarv_stream_info->format == CEDARV_STREAM_FORMAT_MPEG4)
	{
		vstream_info->format = STREAM_FORMAT_MPEG4;
	}
	else if(cedarv_stream_info->format == CEDARV_STREAM_FORMAT_REALVIDEO)
	{
		vstream_info->format = STREAM_FORMAT_REALVIDEO;
	}
	else if(cedarv_stream_info->format == CEDARV_STREAM_FORMAT_H264)
	{
		vstream_info->format = STREAM_FORMAT_H264;
	}
	else if(cedarv_stream_info->format == CEDARV_STREAM_FORMAT_VC1)
	{
		vstream_info->format = STREAM_FORMAT_VC1;
	}
	else if(cedarv_stream_info->format == CEDARV_STREAM_FORMAT_AVS)
	{
		vstream_info->format = STREAM_FORMAT_AVS;
	}
	else if(cedarv_stream_info->format == CEDARV_STREAM_FORMAT_MJPEG)
	{
		vstream_info->format = STREAM_FORMAT_MJPEG;
	}
    else if(cedarv_stream_info->format == CEDARV_STREAM_FORMAT_VP8)
    {
        vstream_info->format = STREAM_FORMAT_VP8;
    }
	else
	{
		vstream_info->format = STREAM_FORMAT_UNKNOW;
	}


	if(cedarv_stream_info->sub_format == CEDARV_SUB_FORMAT_UNKNOW)
	{
		vstream_info->sub_format = STREAM_SUB_FORMAT_UNKNOW;
	}
	else if(cedarv_stream_info->sub_format == CEDARV_MPEG2_SUB_FORMAT_MPEG1)
	{
		vstream_info->sub_format = MPEG2_SUB_FORMAT_MPEG1;
	}
	else if(cedarv_stream_info->sub_format == CEDARV_MPEG2_SUB_FORMAT_MPEG2)
	{
		vstream_info->sub_format = MPEG2_SUB_FORMAT_MPEG2;
	}
	else if(cedarv_stream_info->sub_format == CEDARV_MPEG4_SUB_FORMAT_XVID)
	{
		vstream_info->sub_format = MPEG4_SUB_FORMAT_XVID;
	}
	else if(cedarv_stream_info->sub_format == CEDARV_MPEG4_SUB_FORMAT_DIVX3)
	{
		vstream_info->sub_format = MPEG4_SUB_FORMAT_DIVX3;
	}
	else if(cedarv_stream_info->sub_format == CEDARV_MPEG4_SUB_FORMAT_DIVX4)
	{
		vstream_info->sub_format = MPEG4_SUB_FORMAT_DIVX4;
	}
	else if(cedarv_stream_info->sub_format == CEDARV_MPEG4_SUB_FORMAT_DIVX5)
	{
		vstream_info->sub_format = MPEG4_SUB_FORMAT_DIVX5;
	}
	else if(cedarv_stream_info->sub_format == CEDARV_MPEG4_SUB_FORMAT_SORENSSON_H263)
	{
		vstream_info->sub_format = MPEG4_SUB_FORMAT_SORENSSON_H263;
	}
	else if(cedarv_stream_info->sub_format == CEDARV_MPEG4_SUB_FORMAT_H263)
	{
		vstream_info->sub_format = MPEG4_SUB_FORMAT_H263;
	}
	else if(cedarv_stream_info->sub_format == CEDARV_MPEG4_SUB_FORMAT_RMG2)
	{
		vstream_info->sub_format = MPEG4_SUB_FORMAT_RMG2;
	}
	else if(cedarv_stream_info->sub_format == CEDARV_MPEG4_SUB_FORMAT_VP6)
	{
		vstream_info->sub_format = MPEG4_SUB_FORMAT_VP6;
	}
	else if(cedarv_stream_info->sub_format == CEDARV_MPEG4_SUB_FORMAT_WMV1)
	{
		vstream_info->sub_format = MPEG4_SUB_FORMAT_WMV1;
	}
	else if(cedarv_stream_info->sub_format == CEDARV_MPEG4_SUB_FORMAT_WMV2)
	{
		vstream_info->sub_format = MPEG4_SUB_FORMAT_WMV2;
	}
	else
	{
		vstream_info->sub_format = STREAM_SUB_FORMAT_UNKNOW;
	}

	if(cedarv_stream_info->container_format == CEDARV_CONTAINER_FORMAT_UNKNOW)
	{
		vstream_info->container_format = CONTAINER_FORMAT_UNKNOW;
	}
	else if(cedarv_stream_info->container_format == CEDARV_CONTAINER_FORMAT_AVI)
	{
		vstream_info->container_format = CONTAINER_FORMAT_AVI;
	}
	else if(cedarv_stream_info->container_format == CEDARV_CONTAINER_FORMAT_ASF)
	{
		vstream_info->container_format = CONTAINER_FORMAT_ASF;
	}
	else if(cedarv_stream_info->container_format == CEDARV_CONTAINER_FORMAT_DAT)
	{
		vstream_info->container_format = CONTAINER_FORMAT_DAT;
	}
	else if(cedarv_stream_info->container_format == CEDARV_CONTAINER_FORMAT_FLV)
	{
		vstream_info->container_format = CONTAINER_FORMAT_FLV;
	}
	else if(cedarv_stream_info->container_format == CEDARV_CONTAINER_FORMAT_MKV)
	{
		vstream_info->container_format = CONTAINER_FORMAT_MKV;
	}
	else if(cedarv_stream_info->container_format == CEDARV_CONTAINER_FORMAT_MOV)
	{
		vstream_info->container_format = CONTAINER_FORMAT_MOV;
	}
	else if(cedarv_stream_info->container_format == CEDARV_CONTAINER_FORMAT_MPG)
	{
		vstream_info->container_format = CONTAINER_FORMAT_MPG;
	}
	else if(cedarv_stream_info->container_format == CEDARV_CONTAINER_FORMAT_PMP)
	{
		vstream_info->container_format = CONTAINER_FORMAT_PMP;
	}
	else if(cedarv_stream_info->container_format == CEDARV_CONTAINER_FORMAT_RM)
	{
		vstream_info->container_format = CONTAINER_FORMAT_RM;
	}
	else if(cedarv_stream_info->container_format == CEDARV_CONTAINER_FORMAT_TS)
	{
		vstream_info->container_format = CONTAINER_FORMAT_TS;
	}
	else if(cedarv_stream_info->container_format == CEDARV_CONTAINER_FORMAT_VOB)
	{
		vstream_info->container_format = CONTAINER_FORMAT_VOB;
	}
	else
	{
		vstream_info->container_format = CONTAINER_FORMAT_UNKNOW;
	}

	return;
}

static void vdecoder_set_vbv_vedec_memory(video_decoder_t* p)
{
    u32 vbv_size_rank[4] = {8, 10, 12, 16};
    u32 memorySize = 0;
    u32 rank_index = 0;

    if(p->demuxType==CEDARV_STREAM_FORMAT_NETWORK)
    {
        p->max_vbv_buffer_size = 2*1024*1024;
    }
    else
    {
        memorySize = (p->remainMemorySize==0)? 64*1024*1024:p->remainMemorySize;
        if((p->stream_info.video_height ==0)||(p->stream_info.video_height>=1080))
        {
            rank_index = 0;
        }
        else if(p->stream_info.video_height>=720)
        {
            rank_index = 1;
        }
        else if(p->stream_info.video_height>=480)
        {
            rank_index = 2;
        }
        else
        {
            rank_index = 3;
        }
        p->max_vbv_buffer_size = memorySize/vbv_size_rank[rank_index];
        p->max_vbv_buffer_size = (p->max_vbv_buffer_size/(1024*1024))*1024*1024;
        if(p->max_vbv_buffer_size<2*1024*1024)
        {
            p->max_vbv_buffer_size = 2*1024*1024;
        }
    }

	if((p->remainMemorySize==0)||(p->remainMemorySize>=64*1024*1024))
	{
		//set vbv size as 10M for M2TS.
		if((p->demuxType!=CEDARV_STREAM_FORMAT_NETWORK) &&(p->stream_info._3d_mode==_3D_MODE_DOUBLE_STREAM))
			p->max_vbv_buffer_size  = 12*1024*1024;

		if(p->config_info.max_output_width == 0)
			p->config_info.max_output_width = MAX_SUPPORTED_VIDEO_WIDTH;

		if(p->config_info.max_output_height == 0)
			p->config_info.max_output_height = MAX_SUPPORTED_VIDEO_HEIGHT;
	}
	else if(p->remainMemorySize >= 32*1024*1024)
	{
		if(p->config_info.max_output_width == 0 || p->config_info.max_output_width > 1440)
			p->config_info.max_output_width = 1440;

		if(p->config_info.max_output_height == 0 || p->config_info.max_output_height > 800)
			p->config_info.max_output_height = 800;
	}
	else if(p->remainMemorySize >= 16*1024*1024)
	{
		if(p->stream_info.format == STREAM_FORMAT_H264)
		{
			if(p->config_info.max_output_width == 0 || p->config_info.max_output_width > 1440)
				p->config_info.max_output_width = 1440;

			if(p->config_info.max_output_height == 0 || p->config_info.max_output_height > 800)
				p->config_info.max_output_height = 800;
		}
		else
		{
			if(p->config_info.max_output_width == 0 || p->config_info.max_output_width > 1280)
				p->config_info.max_output_width = 1280;

			if(p->config_info.max_output_height == 0 || p->config_info.max_output_height > 800)
				p->config_info.max_output_height = 800;
		}
	}
	else
	{
		if(p->stream_info.format == STREAM_FORMAT_H264)
		{
			if(p->config_info.max_output_width == 0 || p->config_info.max_output_width > 1440)
				p->config_info.max_output_width = 1440;

			if(p->config_info.max_output_height == 0 || p->config_info.max_output_height > 800)
				p->config_info.max_output_height = 800;
		}
		else
		{
			if(p->config_info.max_output_width == 0 || p->config_info.max_output_width > 720)
				p->config_info.max_output_width = 720;

			if(p->config_info.max_output_height == 0 || p->config_info.max_output_height > 576)
				p->config_info.max_output_height = 576;
		}
	}
    p->config_info.max_memory_available = (p->remainMemorySize==0)? 0: p->remainMemorySize-p->max_vbv_buffer_size;
}

int cedarv_f23_ic_version()
{
	return 1;
}

