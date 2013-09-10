#include "fbm.h"
#include "vbv.h"
#include "vdecoder.h"
#include "awprintf.h"
#include "commom_type.h"

#define DEBUG_SAVE_BITSTREAM	(0)
//#define SAVE_FRAME_DATA
        
static s32 vdecoder_open(cedarv_decoder_t* p);
static s32 vdecoder_close(cedarv_decoder_t* p);
static s32 vdecoder_decode(cedarv_decoder_t* p);
static s32 vdecoder_io_ctrl(cedarv_decoder_t* p, u32 cmd, u32 param);
static s32 vdecoder_request_write(cedarv_decoder_t*p, u32 require_size, u8** buf0, u32* size0, u8** buf1, u32* size1);
static s32 vdecoder_update_data(cedarv_decoder_t* p, cedarv_stream_data_info_t* data_info);
static s32 vdeocder_display_request(cedarv_decoder_t* p, cedarv_picture_t* picture);
static s32 vdecoder_picture_ready(cedarv_decoder_t* p);
static s32 vdecoder_dump_picture(cedarv_decoder_t* p, cedarv_picture_t* picture);
static s32 vdecoder_display_release(cedarv_decoder_t* p, u32 frame_index);
static s32 vdecoder_set_video_bitstream_info(cedarv_decoder_t* p, cedarv_stream_info_t* info);
static s32 vdecoder_query_quality(cedarv_decoder_t* p, cedarv_quality_t* vq);

static void set_format(vstream_info_t* vstream_info, cedarv_stream_info_t* cedarv_stream_info);
static void set_3d_mode_to_decoder(vstream_info_t* vstream_info, cedarv_stream_info_t* cedarv_stream_info);
static void vdecoder_set_vbv_vedec_memory(video_decoder_t* p);   

extern void ve_init_clock(void);
extern void ve_release_clock(void);

static s32 set_cedarv_stream_info(cedarv_stream_info_t *cedarv_stream_info, vstream_info_t *vstream_info);

extern IVEControl_t IVE;
extern IOS_t        IOS;

#ifdef SAVE_FRAME_DATA
static int num =0;
const char* fpFramePath = "/data/camera/frame_yv12.dat";
static FILE *f1y,*f1u,*f1v,*f2y,*f2u,*f2v;
static FILE *fyv12 = NULL;
#endif

#if DEBUG_SAVE_BITSTREAM
#ifdef MELIS
const char* fpStreamPath = "e:\\vbitstream.dat";
static ES_FILE* fpStream = NULL;
#else
const char* fpStreamPath = "/data/camera/1011.dat";
const char* fpStreamPath2 = "/data/camera/1012.dat";
static FILE* fpStream = NULL;
static FILE* fpStream2 = NULL;
#endif
#endif

static void libcedarv_free_vbs_buffer_sem(void* vdecoder)
{
	video_decoder_t* p = (video_decoder_t*)vdecoder;
	if(p && p->icedarv.free_vbs_buffer_sem)
		p->icedarv.free_vbs_buffer_sem(p->icedarv.cedarx_cookie);
}

cedarv_decoder_t* libcedarv_init(s32* return_value)
{
	video_decoder_t* p;

	LogOpen();

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
    p->icedarv.display_request      = vdeocder_display_request;
    p->icedarv.picture_ready        = vdecoder_picture_ready;
    p->icedarv.display_dump_picture = vdecoder_dump_picture;
    p->icedarv.display_release      = vdecoder_display_release;
	p->icedarv.set_vstream_info = vdecoder_set_video_bitstream_info;
	p->icedarv.query_quality    = vdecoder_query_quality;

	p->config_info.max_video_width   = MAX_SUPPORTED_VIDEO_WIDTH;
	p->config_info.max_video_height  = MAX_SUPPORTED_VIDEO_HEIGHT;
	p->config_info.max_output_width  = 2048;
	p->config_info.max_output_height = 1440;

	p->config_info.anaglagh_transform_en   = VDECODER_SUPPORT_ANAGLAGH_TRANSFORM;
	p->config_info.anaglagh_transform_on   = 0;
	p->config_info.anaglath_transform_mode = ANAGLAGH_RED_BLUE;
    p->config_info.same_scale_ratio_enable = 1;
    p->config_info.use_maf = VDECODER_SUPPORT_MAF;

	ve_init_clock();

	*return_value = CEDARV_RESULT_OK;

#if DEBUG_SAVE_BITSTREAM
#ifdef MELIS
	fpStream = eLIBs_fopen(fpStreamPath, "wb");
#else
	fpStream = fopen(fpStreamPath, "wb");
	fpStream2 = fopen(fpStreamPath2, "wb");
#endif
#endif

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

	if(decoder->vbv_num == 2 && decoder->minor_vbv != NULL)
	{
		vbv_release(decoder->minor_vbv);
		decoder->minor_vbv = NULL;
	}

	free_rotate_frame_buffer(decoder);//dy_rot

	mem_free(decoder);

	ve_release_clock();

#if DEBUG_SAVE_BITSTREAM
	if(fpStream)
	{
#ifdef MELIS
		eLIBs_fclose(fpStream);
#else
		fclose(fpStream);
		if(fpStream2){
			fclose(fpStream2);
		}
#endif
		fpStream = NULL;
		fpStream2 = NULL;
	}
#endif

	LogClose();

	return CEDARV_RESULT_OK;    
}


static s32 vdecoder_open(cedarv_decoder_t* p)
{
	video_decoder_t* decoder;
    vresult_e   tmpret;

	if(p == NULL)
		return CEDARV_RESULT_ERR_INVALID_PARAM;

	decoder = (video_decoder_t*)p;
	if(decoder->stream_info.format == STREAM_FORMAT_UNKNOW)
		return CEDARV_RESULT_ERR_UNSUPPORTED;

	#ifdef MELIS
	if(decoder->stream_info.sub_format == CEDARV_MPEG4_SUB_FORMAT_DIVX1)
	{
	    return CEDARV_RESULT_ERR_UNSUPPORTED;
	}
	if(decoder->stream_info.format == STREAM_FORMAT_MPEG4 && decoder->stream_info.sub_format == CEDARV_MPEG4_SUB_FORMAT_DIVX2)
	{
	    if(decoder->stream_info.video_height > 480)//not support 720p for melis
	    {
	        return CEDARV_RESULT_ERR_UNSUPPORTED;
	    }
	}
	
	#endif

	//* currently we only support double stream video in format of MJPEG and H264.
	//* for MJPEG, the two stream are packet in one VBV module.
	if(decoder->stream_info._3d_mode == _3D_MODE_DOUBLE_STREAM)
	{
		if(decoder->stream_info.format == STREAM_FORMAT_MJPEG)
			decoder->vbv_num = 1;
		else if(decoder->stream_info.format == STREAM_FORMAT_H264)
			decoder->vbv_num = 2;
		else
		{
			Log("video have double stream, but not MJPEG or H264, format is %d", decoder->stream_info.format);
			decoder->vbv_num = 1;
		}
	}
	
	if(decoder->disable_3d)
		decoder->stream_info._3d_mode = _3D_MODE_NONE;

	if(decoder->status == CEDARV_STATUS_PREVIEW)
	{
		decoder->vbv = vbv_init(BITSTREAM_BUF_SIZE_IN_PREVIEW_MODE, BITSTREAM_FRAME_NUM_IN_PREVIEW_MODE);
		vbv_set_parent(decoder, decoder->vbv);
		vbv_set_free_vbs_sem_cb(libcedarv_free_vbs_buffer_sem, decoder->vbv);
		if(decoder->vbv_num == 2)
		{
			decoder->minor_vbv = vbv_init(BITSTREAM_BUF_SIZE_IN_PREVIEW_MODE, BITSTREAM_FRAME_NUM_IN_PREVIEW_MODE);
			vbv_set_parent(decoder, decoder->minor_vbv);
			vbv_set_free_vbs_sem_cb(libcedarv_free_vbs_buffer_sem, decoder->minor_vbv);
		}
	}
	else
	{
		vdecoder_set_vbv_vedec_memory(decoder);
    	if(decoder->config_vbv_size != 0)
    		decoder->max_vbv_buffer_size = decoder->config_vbv_size;
			
		decoder->vbv = vbv_init(decoder->max_vbv_buffer_size, BITSTREAM_FRAME_NUM);
		vbv_set_parent(decoder, decoder->vbv);
		vbv_set_free_vbs_sem_cb(libcedarv_free_vbs_buffer_sem, decoder->vbv);
		if(decoder->vbv_num == 2)
		{
			decoder->minor_vbv = vbv_init(decoder->max_vbv_buffer_size, BITSTREAM_FRAME_NUM);
			vbv_set_parent(decoder, decoder->minor_vbv);
			vbv_set_free_vbs_sem_cb(libcedarv_free_vbs_buffer_sem, decoder->minor_vbv);
		}

	}

	if(decoder->vbv == NULL)
	{   
		if(decoder->status == CEDARV_STATUS_PREVIEW)
		{
            LogX(L2, "vbv initialize fail, request size = %d, request max frame num = %d", BITSTREAM_BUF_SIZE_IN_PREVIEW_MODE, BITSTREAM_FRAME_NUM_IN_PREVIEW_MODE);
		}
		else
		{
            LogX(L2, "vbv initialize fail, request size = %d, request max frame num = %d", decoder->max_vbv_buffer_size, BITSTREAM_FRAME_NUM);
		}

		return CEDARV_RESULT_ERR_NO_MEMORY;
	}

	if(decoder->vbv_num == 2)
	{
		if(decoder->minor_vbv == NULL)
		{
			if(decoder->status == CEDARV_STATUS_PREVIEW)
			{
                LogX(L2, "minor_vbv initialize fail, request size = %d, request max frame num = %d", BITSTREAM_BUF_SIZE_IN_PREVIEW_MODE, BITSTREAM_FRAME_NUM_IN_PREVIEW_MODE);
			}
			else
			{
                LogX(L2, "vbv initialize fail, request size = %d, request max frame num = %d", decoder->max_vbv_buffer_size, BITSTREAM_FRAME_NUM);
			}
			return CEDARV_RESULT_ERR_NO_MEMORY;
		}
	}

	LogX(L2, "config_info:");
	LogX(L2, "    max_video_width = %d, max_video_height = %d", decoder->config_info.max_video_width, decoder->config_info.max_video_height);
	LogX(L2, "    max_output_width = %d, max_output_height = %d", decoder->config_info.max_output_width, decoder->config_info.max_output_height);
	LogX(L2, "    scale_down_enable = %d", decoder->config_info.scale_down_enable);
	LogX(L2, "    horizon_scale_ratio = %d, vertical_scale_ratio = %d", decoder->config_info.horizon_scale_ratio, decoder->config_info.vertical_scale_ratio);
	LogX(L2, "    rotate_enable = %d, rotate_angle = %d", decoder->config_info.rotate_enable, decoder->config_info.rotate_angle);
	LogX(L2, "    use_maf = %d, max_memory_available = %d", decoder->config_info.use_maf, decoder->config_info.max_memory_available);
	LogX(L2, "    multi_channel = %d", decoder->config_info.multi_channel);

	LogX(L2, "video stream info:");
	LogX(L2, "    format = %d, sub_format = %d, container_format = %d",
			decoder->stream_info.format, decoder->stream_info.sub_format, decoder->stream_info.container_format);
	LogX(L2, "    video width = %d, video height = %d", decoder->stream_info.video_width, decoder->stream_info.video_height);
	LogX(L2, "    frame rate = %d, frame duration = %d", decoder->stream_info.frame_rate, decoder->stream_info.frame_duration);
	LogX(L2, "    aspec_ration = %d, is_pts_correct = %d", decoder->stream_info.aspec_ratio, decoder->stream_info.is_pts_correct);
	LogX(L2, "    init_data_len = %d, init_data = %x", decoder->stream_info.init_data_len, decoder->stream_info.init_data);

	if(decoder->stream_info.format == STREAM_FORMAT_MJPEG)
	{
		decoder->config_info.anaglagh_transform_en = 0;
	}
    
    if((decoder->stream_info.format!=STREAM_FORMAT_MPEG2&&decoder->stream_info.format!=STREAM_FORMAT_H264))
    {
        decoder->config_info.use_maf = 0;
    }

	if(decoder->stream_info._3d_mode == _3D_MODE_DOUBLE_STREAM)
	{
		decoder->config_info.anaglagh_transform_en = 0;
		decoder->config_info.yv12_output_enable = 0;
	}

    //if(decoder->status == CEDARV_STATUS_PREVIEW)
    {
        decoder->config_info.same_scale_ratio_enable = 1;
    }

	libve_set_ive(&IVE);
	libve_set_ios(&IOS);
	libve_set_ifbm(&IFBM);
	libve_set_ivbv(&IVBV);

	decoder->ve = libve_open(&decoder->config_info, &decoder->stream_info, (void*)decoder);
	if(decoder->ve == NULL)
	{
		LogX(L2, "libve_open() fail");

		vbv_release(decoder->vbv);
		decoder->vbv = NULL;
		if(decoder->vbv_num == 2)
		{
			vbv_release(decoder->minor_vbv);
			decoder->minor_vbv = NULL;
		}
		return CEDARV_RESULT_ERR_FAIL;
	}

    libve_set_vbv(decoder->vbv, decoder->ve);
	if(decoder->vbv_num == 2)
	{
		libve_set_minor_vbv(decoder->minor_vbv, decoder->ve);
		//index is used to save stream id of minor stream
		decoder->index = (u32 *)mem_alloc(FBM_MAX_FRAME_NUM);
		if(decoder->index == NULL)
		{
			return CEDARV_RESULT_ERR_FAIL;
		}
		mem_set(decoder->index, 0, FBM_MAX_FRAME_NUM);
	}

	decoder->fbm = libve_get_fbm(decoder->ve);

	//TODO:wait for all decoder define this function
	if((decoder->vbv_num == 2) && (decoder->disable_3d != 1))
	{
		decoder->fbm_num = libve_get_fbm_num(decoder->ve);
	}
	else
		decoder->fbm_num = 1;

//	decoder->fbm_num = libve_get_fbm_num();
	if(decoder->fbm_num == 2)
	{
		decoder->minor_fbm = libve_get_minor_fbm(decoder->ve);
	}
    
    tmpret = libve_io_ctrl(LIBVE_COMMAND_SET_SEQUENCE_INF, (u32)decoder->decinfTag, decoder->ve);  
    if(tmpret != VRESULT_OK)
    {
        Log("set sequence info fail, ret[%x]\n", tmpret);
        return CEDARV_RESULT_ERR_FAIL;
    }
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

	if(decoder->vbv_num == 2)
	{
		if(decoder->minor_vbv != NULL)
		{
			vbv_release(decoder->minor_vbv);
			decoder->minor_vbv = NULL;
		}
	}

	free_rotate_frame_buffer(decoder);//dy_rot

	return CEDARV_RESULT_OK;
}

static s32 vdecoder_decode(cedarv_decoder_t* p)
{
	vresult_e        vresult;

	s64              video_time;
	video_decoder_t* decoder;

	if(p == NULL)
		return CEDARV_RESULT_ERR_INVALID_PARAM;

	decoder = (video_decoder_t*)p;

	if(decoder->mode_switched)
	{
		libve_reset(0, decoder->ve);

		vbv_reset(decoder->vbv);

		if(decoder->vbv_num == 2)
		{
			vbv_reset(decoder->minor_vbv);
		}
		if(p->free_vbs_buffer_sem != NULL)
			p->free_vbs_buffer_sem(p->cedarx_cookie);

		if(decoder->fbm != NULL)
		{
			fbm_flush(decoder->fbm);
			if(decoder->fbm_num == 2)
			{
				if(decoder->minor_fbm != NULL)
					fbm_flush(decoder->minor_fbm);
			}
			if(p->release_frame_buffer_sem != NULL)
				p->release_frame_buffer_sem(p->cedarx_cookie);
		}

		decoder->mode_switched = 0;
		return CEDARV_RESULT_NO_BITSTREAM;
	}

//	if(decoder->dy_rotate_flag==1 && (decoder->stream_info.format == STREAM_FORMAT_MPEG4 || decoder->stream_info.format == STREAM_FORMAT_MPEG2))// || decoder->stream_info.format == STREAM_FORMAT_MJPEG))//test
	if((decoder->rotate_ve_reset==1 || decoder->dy_rotate_flag==1) && (decoder->stream_info.format == STREAM_FORMAT_MPEG4 || decoder->stream_info.format == STREAM_FORMAT_MPEG2))
	{
		decoder->rotate_ve_reset = 0;
//		IVE.ve_reset_hardware();//test

		reset_ve_internal(decoder->ve);
	}

	if(decoder->status == CEDARV_STATUS_BACKWARD || decoder->status == CEDARV_STATUS_FORWARD)
	{
		LogX(L0, "call libve_decode() in FF/RR mode");
		vresult = libve_decode(1, 0, 0, decoder->ve);
	}
	else
	{
		if(decoder->stream_info.format == STREAM_FORMAT_H264 /* && decoder->display_already_begin == 0 */
				&& decoder->stream_info.container_format != CONTAINER_FORMAT_MOV) //*stream media need to show first frame
		{
			//* for H264, when decoding the first data unit containing PPS, we have to promise
			//* there is more than one bitstream frame for decoding.
			//* that is because the decoder uses the start code of the second bitstream frame
			//* to judge PPS end.
			if(vbv_get_stream_num(decoder->vbv) < 2)
			{
				return CEDARV_RESULT_NO_BITSTREAM;
			}
			if((decoder->vbv_num == 2) && (decoder->disable_3d != 1))
			{
				if(vbv_get_stream_num(decoder->minor_vbv) < 2)
					return CEDARV_RESULT_NO_BITSTREAM;
			}
		}

		if(decoder->status == CEDARV_STATUS_PREVIEW)
		{
			LogX(L0, "call libve_decode() in Preview mode");
			vresult = libve_decode(1, 0, 0, decoder->ve);
		}
		else
		{
#if (DECODE_FREE_RUN == 0)
#ifdef MELIS
    	video_time = esMODS_MIoctrl(vdrv_com->avsync, DRV_AVS_CMD_GET_VID_TIME, DRV_AVS_TIME_TOTAL, 0);
#else
    	if(decoder->icedarv.get_current_time != NULL)
    		video_time = decoder->icedarv.get_current_time(decoder->icedarv.cedarx_cookie);
    	else
    		video_time = 0;
#endif

			LogX(L0, "call libve_decode() in Normal mode, system time = %d ms", (u32)video_time);

			video_time *= 1000;

			vresult = libve_decode(0, 1, video_time, decoder->ve);
#else
			LogX(L0, "call libve_decode() in Normal mode, decoder free run");
			video_time = decoder->sys_time;
			if(video_time < 0)
				video_time = 0;
			if(decoder->drop_b_frame)
			{
				vresult = libve_decode(0, 1, video_time, decoder->ve);
			}
			else
			{
				vresult = libve_decode(0, 0, 0, decoder->ve);
			}
#endif
		}

		LogX(L0, "decode result = %d", vresult);
        
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
	else if(vresult == VRESULT_MULTI_PIXEL)
	{
		return CEDARV_RESULT_MULTI_PIXEL;
	}
	else
	{
		return CEDARV_RESULT_OK;
	}
}

s32 free_rotate_frame_buffer(video_decoder_t* decoder)
{
    u32 		  i = 0;
    s32			  Num_rot_img =3;
    vpicture_t*   picture = NULL;

    for(i=0; i<Num_rot_img; i++)
 	{
        picture = &decoder->rotate_img[i];
        if(picture->y != NULL)
	    {
 		    mem_pfree(picture->y);
 		    picture->y = NULL;
 	    }

 	    if(picture->u != NULL)
	    {
 		    mem_pfree(picture->u);
 		    picture->u = NULL;
 	    }
    }
   return 0;
}

s32 alloc_rotate_frame_buffer(video_decoder_t* decoder)
{
	s32     	  i = 0;
	s32			  Num_rot_img =3;//test.2
	vpicture_t*   picture = NULL;
	fbm_t* 		  fbm = NULL;
	u32           size_y;
	u32           size_u;

	if(decoder->rotate_img[0].y!=NULL)
	{
		return 0;
	}

    fbm = (fbm_t*)decoder->fbm;

    if(fbm == NULL)
    {
        return -1;
    }

    size_y = fbm->frames[0].picture.size_y;
    size_u = fbm->frames[0].picture.size_u;

    for(i=0; i<Num_rot_img; i++)
    {
    	decoder->rotate_img[i].size_y = size_y;
    	decoder->rotate_img[i].size_u = size_u;
    	picture = &decoder->rotate_img[i];

        if(picture->size_y)
        {
 		    picture->y = (u8 *)mem_palloc(picture->size_y, 1024);
 		    if(picture->y == NULL)
            {
 			   break;
 		    }else
 		    {
 		    	decoder->rot_img_y_v[i] = picture->y;
 		    	decoder->rot_img_y[i]	= IOS.mem_get_phy_addr((u32)picture->y);
 		    }
 	    }

 	    if(picture->size_u)
        {
 		    picture->u = (u8 *)mem_palloc(picture->size_u, 1024);
 		    if(picture->u == NULL)
            {
 			    break;
 		    }else
 		    {
 		    	decoder->rot_img_c_v[i] = picture->u;
 		    	decoder->rot_img_c[i]	= IOS.mem_get_phy_addr((u32)picture->u);
 		    }
 	    }
    }

    if(i < Num_rot_img)
    {
        free_rotate_frame_buffer(decoder);
        return -1;
    }
    return 0;
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

		case CEDARV_COMMAND_DYNAMIC_ROTATE://dy_rot
		{
			decoder->dy_rotate_on = 0;
			if(decoder->config_info.yv12_output_enable == 1)
			{
				if(param > 3)
					return CEDARV_RESULT_ERR_UNSUPPORTED;

				decoder->config_info.rotate_angle  = param;
				decoder->rotate_angle = param;
				if(param != 0)
				{
					decoder->config_info.rotate_enable = 1;
//					decoder->video_rotate_flag = 1;
				}else
				{
//					decoder->video_rotate_flag = 0;
				}

	        	if(libve_io_ctrl(LIBVE_COMMAND_SET_VIDEO_ROTATE_ANGLE, param, decoder->ve) != VRESULT_OK)
	        	{
	        		return CEDARV_RESULT_ERR_UNSUPPORTED;
	        	}

	        	decoder->dy_rotate_on = 1;
				return CEDARV_RESULT_OK;
			}
			else
			{
				if(param > 5)
					return CEDARV_RESULT_ERR_UNSUPPORTED;

				if(decoder->stream_info._3d_mode == _3D_MODE_DOUBLE_STREAM)
					return CEDARV_RESULT_ERR_UNSUPPORTED;

				if(decoder->config_info.rotate_angle != param)
				{
                    Log("CEDARV_COMMAND_DYNAMIC_ROTATE, decoder->config_info.rotate_angle[%d], param[%d]", decoder->config_info.rotate_angle, param);
					decoder->rotate_angle = param;

	                if(alloc_rotate_frame_buffer(decoder) == VRESULT_ERR_FAIL)
	                {
	                    return CEDARV_RESULT_ERR_FAIL;
	                }
	                decoder->rotate_ve_reset = 1;

	                decoder->dy_rotate_flag = 1;
				}else
				{
					decoder->rotate_angle = param;//test
					decoder->rotate_ve_reset = 1;
					decoder->dy_rotate_flag = 0;
				}
				decoder->dy_rotate_on = 1;
				return CEDARV_RESULT_OK;
			}

			return CEDARV_RESULT_ERR_UNSUPPORTED;
		}

		case CEDARV_COMMAND_JUMP:
		{
#if 1
			if(decoder->ve)
			{
				libve_flush(0, decoder->ve);
				libve_reset(0, decoder->ve);
			}
#else
			if(decoder->ve)
				libve_flush(0, decoder->ve);
#endif

			mem_set(&decoder->cur_stream_part, 0, sizeof(vstream_data_t));
			mem_set(&decoder->cur_minor_stream_part, 0, sizeof(vstream_data_t));

			vbv_reset(decoder->vbv);
			if(decoder->vbv_num == 2)
			{
				vbv_reset(decoder->minor_vbv);
			}

			if(p->free_vbs_buffer_sem != NULL)
				p->free_vbs_buffer_sem(p->cedarx_cookie);

			if(decoder->fbm != NULL)
			{
				fbm_flush(decoder->fbm);
				if(decoder->fbm_num == 2 && decoder->minor_fbm)
				{					
					fbm_flush(decoder->minor_fbm);
				}

				if(p->release_frame_buffer_sem != NULL)
					p->release_frame_buffer_sem(p->cedarx_cookie);
			}

			return CEDARV_RESULT_OK;
		}

		case CEDARV_COMMAND_SET_TOTALMEMSIZE:
		{
			decoder->remainMemorySize = param;
			return CEDARV_RESULT_OK;
		}


		case CEDARV_COMMAND_PREVIEW_MODE:
		{
			decoder->status = CEDARV_STATUS_PREVIEW;
			decoder->stream_info.is_preview_mode = 1;
			return CEDARV_RESULT_OK;
		}

		case CEDARV_COMMAND_RESET:
		{
			libve_reset(0, decoder->ve);
			vbv_reset(decoder->vbv);
			if(decoder->vbv_num == 2)
			{
				vbv_reset(decoder->minor_vbv);
			}

			if(p->free_vbs_buffer_sem != NULL)
				p->free_vbs_buffer_sem(p->cedarx_cookie);

			if(decoder->fbm != NULL)
			{
				fbm_flush(decoder->fbm);
				if(decoder->fbm_num == 2)
				{
					fbm_flush(decoder->minor_fbm);
				}

				if(p->release_frame_buffer_sem != NULL)
					p->release_frame_buffer_sem(p->cedarx_cookie);
			}

			decoder->mode_switched = 0;
            return CEDARV_RESULT_OK;
        }

		case CEDARV_COMMAND_FLUSH:
		{
			if(decoder->ve)
				libve_flush(1, decoder->ve);
            return CEDARV_RESULT_OK;
        }

        case CEDARV_COMMAND_GET_STREAM_INFO:    //param = cedarv_stream_info_t*
        {
            vresult_e   			tmpret;
            vstream_info_t 			vstream_info;
            cedarv_stream_info_t*	pCedarvStreamInfo;

            pCedarvStreamInfo = (cedarv_stream_info_t*)param;

            mem_set(&vstream_info, 0, sizeof(vstream_info_t));
            tmpret = libve_get_stream_info(&vstream_info, decoder->ve);    
            if(tmpret != VRESULT_OK)
            {
                Log("get stream info fail, ret[%x]\n", tmpret);
                return CEDARV_RESULT_ERR_FAIL;
            }

            set_cedarv_stream_info(pCedarvStreamInfo, &vstream_info);
            return CEDARV_RESULT_OK;
        }

        //* set maximum output picture size, for scale mode.
		case CEDARV_COMMAND_SET_MAX_OUTPUT_WIDTH:
		{
			decoder->config_info.max_output_width = param;
			return 	CEDARV_RESULT_OK;
		}

		case CEDARV_COMMAND_SET_MAX_OUTPUT_HEIGHT:
		{
			decoder->config_info.max_output_height = param;
			return 	CEDARV_RESULT_OK;
		}

		//* for mepg2 tag play.
        case CEDARV_COMMAND_SET_SEQUENCE_INFO:
        {   
            mem_cpy(decoder->decinfTag, (void*)param, VDECODER_TAG_INF_SIZE);
            return CEDARV_RESULT_OK;
        }
        case CEDARV_COMMAND_GET_SEQUENCE_INFO:
        {
            vresult_e   tmpret;
            //tmpret = libve_get_sequence_info(param);   
            tmpret = libve_io_ctrl(LIBVE_COMMAND_GET_SEQUENCE_INF,param, decoder->ve); 
            if(tmpret != VRESULT_OK)
            {
                Log("get sequence info fail, ret[%x]\n", tmpret);
                return CEDARV_RESULT_ERR_FAIL;
            }
            return CEDARV_RESULT_OK;
        }

        //* for transforming 3d 'size by size' or 'top to bottom' pictures to 'anaglagh' pictures.
        case CEDARV_COMMAND_SET_STREAM_3D_MODE:	//* reset the 3d mode of input stream.
        {
        	cedarv_3d_mode_e tmp;

        	tmp = (cedarv_3d_mode_e)param;

        	if(decoder->stream_info._3d_mode >= _3D_MODE_MAX)
        		return CEDARV_RESULT_ERR_INVALID_PARAM;

        	if(libve_io_ctrl(LIBVE_COMMAND_SET_SOURCE_3D_MODE, param, decoder->ve) == VRESULT_OK)
        	{
            	decoder->stream_info._3d_mode = (_3d_mode_e)tmp;
        		return CEDARV_RESULT_OK;
        	}
        	else
        		return CEDARV_RESULT_ERR_UNSUPPORTED;
        }

        case CEDARV_COMMAND_SET_ANAGLATH_TRANS_MODE:
        {
        	cedarv_anaglath_trans_mode_e tmp;

        	if(decoder->stream_info._3d_mode != _3D_MODE_SIDE_BY_SIDE &&
        	   decoder->stream_info._3d_mode != _3D_MODE_TOP_TO_BOTTOM &&
        	   decoder->stream_info._3d_mode != _3D_MODE_NONE) /* for some stream, parser don't know it is really side by size or top to bottom format*/
        	{
        		return CEDARV_RESULT_ERR_UNSUPPORTED;
        	}

        	tmp = (cedarv_anaglath_trans_mode_e)param;
        	if(libve_io_ctrl(LIBVE_COMMAND_SET_ANAGLATH_TRANSMISSION_MODE, (anaglath_trans_mode_e)tmp, decoder->ve) == VRESULT_OK)
        	{
        		decoder->config_info.anaglath_transform_mode = (anaglath_trans_mode_e)tmp;
        		return CEDARV_RESULT_OK;
        	}
        	else
        		return CEDARV_RESULT_ERR_UNSUPPORTED;
        }

        case CEDARV_COMMAND_OPEN_ANAGLATH_TRANSFROM:
        {
            if(decoder->config_info.anaglagh_transform_en == 1)
            {
                if(fbm_alloc_redBlue_frame_buffer(decoder->fbm) == VRESULT_ERR_FAIL)
                {   
                    return CEDARV_RESULT_ERR_UNSUPPORTED;
                }
            }
        	if(libve_io_ctrl(LIBVE_COMMAND_OPEN_ANAGLATH_TRANSFROM, 0, decoder->ve) == VRESULT_OK)
        	{
        		decoder->config_info.anaglagh_transform_on = 1;
        		return CEDARV_RESULT_OK;
        	}
        	else
        		return CEDARV_RESULT_ERR_UNSUPPORTED;
        }

        case CEDARV_COMMAND_CLOSE_ANAGLATH_TRANSFROM:
        {   
            if(decoder->config_info.anaglagh_transform_en == 1)
            {
                if(fbm_free_redBlue_frame_buffer(decoder->fbm) == VRESULT_ERR_FAIL)
                {
                    return CEDARV_RESULT_ERR_UNSUPPORTED;
                }
            }
        	libve_io_ctrl(LIBVE_COMMAND_CLOSE_ANAGLATH_TRANSFROM, 0, decoder->ve);
        	decoder->config_info.anaglagh_transform_on = 0;
        	return CEDARV_RESULT_OK;
        }

        case CEDARV_COMMAND_OPEN_YV32_TRANSFROM:
        {
        	if(decoder->config_info.yv32_output_enable == 1)
        	{
        		if(fbm_alloc_YV12_frame_buffer(decoder->fbm) == VRESULT_ERR_FAIL)
        		{
        			return CEDARV_RESULT_ERR_UNSUPPORTED;
        		}
        	}else
        		return CEDARV_RESULT_ERR_UNSUPPORTED;

        	if(libve_io_ctrl(LIBVE_COMMAND_OPEN_YV12_TRANSFROM, 0, decoder->ve) == VRESULT_OK)
        	{
        		decoder->config_info.yv32_transform_on_yv12 = 1;
        		return CEDARV_RESULT_OK;
        	}
        	else
        		return CEDARV_RESULT_ERR_UNSUPPORTED;

        	return CEDARV_RESULT_OK;
        }

        case CEDARV_COMMAND_CLOSE_YV32_TRANSFROM:
        {
            if(decoder->config_info.yv32_output_enable == 1)
            {
                if(fbm_free_YV12_frame_buffer(decoder->fbm) == VRESULT_ERR_FAIL)
                {
                    return CEDARV_RESULT_ERR_UNSUPPORTED;
                }
            }
        	libve_io_ctrl(LIBVE_COMMAND_CLOSE_YV12_TRANSFROM, 0, decoder->ve);
        	decoder->config_info.yv32_transform_on_yv12 = 0;
        	return CEDARV_RESULT_OK;
        }
        case CEDARV_COMMAND_DISABLE_3D:
        	decoder->disable_3d = 1;
        	return CEDARV_RESULT_OK;

        case CEDARV_COMMAN_SET_SYS_TIME:
        	decoder->sys_time = *((s64 *)param);
        	return CEDARV_RESULT_OK;

        case CEDARV_COMMAND_DROP_B_FRAME:
            if(decoder->stream_info._3d_mode == _3D_MODE_DOUBLE_STREAM)
            {   
                decoder->drop_b_frame = 0;
            }
            else
        	{   
                decoder->drop_b_frame = param;
            }
        	return CEDARV_RESULT_OK;

        case CEDARV_COMMAND_CLOSE_MAF:
        {
        	decoder->config_info.use_maf = 0;
            if(decoder->ve != NULL)
            {
                 libve_io_ctrl(LIBVE_COMMAND_CLOSE_MAF, 0, decoder->ve);
            }
        	return CEDARV_RESULT_OK;
        }
        case CEDARV_COMMAND_DECODE_NO_DELAY:
		{
			decoder->config_info.decode_no_delay = param;
			return CEDARV_RESULT_OK;
		}
		case CEDARV_COMMAND_SET_VBV_SIZE:
		{
			decoder->config_vbv_size = param;
			return CEDARV_RESULT_OK;
		}
		case CEDARV_COMMAND_MULTI_PIXEL:
        {
    		if(decoder->ve != NULL)
    		{
    			libve_close(0, decoder->ve);
    			decoder->ve = NULL;
    			decoder->fbm = NULL;
    		}

    		decoder->ve = libve_open(&decoder->config_info, &decoder->stream_info, (void*)decoder);
    		if(decoder->ve == NULL)
    		{
    			LogX(L2, "libve_open() fail");

    			vbv_release(decoder->vbv);
    			decoder->vbv = NULL;
    			if(decoder->vbv_num == 2)
    			{
    				vbv_release(decoder->minor_vbv);
    				decoder->minor_vbv = NULL;
    			}
    			return CEDARV_RESULT_ERR_FAIL;
    		}

    	    libve_set_vbv(decoder->vbv, decoder->ve);

    		decoder->fbm = libve_get_fbm(decoder->ve);

    		decoder->fbm_num = 1;

    	//	decoder->fbm_num = libve_get_fbm_num();
    		if(decoder->fbm_num == 2)
    		{
    			decoder->minor_fbm = libve_get_minor_fbm(decoder->ve);
    		}
    		return CEDARV_RESULT_OK;
        }
        case CEDARV_COMMAND_SET_PIXEL_FORMAT:
        {
        	decoder->config_info.yv12_output_enable = param;
            return CEDARV_RESULT_OK;
        }
        case CEDARV_COMMAND_SET_DISPLAYFRAME_REQUESTMODE:
        {
        	decoder->config_info.nDisplayFrameRequestMode = param;
            return CEDARV_RESULT_OK;
        }
		case CEDARV_COMMAND_ENABLE_SCALE_DOWN:
		{
			if (param < 0 || param > 1)
			{
				return CEDARV_RESULT_ERR_FAIL;
			}
			decoder->config_info.scale_down_enable = param;
			return CEDARV_RESULT_OK;
		}
		case CEDARV_COMMAND_SET_HORIZON_SCALE_RATIO:
		{
			if (param < 0 || param > 2) 
			{
				return CEDARV_RESULT_ERR_FAIL;
			}
			decoder->config_info.horizon_scale_ratio = param;
			return CEDARV_RESULT_OK;
		}
		case CEDARV_COMMAND_SET_VERTICAL_SCALE_RATIO:
		{
			if (param < 0 || param > 2) 
			{
				return CEDARV_RESULT_ERR_FAIL;
			}
			decoder->config_info.vertical_scale_ratio = param;
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
	if(decoder->vbv_num == 2)
	{
		if(decoder->minor_vbv == NULL)
		{
			return CEDARV_RESULT_ERR_FAIL;
		}
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
		if(decoder->vbv_num == 2)
		{
			if(vbv_get_stream_num(decoder->minor_vbv) > 1)
			{
				return CEDARV_RESULT_ERR_FAIL;
			}
		}
	}

	if(require_size == 0)
	{
		require_size += 4;
	}

	if((p->video_stream_type == CDX_VIDEO_STREAM_MAJOR) || (decoder->vbv_num != 2))
	{
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
	}
	else if((p->video_stream_type == CDX_VIDEO_STREAM_MINOR) && (decoder->vbv_num == 2))
	{
		if(vbv_request_buffer(&start, &free_size, require_size + decoder->cur_minor_stream_part.length, decoder->minor_vbv) != 0)
		{
			return CEDARV_RESULT_ERR_FAIL;
		}

		if(free_size <= decoder->cur_minor_stream_part.length)
		{
			return CEDARV_RESULT_ERR_FAIL;
		}

		vbv_end  = vbv_get_base_addr(decoder->minor_vbv) + vbv_get_buffer_size(decoder->minor_vbv);
		start    = start + decoder->cur_minor_stream_part.length;
		if(start >= vbv_end)
		{
			start -= vbv_get_buffer_size(decoder->minor_vbv);
		}
		free_size -= decoder->cur_minor_stream_part.length;
	}

#if 1
	//* give ring buffer.
	if((start + free_size) > vbv_end)
	{
		*buf0  = start;
		*size0 = vbv_end - start;
		if((p->video_stream_type == CDX_VIDEO_STREAM_MAJOR) || (decoder->vbv_num != 2))
		{
			*buf1 = vbv_get_base_addr(decoder->vbv);
		}
		else if((p->video_stream_type == CDX_VIDEO_STREAM_MINOR) && (decoder->vbv_num == 2 ))
		{
			*buf1= vbv_get_base_addr(decoder->minor_vbv);
		}
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
	CDX_VIDEO_STREAM_TYPE type;


	if(p == NULL)
		return CEDARV_RESULT_ERR_INVALID_PARAM;

	decoder = (video_decoder_t*)p;

	if(data_info == NULL)
		return CEDARV_RESULT_ERR_INVALID_PARAM;

	type = data_info->type;
	if((type == CDX_VIDEO_STREAM_MAJOR) || (decoder->vbv_num != 2 ))
	{
		vbv    = decoder->vbv;
		stream = &decoder->cur_stream_part;

		if(data_info->flags & CEDARV_FLAG_FIRST_PART)
		{
			if(stream->length != 0)
			{
				stream->valid = 0;
				vbv_add_stream(stream, vbv);
			}
			if(type == CDX_VIDEO_STREAM_MAJOR)
			{
				stream->stream_type = CEDARV_STREAM_TYPE_MAJOR;
			}
			else if(type == CDX_VIDEO_STREAM_MINOR)
			{
				stream->stream_type = CEDARV_STREAM_TYPE_MINOR;
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

			stream->pict_prop = data_info->flags;
		}
		else
		{
			stream->length += data_info->lengh;
		}
	}
	else if((type == CDX_VIDEO_STREAM_MINOR) && (decoder->vbv_num == 2 ))
	{
		if(decoder->disable_3d)
			return 0;
		vbv    = decoder->minor_vbv;
		stream = &decoder->cur_minor_stream_part;
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
	}


	if(data_info->flags & CEDARV_FLAG_LAST_PART)
	{   
		if(data_info->flags & CEDARV_FLAG_DATA_INVALID)
			stream->valid = 0;
		else
			stream->valid = 1;

#if 0
		if(stream->data != NULL && stream->length != 0)
		{
	        //* flush cache.
	        {
				u8* vbv_end;
				u32 tmp_length;
				vbv_end  = vbv_get_base_addr(decoder->vbv) + vbv_get_buffer_size(decoder->vbv);
	    		if(stream->data + stream->length > vbv_end)
	    		{
	    			tmp_length = vbv_end - stream->data;
	                IOS.mem_flush_cache(stream->data, tmp_length);
	                IOS.mem_flush_cache(vbv_get_base_addr(decoder->vbv), stream->length - tmp_length);
	    		}
	    		else
	    		{   
	    			IOS.mem_flush_cache(stream->data, stream->length);
	    		}
	        }
			vbv_add_stream(stream, vbv);
		}
    	else
    	{
    		if(data_info->flags & CEDARV_FLAG_MPEG4_EMPTY_FRAME)	//* MPEG 4 Empty frame from AVI parser.
        		vbv_add_stream(stream, vbv);
    	}
#else
        if(stream->data != NULL && stream->length != 0)
		{
	        //* flush cache.
	        {
				u8* vbv_end;
				u32 tmp_length;
				vbv_end  = vbv_get_base_addr(vbv) + vbv_get_buffer_size(vbv);
	    		if(stream->data + stream->length > vbv_end)
	    		{
	    			tmp_length = vbv_end - stream->data;
	                IOS.mem_flush_cache(stream->data, tmp_length);
	                IOS.mem_flush_cache(vbv_get_base_addr(vbv), stream->length - tmp_length);
	    		}
	    		else
	    		{   
	    			IOS.mem_flush_cache(stream->data, stream->length);
	    		}
	        }
			vbv_add_stream(stream, vbv);
		}
    	else
    	{
    		if(data_info->flags & CEDARV_FLAG_MPEG4_EMPTY_FRAME)	//* MPEG 4 Empty frame from AVI parser.
        		vbv_add_stream(stream, vbv);
    	}
#endif

#if DEBUG_SAVE_BITSTREAM
		if(fpStream != NULL)
		{
			u8* vbv_end;
			u32 tmp_length;

			if(type == CDX_VIDEO_STREAM_MAJOR || decoder->vbv_num != 2)
			{
				vbv_end  = vbv_get_base_addr(decoder->vbv) + vbv_get_buffer_size(decoder->vbv);
				if(stream->data + stream->length > vbv_end)
				{
					tmp_length = vbv_end - stream->data;
#ifdef MELIS
					eLIBs_fwrite(stream->data, 1, tmp_length, fpStream);
					eLIBs_fwrite(vbv_get_base_addr(decoder->vbv), 1, stream->length - tmp_length, fpStream);
#else
					fwrite(stream->data, 1, tmp_length, fpStream);
					fwrite(vbv_get_base_addr(decoder->vbv), 1, stream->length - tmp_length, fpStream);
#endif
				}
				else
				{
#ifdef MELIS
					eLIBs_fwrite(stream->data, 1, stream->length, fpStream);
#else
					fwrite(stream->data, 1, stream->length, fpStream);
#endif
				}
			}
			else if(fpStream2 != NULL)
			{
				vbv_end = vbv_get_base_addr(decoder->minor_vbv) + vbv_get_buffer_size(decoder->minor_vbv);
				if(stream->data + stream->length > vbv_end)
				{
					tmp_length = vbv_end - stream->data;
#ifdef MELIS
					eLIBs_fwrite(stream->data, 1, tmp_length, fpStream2);
					eLIBs_fwrite(vbv_get_base_addr(decoder->minor_vbv), 1, stream->length - tmp_length, fpStream2);
#else
					fwrite(stream->data, 1, tmp_length, fpStream2);
					fwrite(vbv_get_base_addr(decoder->minor_vbv), 1, stream->length - tmp_length, fpStream2);
#endif
				}
				else
				{
#ifdef MELIS
					eLIBs_fwrite(stream->data, 1, stream->length, fpStream2);
#else
					fwrite(stream->data, 1, stream->length, fpStream2);
#endif
				}
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



static s32 vdecoder_picture_ready(cedarv_decoder_t* p)
{
	video_decoder_t* decoder = NULL;

    if(p == NULL)
        return 0;

    decoder = (video_decoder_t*)p;

    if(decoder->fbm == NULL)
    {
        if(decoder->ve == NULL)
            return 0;

        decoder->fbm = libve_get_fbm(decoder->ve);

        if(decoder->fbm == NULL)
            return 0;
    }

    if(fbm_display_pick_frame(decoder->fbm) != NULL)
    	return 1;
    else
    	return 0;
}


static s32 vdecoder_dump_picture(cedarv_decoder_t* p, cedarv_picture_t* picture)
{
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


    frame = fbm_display_pick_frame(decoder->fbm);
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
    picture->y                      = (unsigned char*)frame->y;
    picture->u                      = (unsigned char*)frame->u;
    picture->v                      = NULL;
    picture->alpha                  = NULL;

    return CEDARV_RESULT_OK;
}
static s32 vdeocder_display_request(cedarv_decoder_t* p, cedarv_picture_t* picture)
{
#if (DISPLAY_FREE_RUN == 0)
	u32              video_time;
#endif

	vpicture_t*      frame = NULL;
	vpicture_t*		 minor_frame = NULL;
	video_decoder_t* decoder = NULL;
	u32 top_offset;
	u32 left_offset;
	u32 width_align;
	u32 height_align;
	u32 width_offset;
	u32 height_offset;
	u32 ori_width;
	u32 ori_height;

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

	if(decoder->fbm_num == 2 && decoder->minor_fbm == NULL)
	{
		decoder->minor_fbm = libve_get_minor_fbm(decoder->ve);
		if(decoder->minor_fbm == NULL)
		{
			return CEDARV_RESULT_ERR_FAIL;
		}
	}

	if(decoder->status == CEDARV_STATUS_PREVIEW)
	{
		if(decoder->fbm_num != 2)
		{
			frame = fbm_display_request_frame(decoder->fbm);
		}
		else
		{
			frame = fbm_display_pick_frame(decoder->fbm);
			minor_frame = fbm_display_pick_frame(decoder->minor_fbm);
			if(frame == NULL || minor_frame == NULL)
			{
				return CEDARV_RESULT_ERR_FAIL;
			}
			else
			{
				frame = fbm_display_request_frame(decoder->fbm);
				minor_frame = fbm_display_request_frame(decoder->minor_fbm);
			}
		}

	}
	else
	{
#if (DISPLAY_FREE_RUN == 1)
get_frame_again:
		if(decoder->fbm_num != 2)
		{
			frame = fbm_display_request_frame(decoder->fbm);
		}
		else
		{
			frame = fbm_display_pick_frame(decoder->fbm);
			minor_frame = fbm_display_pick_frame(decoder->minor_fbm);
			if(frame == NULL || minor_frame == NULL)
			{
				return CEDARV_RESULT_ERR_FAIL;
			}
			else
			{
				frame = fbm_display_request_frame(decoder->fbm);
				minor_frame = fbm_display_request_frame(decoder->minor_fbm);
			}
		}
#else
		if(decoder->fbm_num != 2)
		{
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
		}
		else
		{
			if(decoder->display_already_begin == 1)
			{
get_frame_again:
				frame = fbm_display_pick_frame(decoder->fbm);
				minor_frame = fbm_display_pick_frame(decoder->minor_fbm);
				if(frame != NULL && minor_frame != NULL)
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
						minor_frame = fbm_display_request_frame(decoder->minor_fbm);

						if(decoder->status == CEDARV_STATUS_PLAY)
						{
							if((frame->pts/1000) + DISPLAY_TIME_WINDOW_WIDTH < video_time)
							{
								if(fbm_display_pick_frame(decoder->fbm) != NULL)
								{
									fbm_display_return_frame(frame, decoder->fbm);
									fbm_display_return_frame(minor_frame, decoder->minor_fbm);
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
						minor_fbm = fbm_display_request_frame(decoder->minor_fbm);
					}
				}
			}
			else
			{
				frame = fbm_display_request_frame(decoder->fbm);
				minor_frame = fbm_display_request_frame(decoder->minor_fbm);
			}
		}
#endif
	}

	if(decoder->fbm_num != 2)
	{
		if(frame == NULL)
		{
			return CEDARV_RESULT_ERR_FAIL;
		}
	}
	else
	{
		if(frame != NULL && minor_frame == NULL)
		{
			fbm_vdecoder_return_frame(frame, decoder->fbm);
			return CEDARV_RESULT_ERR_FAIL;
		}
		else if(frame == NULL && minor_frame != NULL)
		{
			fbm_vdecoder_return_frame(minor_frame, decoder->minor_fbm);
			return CEDARV_RESULT_ERR_FAIL;

		}
		else if(frame == NULL && minor_frame == NULL)
			return CEDARV_RESULT_ERR_FAIL;
	}

	//if(decoder->dy_rotate_flag == 1)//dy_rot
	if(frame->pixel_format != CEDARV_PIXEL_FORMAT_PLANNER_YVU420)
	{
        if(decoder->dy_rotate_on && decoder->dy_rotate_flag)
        {
            //Log("mb32_rot[%d], pixel_format[0x%x], decoder->dy_rotate_on[%d], dy_rotate_flag[%d], rotate_angle[%d]", 
            //    decoder->cur_img_flag, frame->pixel_format, decoder->dy_rotate_on, decoder->dy_rotate_flag, decoder->rotate_angle);
    		decoder->cur_img_flag++;
    		if(decoder->cur_img_flag>=3)
    			decoder->cur_img_flag = 0;

    		dy_rotate_frame(decoder,frame);
            //Log("mb32_rot_done");
        }
	}
	else
	{
		if(decoder->config_info.rotate_angle != frame->rotate_angle && decoder->fbm_num != 2)
		{
            Log("config_rotate_angle[%d], frame->rotate_angle[%d], will ignore frame", decoder->config_info.rotate_angle, frame->rotate_angle);
			fbm_display_return_frame(frame, decoder->fbm);
            decoder->mRotateFlag = 1;
            decoder->mRotateIgnoreFrameNum = 0;
			goto get_frame_again;
		}
        else
        {
            if(decoder->mRotateFlag)
            {
                if(decoder->mRotateIgnoreFrameNum < 16)  //ignore 8 frame to guarantee frame rotation right
                {
                    Log("after rotate, num[%d] ignore", decoder->mRotateIgnoreFrameNum);
                    decoder->mRotateIgnoreFrameNum++;
//                    Log("frame, y[%p], u[%p], v[%p], size_y[%d], size_u[%d], size_v[%d],rotate_angle[%d]", 
//                        frame->y, frame->u, frame->v, frame->size_y, frame->size_u, frame->size_v, frame->rotate_angle);
//                    fyv12 = fopen(fpFramePath, "wb");
//                    fwrite(frame->y, 1, frame->size_y+frame->size_u+frame->size_v, fyv12);
//                    fclose(fyv12);
//                    fyv12 = NULL;
                    fbm_display_return_frame(frame, decoder->fbm);
                    goto get_frame_again;
                }
                else
                {
                    decoder->mRotateFlag = 0;
                    decoder->mRotateIgnoreFrameNum = 0;
                    Log("set mRotateFlag = 0");
                }
            }
        }
	}

 //**********************modified by xyliu 2012-02-23**************************************************//

    if(((decoder->config_info.anaglagh_transform_on==1) &&(frame->anaglath_transform_mode==ANAGLAGH_NONE)) ||
        ((decoder->config_info.anaglagh_transform_on==0) &&(frame->anaglath_transform_mode!=ANAGLAGH_NONE)))
    {   
        if(decoder->fbm_num != 2)
        {
            fbm_display_return_frame(frame, decoder->fbm);
        }
        else
        {
            fbm_display_return_frame(frame, decoder->fbm);
			fbm_display_return_frame(minor_frame, decoder->minor_fbm);
        }
	    if(p->release_frame_buffer_sem != NULL)
	    {
            p->release_frame_buffer_sem(p->cedarx_cookie);
	    }
        return CEDARV_RESULT_ERR_FAIL;
    }

	if(decoder->config_info.rotate_enable)
	{
		if(frame->rotate_angle == 1 || frame->rotate_angle == 3)
		{
			ori_width = frame->height;
			ori_height = frame->width;
		}else
		{
			ori_width = frame->width;
			ori_height = frame->height;
		}

		width_align = ((ori_width + ((1<<(4-frame->horizontal_scale_ratio))-1))>>(4-frame->horizontal_scale_ratio))<<(4-frame->horizontal_scale_ratio);
		height_align = ((ori_height + ((1<<(4-frame->vertical_scale_ratio))-1))>>(4-frame->vertical_scale_ratio))<<(4-frame->vertical_scale_ratio);
		width_offset = width_align - ori_width;
		height_offset = height_align - ori_height;

		if(frame->rotate_angle == 1)
		{
			top_offset = 0;
			left_offset = height_offset;
		}else if(frame->rotate_angle == 2)
		{
			top_offset = height_offset;
			left_offset = width_offset;
		}else if(frame->rotate_angle == 3)
		{
			top_offset = width_offset;
			left_offset = 0;
		}else if(frame->rotate_angle == 4)
		{
			top_offset = 0;
			left_offset = width_offset;
		}else if(frame->rotate_angle == 5)
		{
			top_offset = height_offset;
			left_offset = 0;
		}else
		{
			top_offset = 0;
			left_offset = 0;
		}
	}else
	{
		top_offset = 0;
		left_offset = 0;
	}

	picture->id                     = frame->id;
	picture->width                  = frame->width;
	picture->height                 = frame->height;
	picture->top_offset				= top_offset;//frame->top_offset;
	picture->left_offset			= left_offset;//frame->left_offset;
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
	picture->pict_prop              = frame->pict_prop;
	picture->repeat_top_field       = frame->repeat_top_field;
	picture->repeat_bottom_field    = frame->repeat_bottom_field;
	picture->_3d_mode				= (cedarv_3d_mode_e)frame->_3d_mode;
	picture->anaglath_transform_mode = (cedarv_anaglath_trans_mode_e)frame->anaglath_transform_mode;
	
	if(decoder->dy_rotate_on==1)
	{
		if(decoder->stream_info.video_height==0 || decoder->stream_info.video_width==0)
		{
			if(frame->width > frame->height)
			{
				decoder->stream_info.video_width  = frame->width;
				decoder->stream_info.video_height = frame->height;
			}else
			{
				decoder->stream_info.video_width  = frame->height;
				decoder->stream_info.video_height = frame->width;
			}
		}

		if(decoder->rotate_angle == 1 || decoder->rotate_angle == 3)
		{
			ori_width = decoder->stream_info.video_height;
			ori_height = decoder->stream_info.video_width;
		}else
		{
			ori_width = decoder->stream_info.video_width;
			ori_height = decoder->stream_info.video_height;
		}

		width_align = (((ori_width>>frame->horizontal_scale_ratio) + ((1<<(4-frame->horizontal_scale_ratio))-1))>>(4-frame->horizontal_scale_ratio))<<(4-frame->horizontal_scale_ratio);
		height_align = (((ori_height>>frame->vertical_scale_ratio) + ((1<<(4-frame->vertical_scale_ratio))-1))>>(4-frame->vertical_scale_ratio))<<(4-frame->vertical_scale_ratio);

		picture->width = width_align;
		picture->height = height_align;
		picture->display_width = ori_width>>frame->horizontal_scale_ratio;
		picture->display_height = ori_height>>frame->vertical_scale_ratio;
		picture->rotate_angle	= decoder->rotate_angle;
	}

	if((decoder->config_info.anaglagh_transform_on==1) && (picture->anaglath_transform_mode != ANAGLAGH_NONE))
		picture->is_progressive = 1;

	if(frame->pixel_format == CEDARV_PIXEL_FORMAT_AW_YUV422)
	{
		picture->pixel_format = CEDARV_PIXEL_FORMAT_AW_YUV422;
		if(decoder->rotate_angle +decoder->config_info.rotate_angle > 0)
		{
			picture->pixel_format = CEDARV_PIXEL_FORMAT_AW_YUV420;
		}
	}
	else if(frame->pixel_format == CEDARV_PIXEL_FORMAT_AW_YUV411)
		picture->pixel_format = CEDARV_PIXEL_FORMAT_AW_YUV411;
	else
		picture->pixel_format = CEDARV_PIXEL_FORMAT_AW_YUV420;

	if(decoder->config_info.yv12_output_enable==1)//1633
		picture->pixel_format = CEDARV_PIXEL_FORMAT_PLANNER_YVU420;

	//save minor stream id
	if(decoder->fbm_num == 2)
	{
		if(decoder->index[picture->id] == 0)
		{
			decoder->index[picture->id] = minor_frame->id;
		}
		else
		{
		}
	}
	picture->pts                    = frame->pts;
	picture->pcr                    = frame->pcr;

	picture->size_y					= frame->size_y;
	picture->size_u					= frame->size_u;
	picture->size_v					= frame->size_v;
	picture->size_alpha				= (u32)frame->alpha;
    
	picture->flag_stride             = frame->flag_stride;
	picture->maf_valid               = frame->maf_valid;
	picture->pre_frame_valid         = frame->pre_frame_valid;
	if(decoder->fbm_num != 2)
	{
		picture->size_y2			= frame->size_y2;
		picture->size_u2			= frame->size_u2;
		picture->size_v2			= frame->size_v2;
		picture->size_alpha2		= (u32)frame->alpha2;
	}
	else
	{
		picture->size_y2			= minor_frame->size_y;
		picture->size_u2			= minor_frame->size_u;
		picture->size_v2			= minor_frame->size_v;
		picture->size_alpha2		= (u32)minor_frame->alpha;
	}
    
	//* the CedarX player need we return physic address of display frame.
	if(decoder->dy_rotate_flag == 1)
	{
		picture->y                      = decoder->rot_img_y[decoder->cur_img_flag];
		picture->u                      = decoder->rot_img_c[decoder->cur_img_flag];
	}else{
		picture->flag_addr               = (u32)mem_get_phy_addr((u32)frame->flag_addr);
		picture->y                      = (u8*)mem_get_phy_addr((u32)frame->y);
		if(decoder->config_info.yv12_output_enable==1)
		{
			picture->v 					= picture->y + picture->size_y;
			picture->u					= picture->v + picture->size_v;
		}
		else
		{
			picture->u                  = (u8*)mem_get_phy_addr((u32)frame->u);
			picture->v					= (u8*)mem_get_phy_addr((u32)frame->v);
		}
		picture->alpha					= (u8*)mem_get_phy_addr((u32)frame->alpha);
	}
	if(decoder->fbm_num != 2)
	{
		picture->y2						= (u8*)mem_get_phy_addr((u32)frame->y2);
		if(decoder->config_info.yv32_output_enable==1 && decoder->config_info.yv32_transform_on_yv12==1)
		{
			picture->u2					= picture->y2 + picture->size_y2;
			picture->v2					= picture->u2 + picture->size_u2/2;
		}else
		{
			picture->u2						= (u8*)mem_get_phy_addr((u32)frame->u2);
			picture->v2						= (u8*)mem_get_phy_addr((u32)frame->v2);
			picture->alpha2					= (u8*)mem_get_phy_addr((u32)frame->alpha2);
		}
	}
	else
	{

		picture->y2					= (u8*)mem_get_phy_addr((u32)minor_frame->y);
		picture->u2					= (u8*)mem_get_phy_addr((u32)minor_frame->u);
		picture->v2 				= (u8*)mem_get_phy_addr((u32)minor_frame->v);
		picture->alpha2 			= (u8*)mem_get_phy_addr((u32)minor_frame->alpha2);
	}


#if 1	//* add by chenxiaochuan for MJPEG YUV420 picture transformed from YUV422.
	if(picture->pixel_format == CEDARV_PIXEL_FORMAT_PLANNER_YVU420 &&
	   decoder->dy_rotate_flag == 0)
	{
		int widthAlign, heightAlign;
		int ysize, csize;
		widthAlign = (picture->display_width + 15) & ~15;
		heightAlign = (picture->display_height + 15) & ~15;
		ysize = widthAlign * heightAlign;
		picture->v = picture->y + ysize;
		csize = ysize>>2;
		picture->u = picture->v + csize;
	}
#endif

#ifdef SAVE_FRAME_DATA
	num++;
	if(picture->pixel_format == CEDARV_PIXEL_FORMAT_PLANNER_YVU420)
	{
		if(num == 100)
		{
			f1y = fopen("/data/camera/yv12_frame.dat","wb");
			fwrite(frame->y,1,frame->size_y+2*frame->size_u, f1y);
			fclose(f1y);
		}
	}else
	{
		if(num == 100)
		{
			f1y = fopen("/data/camera/y1.dat","wb");
			f1u = fopen("/data/camera/u1.dat","wb");
			f1v = fopen("/data/camera/v1.dat","wb");
			f2y = fopen("/data/camera/y2.dat","wb");
			f2u = fopen("/data/camera/u2.dat","wb");
			f2v = fopen("/data/camera/v2.dat","wb");
			fwrite(frame->y,1,frame->size_y, f1y);
			fwrite(frame->u,1,frame->size_u, f1u);
			fwrite(frame->v,1,frame->size_v, f1v);
			if(decoder->vbv_num != 2){
				fwrite(frame->y2, 1, frame->size_y2, f2y);
				fwrite(frame->u2, 1, frame->size_u2, f2u);
				fwrite(frame->v2, 1, frame->size_v2, f2v);
			}
			else
			{
				fwrite(minor_frame->y, 1, minor_frame->size_y, f2y);
				fwrite(minor_frame->u, 1, minor_frame->size_u, f2u);
				fwrite(minor_frame->v, 1, minor_frame->size_v, f2v);
			}
			fclose(f1y);
			fclose(f1u);
			fclose(f1v);
			fclose(f2y);
			fclose(f2u);
			fclose(f2v);
		}
	}
#endif

	if(decoder->display_already_begin == 0)
		decoder->display_already_begin = 1;

	return CEDARV_RESULT_OK;
}


static s32 vdecoder_display_release(cedarv_decoder_t* p, u32 frame_index)
{
	vpicture_t*      frame;
	video_decoder_t* decoder;
	u32				minor_frame_index;
	if(p == NULL)
		return CEDARV_RESULT_ERR_INVALID_PARAM;

	decoder = (video_decoder_t*)p;

	if(decoder == NULL || decoder->fbm == NULL)
		return CEDARV_RESULT_ERR_FAIL;

	frame = fbm_index_to_pointer(frame_index, decoder->fbm);
	if(frame == NULL)
		return CEDARV_RESULT_ERR_FAIL;

	frame->anaglath_transform_mode = ANAGLAGH_NONE;
	fbm_display_return_frame(frame, decoder->fbm);

	if(decoder->fbm_num == 2)
	{
		if(decoder->minor_fbm == NULL)
			return CEDARV_RESULT_ERR_FAIL;

		minor_frame_index = decoder->index[frame_index];
		decoder->index[frame_index] = 0;
		frame = fbm_index_to_pointer(minor_frame_index, decoder->minor_fbm);
		if(frame == NULL)
			return CEDARV_RESULT_ERR_FAIL;

		fbm_display_return_frame(frame, decoder->minor_fbm);
	}

	if(p->release_frame_buffer_sem != NULL)
		p->release_frame_buffer_sem(p->cedarx_cookie);

	return CEDARV_RESULT_OK;
}


static s32 vdecoder_set_video_bitstream_info(cedarv_decoder_t* p, cedarv_stream_info_t* info)
{
	video_decoder_t*      decoder;

	decoder = (video_decoder_t*)p;

	if(p == NULL || info == NULL)
		return CEDARV_RESULT_ERR_INVALID_PARAM;

	set_format(&decoder->stream_info, info);
	set_3d_mode_to_decoder(&decoder->stream_info, info);

	decoder->stream_info.video_width      = info->video_width;
	decoder->stream_info.video_height     = info->video_height;
	decoder->stream_info.frame_rate       = info->frame_rate;
	decoder->stream_info.frame_duration   = info->frame_duration;
	decoder->stream_info.aspec_ratio      = info->aspect_ratio;
	decoder->stream_info.is_pts_correct   = info->is_pts_correct;
	decoder->stream_info.init_data_len    = info->init_data_len;

	if(info->init_data_len > 0)
	{
		decoder->stream_info.init_data = (u8*)mem_alloc(decoder->stream_info.init_data_len);
		if(decoder->stream_info.init_data == NULL)
			return CEDARV_RESULT_ERR_NO_MEMORY;

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

	if((p->video_stream_type == CDX_VIDEO_STREAM_MAJOR) || (decoder->vbv_num != 2))
	{
		vq->vbv_buffer_usage = vbv_get_valid_data_size(decoder->vbv) * 100 / vbv_get_buffer_size(decoder->vbv);
		vq->frame_num_in_vbv = vbv_get_valid_frame_num(decoder->vbv);
	}
	else if(p->video_stream_type == CDX_VIDEO_STREAM_MINOR)
	{
		vq->vbv_buffer_usage = vbv_get_valid_data_size(decoder->minor_vbv) * 100/vbv_get_buffer_size(decoder->minor_vbv);
		vq->frame_num_in_vbv = vbv_get_valid_frame_num(decoder->minor_vbv);
	}

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
	else if(cedarv_stream_info->sub_format == CEDARV_MPEG4_SUB_FORMAT_DIVX1)//MSMPEGV1
	{
		vstream_info->sub_format = MPEG4_SUB_FORMAT_DIVX1;
	}
	else if(cedarv_stream_info->sub_format == CEDARV_MPEG4_SUB_FORMAT_DIVX2)//MSMPEGV2
	{
		vstream_info->sub_format = MPEG4_SUB_FORMAT_DIVX2;
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
	else if(cedarv_stream_info->container_format == CEDARV_CONTAINER_FORMAT_OGM)
	{
		vstream_info->container_format = CONTAINER_FORMAT_OGM;
	}
	else if(cedarv_stream_info->container_format == CEDARV_CONTAINER_FORMAT_RAW)
	{
		vstream_info->container_format = CONTAINER_FORMAT_RAW;
	}
	else
	{
		vstream_info->container_format = CONTAINER_FORMAT_UNKNOW;
	}

	return;
}

void set_3d_mode_to_decoder(vstream_info_t* vstream_info, cedarv_stream_info_t* cedarv_stream_info)
{
	if(cedarv_stream_info->_3d_mode == CEDARV_3D_MODE_DOUBLE_STREAM)
	{
		vstream_info->_3d_mode = _3D_MODE_DOUBLE_STREAM;
	}
	else if(cedarv_stream_info->_3d_mode == CEDARV_3D_MODE_SIDE_BY_SIDE)
	{
		vstream_info->_3d_mode = _3D_MODE_SIDE_BY_SIDE;
	}
	else if(cedarv_stream_info->_3d_mode == CEDARV_3D_MODE_TOP_TO_BOTTOM)
	{
		vstream_info->_3d_mode = _3D_MODE_TOP_TO_BOTTOM;
	}
	else if(cedarv_stream_info->_3d_mode == CEDARV_3D_MODE_LINE_INTERLEAVE)
	{
		vstream_info->_3d_mode = _3D_MODE_LINE_INTERLEAVE;
	}
	else if(cedarv_stream_info->_3d_mode == CEDARV_3D_MODE_COLUME_INTERLEAVE)
	{
		vstream_info->_3d_mode = _3D_MODE_COLUME_INTERLEAVE;
	}
	else
	{
		//* original anaglagh pictures are handled as 2D pictures.
		vstream_info->_3d_mode = _3D_MODE_NONE;
	}
}


static void vdecoder_set_vbv_vedec_memory(video_decoder_t* p)
{   
    u32 vbv_size_rank[4] = {8, 10, 12, 16};
    u32 memorySize = 0;
    u32 rank_index = 0;
    
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
    
	if((p->remainMemorySize==0)||(p->remainMemorySize>=64*1024*1024))
	{
		//set vbv size as 10M for M2TS.
		if(p->stream_info._3d_mode==_3D_MODE_DOUBLE_STREAM)
			p->max_vbv_buffer_size  = 12*1024*1024;
		else if(p->stream_info.video_height >= 2048)
			p->max_vbv_buffer_size = 12*1024*1024;
		else if(p->stream_info.video_height >= 720)
			p->max_vbv_buffer_size = 10*1024*1024;
        
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


static void set_cedarv_format(cedarv_stream_info_t* cedarv_stream_info, vstream_info_t* vstream_info)
{
    if(vstream_info->format == STREAM_FORMAT_MPEG2)
	{
		cedarv_stream_info->format = CEDARV_STREAM_FORMAT_MPEG2;
	}
	else if(vstream_info->format == STREAM_FORMAT_MPEG4)
	{
		cedarv_stream_info->format = CEDARV_STREAM_FORMAT_MPEG4;
	}
	else if(vstream_info->format == STREAM_FORMAT_REALVIDEO)
	{
		cedarv_stream_info->format = CEDARV_STREAM_FORMAT_REALVIDEO;
	}
	else if(vstream_info->format == STREAM_FORMAT_H264)
	{
        cedarv_stream_info->format = CEDARV_STREAM_FORMAT_H264;
	}
	else if(vstream_info->format == STREAM_FORMAT_VC1)
	{
        cedarv_stream_info->format = CEDARV_STREAM_FORMAT_VC1;
	}
	else if(vstream_info->format == STREAM_FORMAT_AVS)
	{
        cedarv_stream_info->format = CEDARV_STREAM_FORMAT_AVS;
	}
	else if(vstream_info->format == STREAM_FORMAT_MJPEG)
	{
        cedarv_stream_info->format = CEDARV_STREAM_FORMAT_MJPEG;
	}
    else if(vstream_info->format == STREAM_FORMAT_VP8)
    {
        cedarv_stream_info->format = CEDARV_STREAM_FORMAT_VP8;
    }
	else
	{
        cedarv_stream_info->format = CEDARV_STREAM_FORMAT_UNKNOW;
	}
	
	if(vstream_info->sub_format == STREAM_SUB_FORMAT_UNKNOW)
	{
		cedarv_stream_info->sub_format = CEDARV_SUB_FORMAT_UNKNOW;
	}
	else if(vstream_info->sub_format == MPEG2_SUB_FORMAT_MPEG1)
	{
		cedarv_stream_info->sub_format = CEDARV_MPEG2_SUB_FORMAT_MPEG1;
	}
	else if(vstream_info->sub_format == MPEG2_SUB_FORMAT_MPEG2)
	{
		cedarv_stream_info->sub_format = CEDARV_MPEG2_SUB_FORMAT_MPEG2;
	}
	else if(vstream_info->sub_format == MPEG4_SUB_FORMAT_XVID)
	{
		cedarv_stream_info->sub_format = CEDARV_MPEG4_SUB_FORMAT_XVID;
	}
	else if(vstream_info->sub_format == MPEG4_SUB_FORMAT_DIVX3)
	{
		cedarv_stream_info->sub_format = CEDARV_MPEG4_SUB_FORMAT_DIVX3;
	}
	else if(vstream_info->sub_format == MPEG4_SUB_FORMAT_DIVX4)
	{
		cedarv_stream_info->sub_format = CEDARV_MPEG4_SUB_FORMAT_DIVX4;
	}
	else if(vstream_info->sub_format == MPEG4_SUB_FORMAT_DIVX5)
	{
		cedarv_stream_info->sub_format = CEDARV_MPEG4_SUB_FORMAT_DIVX5;
	}
	else if(vstream_info->sub_format == MPEG4_SUB_FORMAT_SORENSSON_H263)
	{
		cedarv_stream_info->sub_format = CEDARV_MPEG4_SUB_FORMAT_SORENSSON_H263;
	}
	else if(vstream_info->sub_format == MPEG4_SUB_FORMAT_H263)
	{
		cedarv_stream_info->sub_format = CEDARV_MPEG4_SUB_FORMAT_H263;
	}
	else if(vstream_info->sub_format == MPEG4_SUB_FORMAT_RMG2)
	{
		cedarv_stream_info->sub_format = CEDARV_MPEG4_SUB_FORMAT_RMG2;
	}
	else if(vstream_info->sub_format == MPEG4_SUB_FORMAT_VP6)
	{
		cedarv_stream_info->sub_format = CEDARV_MPEG4_SUB_FORMAT_VP6;
	}
	else if(vstream_info->sub_format == MPEG4_SUB_FORMAT_WMV1)
	{
		cedarv_stream_info->sub_format = CEDARV_MPEG4_SUB_FORMAT_WMV1;
	}
	else if(vstream_info->sub_format == MPEG4_SUB_FORMAT_WMV2)
	{
		cedarv_stream_info->sub_format = CEDARV_MPEG4_SUB_FORMAT_WMV2;
	}
	else
	{
        cedarv_stream_info->sub_format = CEDARV_SUB_FORMAT_UNKNOW;
	}
	
	if(vstream_info->container_format == CONTAINER_FORMAT_UNKNOW)
	{
		cedarv_stream_info->container_format = CEDARV_CONTAINER_FORMAT_UNKNOW;
	}
	else if(vstream_info->container_format == CONTAINER_FORMAT_AVI)
	{
		cedarv_stream_info->container_format = CEDARV_CONTAINER_FORMAT_AVI;
	}
	else if(vstream_info->container_format == CONTAINER_FORMAT_ASF)
	{
		cedarv_stream_info->container_format = CEDARV_CONTAINER_FORMAT_ASF;
	}
	else if(vstream_info->container_format == CONTAINER_FORMAT_DAT)
	{
		cedarv_stream_info->container_format = CEDARV_CONTAINER_FORMAT_DAT;
	}
	else if(vstream_info->container_format == CONTAINER_FORMAT_FLV)
	{
		cedarv_stream_info->container_format = CEDARV_CONTAINER_FORMAT_FLV;
	}
	else if(vstream_info->container_format == CONTAINER_FORMAT_MKV)
	{
		cedarv_stream_info->container_format = CEDARV_CONTAINER_FORMAT_MKV;
	}
	else if(vstream_info->container_format == CONTAINER_FORMAT_MOV)
	{
		cedarv_stream_info->container_format = CEDARV_CONTAINER_FORMAT_MOV;
	}
	else if(vstream_info->container_format == CONTAINER_FORMAT_MPG)
	{
		cedarv_stream_info->container_format = CEDARV_CONTAINER_FORMAT_MPG;
	}
	else if(vstream_info->container_format == CONTAINER_FORMAT_PMP)
	{
		cedarv_stream_info->container_format = CEDARV_CONTAINER_FORMAT_PMP;
	}
	else if(vstream_info->container_format == CONTAINER_FORMAT_RM)
	{
		cedarv_stream_info->container_format = CEDARV_CONTAINER_FORMAT_RM;
	}
	else if(vstream_info->container_format == CONTAINER_FORMAT_TS)
	{
		cedarv_stream_info->container_format = CEDARV_CONTAINER_FORMAT_TS;
	}
	else if(vstream_info->container_format == CONTAINER_FORMAT_VOB)
	{
		cedarv_stream_info->container_format = CEDARV_CONTAINER_FORMAT_VOB;
	}
	else
	{
        cedarv_stream_info->container_format = CEDARV_CONTAINER_FORMAT_UNKNOW;
	}
	
	return;
}

static s32 set_cedarv_stream_info(cedarv_stream_info_t *cedarv_stream_info, vstream_info_t *vstream_info)
{
    mem_set(cedarv_stream_info, 0, sizeof(cedarv_stream_info_t));

    set_cedarv_format(cedarv_stream_info, vstream_info);

    cedarv_stream_info->video_width      = vstream_info->video_width;
    cedarv_stream_info->video_height     = vstream_info->video_height;
    cedarv_stream_info->frame_rate       = vstream_info->frame_rate;
    cedarv_stream_info->frame_duration   = vstream_info->frame_duration;
    cedarv_stream_info->aspect_ratio     = vstream_info->aspec_ratio;
    cedarv_stream_info->is_pts_correct   = vstream_info->is_pts_correct;
    cedarv_stream_info->_3d_mode		 = (cedarv_3d_mode_e)vstream_info->_3d_mode;
    cedarv_stream_info->init_data_len    = vstream_info->init_data_len;
    cedarv_stream_info->init_data    	 = vstream_info->init_data;

    return CEDARV_RESULT_OK;
}

