/*
********************************************************************************
*                                    eMOD
*                   the Easy Portable/Player Develop Kits
*                               mod_cedar sub-system
*                                   vdrv_mp4 module
*
*          (c) Copyright 2009-2010, Allwinner Microelectronic Co., Ltd.
*                              All Rights Reserved
*
* File   : vdrv_mpeg4_oal.c
* Version: V1.0
* By     : Eric_wang
* Date   : 2009-10-27
********************************************************************************
*/
#include "vdecoder_config.h"
#include "vdecoder.h"

s32 vdrv_priolevel = KRNL_priolevel2;
s32 waitSemFrameBufTimeOut = 10;
s32 play_mode = MEDIA_STATUS_STOP;
__hdle hve = 0;

__s32 set_vformat(__video_codec_format_t* stream_info_oal, cedarv_stream_info_t *stream_info)
{
    //cedarv_stream_info_t    stream_info;
    //__video_codec_format_t* stream_info_oal;
    memset(stream_info_oal, 0, sizeof(__video_codec_format_t));
    switch(stream_info->format)
    {
        case CEDARV_STREAM_FORMAT_MPEG2:
        {
            if(stream_info->sub_format == CEDARV_MPEG2_SUB_FORMAT_MPEG1)
            {
                stream_info_oal->codec_type = VIDEO_MPEG1_ES;
            }
            else
            {
                stream_info_oal->codec_type = VIDEO_MPEG2_ES;
            }
            break;
        }
        case CEDARV_STREAM_FORMAT_MPEG4:
        {
            if(stream_info->sub_format == CEDARV_MPEG4_SUB_FORMAT_XVID)
                stream_info_oal->codec_type = VIDEO_XVID;
            else if(stream_info->sub_format == CEDARV_MPEG4_SUB_FORMAT_DIVX3)
                stream_info_oal->codec_type = VIDEO_DIVX3;
            else if(stream_info->sub_format == CEDARV_MPEG4_SUB_FORMAT_DIVX4)
                stream_info_oal->codec_type = VIDEO_DIVX4;
            else if(stream_info->sub_format == CEDARV_MPEG4_SUB_FORMAT_DIVX5)
                stream_info_oal->codec_type = VIDEO_DIVX5;
            else if(stream_info->sub_format == CEDARV_MPEG4_SUB_FORMAT_SORENSSON_H263)
                stream_info_oal->codec_type = VIDEO_SORENSSON_H263;
            else if(stream_info->sub_format == CEDARV_MPEG4_SUB_FORMAT_RMG2)
                stream_info_oal->codec_type = VIDEO_RMG2;
            else if(stream_info->sub_format == CEDARV_MPEG4_SUB_FORMAT_H263)
                stream_info_oal->codec_type = VIDEO_H263;
            else if(stream_info->sub_format == CEDARV_MPEG4_SUB_FORMAT_VP6)
                stream_info_oal->codec_type = VIDEO_VP6;
            else if(stream_info->sub_format == CEDARV_MPEG4_SUB_FORMAT_WMV1)
                stream_info_oal->codec_type = VIDEO_WMV1;
            else if(stream_info->sub_format == CEDARV_MPEG4_SUB_FORMAT_WMV2)
                stream_info_oal->codec_type = VIDEO_WMV2;
            else
                stream_info_oal->codec_type = VIDEO_UNKNOWN;

            break;
        }
        case CEDARV_STREAM_FORMAT_REALVIDEO:
        {
            stream_info_oal->codec_type = VIDEO_RMVB8;
            break;
        }
        case CEDARV_STREAM_FORMAT_H264:
        {
            stream_info_oal->codec_type = VIDEO_H264;
            break;
        }
        case CEDARV_STREAM_FORMAT_VC1:
        {
            stream_info_oal->codec_type = VIDEO_VC1;
            break;
        }
        case CEDARV_STREAM_FORMAT_MJPEG:
        {
            stream_info_oal->codec_type = VIDEO_MJPEG;
            break;
        }
        case CEDARV_STREAM_FORMAT_AVS:
        {
            stream_info_oal->codec_type = VIDEO_AVS;
            break;
        }
        default:
        {
            stream_info_oal->codec_type = VIDEO_UNKNOWN;
            break;
        }
    }

    stream_info_oal->width          = stream_info->video_width   ; 
    stream_info_oal->height         = stream_info->video_height  ; 
    stream_info_oal->frame_rate     = stream_info->frame_rate    ; 
    stream_info_oal->mic_sec_per_frm = stream_info->frame_duration; 
    stream_info_oal->priv_inf_len   = stream_info->init_data_len ; 
    stream_info_oal->private_inf    = stream_info->init_data     ; 
    return EPDK_OK;
}

static void ReleaseFrameBufferSem(void* cookie)
{
	ReleaseFrmBufSem();
	return;
}

static void FreeVbsBufferSem(void* cookie)
{
	VDrvFreeVbsBuffer();
}

void* VDecDev_MInit(s32 *ret)
{
    u8  i =0;
	cedarv_decoder_t* p;
	
    while(!hve)
    {
        hve = esSVC_ResourceReq(RESOURCE_VE_HW, RESOURCE_REQ_MODE_NWAIT, 0);
        
        if(!hve)
        {
            if(i >= 5)
            {
               return NULL;
            }
            i++;
            esKRNL_TimeDly(1);
        }
    }
	
	p = libcedarv_init(ret);
	
	if(p != NULL)
	{
		p->release_frame_buffer_sem = ReleaseFrameBufferSem;
		p->free_vbs_buffer_sem = FreeVbsBufferSem;
	}
	else
	{
		esSVC_ResourceRel(hve);
		hve = 0;
	}
	
    return p;
}

s16 VDecDev_MExit(void *pDev)     //struct VDEC_DEVICE *pDev
{
	s32 ret;
	
    ret = libcedarv_exit(pDev);
    
	if(hve != 0)
	{
		esSVC_ResourceRel(hve);
		hve = 0;
	}
	
	return ret;
}

s32 VDecDev_MIoctrl(void *pDecDev, s32 cmd, s32 aux, void *pbuffer)
{
    s32               result;
    cedarv_decoder_t* p;
    
    p = (cedarv_decoder_t*)pDecDev;

    switch(cmd)
    {
        case VDRV_INTERNAL_CMD_QUERY_FRAME:
        {
            cedarv_picture_t  picture;
            __display_info_t* tmpDispFrm = (__display_info_t *)pbuffer;
            
            result = p->display_request(p, &picture);
            
            if(result == 0)
            {
                tmpDispFrm->bProgressiveSrc          	 = picture.is_progressive;
                tmpDispFrm->bTopFieldFirst           	 = picture.top_field_first;
                
                if(picture.is_progressive)
                    tmpDispFrm->eRepeatField = REPEAT_FIELD_NONE;
                else if(picture.repeat_top_field)
                    tmpDispFrm->eRepeatField = REPEAT_FIELD_TOP;
                else if(picture.repeat_bottom_field)
                    tmpDispFrm->eRepeatField = REPEAT_FIELD_BOTTOM;
                else
                    tmpDispFrm->eRepeatField = REPEAT_FIELD_NONE;
                    
                if(picture.pixel_format == CEDARV_PIXEL_FORMAT_AW_YUV422)
                	tmpDispFrm->pVideoInfo.color_format  = PIC_CFMT_YCBCR_422;
                else if(picture.pixel_format == CEDARV_PIXEL_FORMAT_AW_YUV411)
                	tmpDispFrm->pVideoInfo.color_format  = PIC_CFMT_YCBCR_411;
                else
                	tmpDispFrm->pVideoInfo.color_format  = PIC_CFMT_YCBCR_420;
                	
                tmpDispFrm->pVideoInfo.frame_rate        = picture.frame_rate;//INVALID_FRAME_RATE;
                tmpDispFrm->pVideoInfo.eAspectRatio  	 = picture.aspect_ratio;
                tmpDispFrm->pPanScanInfo.uNumberOfOffset = 0;
                tmpDispFrm->horizontal_scale_ratio       = picture.horizontal_scale_ratio;
                tmpDispFrm->vertical_scale_ratio         = picture.vertical_scale_ratio;
                tmpDispFrm->src_rect.uStartX 			 = 0;
                tmpDispFrm->src_rect.uStartY 			 = 0;
                tmpDispFrm->src_rect.uWidth  		     = picture.width;
                tmpDispFrm->src_rect.uHeight 		     = picture.height;
                tmpDispFrm->dst_rect.uStartX 			 = picture.left_offset;
                tmpDispFrm->dst_rect.uStartY 			 = picture.top_offset;
                tmpDispFrm->dst_rect.uWidth  		     = picture.display_width;
                tmpDispFrm->dst_rect.uHeight 		     = picture.display_height;
                if(picture.pts == (u64)(-1))
                	tmpDispFrm->uPts = 0xffffffff;
                else
                	tmpDispFrm->uPts 						 = picture.pts/1000;
                tmpDispFrm->top_index        			 = picture.id;
                tmpDispFrm->top_y 						 = (u32)picture.y;
                tmpDispFrm->top_c 						 = (u32)picture.u;
                return EPDK_OK;
            }
            else
            {
                return EPDK_FAIL;
            }
        }
        
        case VDRV_INTERNAL_CMD_RELEASE_FRAME:
        {
            p->display_release(p, (u8)aux);
            return EPDK_OK;
        }
        
        case VDRV_INTERNAL_CMD_QUERY_VBSBUFFER:
        {
            __ring_bs_buf_t    *pbsbuf = (__ring_bs_buf_t*)pbuffer;
            
            result = p->request_write(p, 1024 * 512, &pbsbuf->pData0, &pbsbuf->uSizeGot0, &pbsbuf->pData1, &pbsbuf->uSizeGot1);
            if(result != CEDARV_RESULT_OK)
                return EPDK_FAIL;
            
            return EPDK_OK;
        }
        
        case VDRV_INTERNAL_CMD_SET_DATAINPUT:
        {
            cedarv_stream_data_info_t   data_info;
            __vdec_datctlblk_t*         data_info_oal;
            
            data_info_oal = (__vdec_datctlblk_t*)pbuffer;
            
            data_info.lengh = (u32)aux;
            
            data_info.flags = 0;
            if(data_info_oal->CtrlBits & FIRST_PART_CTRL)
                data_info.flags |= CEDARV_FLAG_FIRST_PART;
            
            if(data_info_oal->CtrlBits & LAST_PART_CTRL)
                data_info.flags |= CEDARV_FLAG_LAST_PART;
            
            if(data_info_oal->CtrlBits & PTS_VALID_CTRL)
            {
                data_info.flags |= CEDARV_FLAG_PTS_VALID;
                data_info.pts = data_info_oal->uPTS;
            }
            else
                data_info.pts = (u64)(-1);
            
            if(data_info_oal->uFrmDecMode != play_mode)
            	data_info.flags |= CEDARV_FLAG_DATA_INVALID;
            	
            result = p->update_data(p, &data_info);
            
            if(result != CEDARV_RESULT_OK)
                return EPDK_FAIL;
            
            return EPDK_OK;
        }
        
        case VDRV_INTERNAL_CMD_DATA_END:
        {
            return EPDK_OK;
        }
        
        case VDRV_INTERNAL_CMD_OPEN_DEV:
        {
            return EPDK_OK;
        }
        
        case VDRV_INTERNAL_CMD_OPEN:
        {
            result = p->open(p);
            if(result != CEDARV_RESULT_OK)
                return EPDK_FAIL;
            
            return EPDK_OK;
        }
        
        case VDRV_INTERNAL_CMD_CLOSE_DEV:
        {
            return EPDK_OK;
        }
        
        case VDRV_INTERNAL_CMD_CLOSE:
        {
            result = p->close(p);
            if(result != CEDARV_RESULT_OK)
                return EPDK_FAIL;
            
            return EPDK_OK;
        }
        case VDRV_INTERNAL_CMD_PREVIEW:
        {
            result = p->ioctrl(p, CEDARV_COMMAND_PREVIEW_MODE, 0);
            if(result != CEDARV_RESULT_OK)
                return EPDK_FAIL;
            
            play_mode = MEDIA_STATUS_PLAY;
            return EPDK_OK;
        }
        case VDRV_INTERNAL_CMD_PLAY:
        {
            result = p->ioctrl(p, CEDARV_COMMAND_PLAY, 0);
            if(result != CEDARV_RESULT_OK)
                return EPDK_FAIL;
            
            play_mode = MEDIA_STATUS_PLAY;
            return EPDK_OK;
        }
        
        case VDRV_INTERNAL_CMD_STOP:
        {
            result = p->ioctrl(p, CEDARV_COMMAND_STOP, 0);
            if(result != CEDARV_RESULT_OK)
                return EPDK_FAIL;
            
            play_mode = MEDIA_STATUS_STOP;
            return EPDK_OK;
        }
        
        case VDRV_INTERNAL_CMD_FF:
        {
            result = p->ioctrl(p, CEDARV_COMMAND_FORWARD, 0);
            if(result != CEDARV_RESULT_OK)
                return EPDK_FAIL;
            
            play_mode = MEDIA_STATUS_FORWARD;
            return EPDK_OK;
        }
        
        case VDRV_INTERNAL_CMD_RR:
        {
            result = p->ioctrl(p, CEDARV_COMMAND_BACKWARD, 0);
            if(result != CEDARV_RESULT_OK)
                return EPDK_FAIL;
            
            play_mode = MEDIA_STATUS_BACKWARD;
            return EPDK_OK;
        }
        
        case VDRV_INTERNAL_CMD_SET_VBSINF:
        {
            cedarv_stream_info_t    stream_info;
            __video_codec_format_t* stream_info_oal;
            
            stream_info_oal = (__video_codec_format_t*)pbuffer;
            
            mem_set(&stream_info, 0, sizeof(cedarv_stream_info_t));
    
            switch(stream_info_oal->codec_type)
            {
                case VIDEO_MPEG1_ES:
                case VIDEO_MPEG2_ES:
                {
                    stream_info.format 				= CEDARV_STREAM_FORMAT_MPEG2;
                    stream_info.container_format	= CEDARV_CONTAINER_FORMAT_UNKNOW;
                    if(stream_info_oal->codec_type == VIDEO_MPEG1_ES)
                    	stream_info.sub_format = CEDARV_MPEG2_SUB_FORMAT_MPEG1;
                    else
                    	stream_info.sub_format = CEDARV_MPEG2_SUB_FORMAT_MPEG2;
                    break;
                }
                case VIDEO_XVID:
                case VIDEO_DIVX3:
                case VIDEO_DIVX4:
                case VIDEO_DIVX5:
                case VIDEO_SORENSSON_H263:
                case VIDEO_RMG2:
                case VIDEO_H263:
                case VIDEO_VP6:
                case VIDEO_WMV1:
                case VIDEO_WMV2:
                {
                    stream_info.format 				= CEDARV_STREAM_FORMAT_MPEG4;
                    stream_info.container_format	= CEDARV_CONTAINER_FORMAT_UNKNOW;
                    if(stream_info_oal->video_bs_src == CEDARLIB_FILE_FMT_ASF)
                    	stream_info.container_format = CEDARV_CONTAINER_FORMAT_ASF;
                    
                    if(stream_info_oal->codec_type == VIDEO_XVID)
                    	stream_info.sub_format = CEDARV_MPEG4_SUB_FORMAT_XVID;
                    else if(stream_info_oal->codec_type == VIDEO_DIVX3)
                    	stream_info.sub_format = CEDARV_MPEG4_SUB_FORMAT_DIVX3;
                    else if(stream_info_oal->codec_type == VIDEO_DIVX4)
                    	stream_info.sub_format = CEDARV_MPEG4_SUB_FORMAT_DIVX4;
                    else if(stream_info_oal->codec_type == VIDEO_DIVX5)
                    	stream_info.sub_format = CEDARV_MPEG4_SUB_FORMAT_DIVX5;
                    else if(stream_info_oal->codec_type == VIDEO_SORENSSON_H263)
                    {
                    	if(stream_info_oal->video_bs_src == CEDARLIB_FILE_FMT_FLV)
                    		stream_info.container_format = CEDARV_CONTAINER_FORMAT_FLV;
                    	else
                    		stream_info.container_format = CEDARV_CONTAINER_FORMAT_UNKNOW;
                    	stream_info.sub_format = CEDARV_MPEG4_SUB_FORMAT_SORENSSON_H263;
                    }
                    else if(stream_info_oal->codec_type == VIDEO_RMG2)
                    {
                    	stream_info.sub_format = CEDARV_MPEG4_SUB_FORMAT_RMG2;
                    	stream_info_oal->priv_inf_len = sizeof(__vdec_rmg2_init_info);
                    	stream_info_oal->private_inf  = &((__vdec_mpeg4_vbs_inf_t*)(stream_info_oal))->vRmg2Inf;
                    }
                    else if(stream_info_oal->codec_type == VIDEO_H263)
                    	stream_info.sub_format = CEDARV_MPEG4_SUB_FORMAT_H263;
                    else if(stream_info_oal->codec_type == VIDEO_VP6)
                    {
                    	stream_info.sub_format = CEDARV_MPEG4_SUB_FORMAT_VP6;
                    	if(stream_info_oal->video_bs_src == CEDARLIB_FILE_FMT_FLV)
                    		stream_info.container_format = CEDARV_CONTAINER_FORMAT_FLV;
                    	else
                    		stream_info.container_format = CEDARV_CONTAINER_FORMAT_UNKNOW;
                    }
                    else if(stream_info_oal->codec_type == VIDEO_WMV1)
                    	stream_info.sub_format = CEDARV_MPEG4_SUB_FORMAT_WMV1;
                    else if(stream_info_oal->codec_type == VIDEO_WMV2)
                    	stream_info.sub_format = CEDARV_MPEG4_SUB_FORMAT_WMV2;
                    else
                    	stream_info.sub_format = CEDARV_SUB_FORMAT_UNKNOW;
                    	
                    break;
                }
                case VIDEO_RMVB8:
                case VIDEO_RMVB9:
                {
                    stream_info.format 					= CEDARV_STREAM_FORMAT_REALVIDEO;
                	stream_info.sub_format 				= CEDARV_SUB_FORMAT_UNKNOW;
                    stream_info.container_format 		= CEDARV_CONTAINER_FORMAT_UNKNOW;
                    break;
                }
                case VIDEO_H264:
                {
                    stream_info.format 					= CEDARV_STREAM_FORMAT_H264;
                	stream_info.sub_format 				= CEDARV_SUB_FORMAT_UNKNOW;
                    if(stream_info_oal->video_bs_src == CEDARLIB_FILE_FMT_TS)
                        stream_info.container_format 	= CEDARV_CONTAINER_FORMAT_TS;
                    else
                        stream_info.container_format 	= CEDARV_CONTAINER_FORMAT_UNKNOW;
                        
                    break;
                }
                
                case VIDEO_VC1:
                {
                    stream_info.format 					= CEDARV_STREAM_FORMAT_VC1;
                	stream_info.sub_format 				= CEDARV_SUB_FORMAT_UNKNOW;
                    if(stream_info_oal->video_bs_src == CEDARLIB_FILE_FMT_TS)
                        stream_info.container_format 	= CEDARV_CONTAINER_FORMAT_TS;
                    else
                        stream_info.container_format 	= CEDARV_CONTAINER_FORMAT_UNKNOW;
                    break;
                }
                
                case VIDEO_MJPEG:
                {
                	stream_info.format 				= CEDARV_STREAM_FORMAT_MJPEG;
                	stream_info.sub_format 			= CEDARV_SUB_FORMAT_UNKNOW;
                	stream_info.container_format 	= CEDARV_CONTAINER_FORMAT_UNKNOW;
                	break;
                }
                
                case VIDEO_AVS:
                case VIDEO_JPEG:
                default:
                {
                    stream_info.format = CEDARV_STREAM_FORMAT_UNKNOW;
                    return EPDK_FAIL;
                }
            }
            
            if(stream_info.container_format == CEDARV_CONTAINER_FORMAT_TS)
    	        stream_info.is_pts_correct = 1;
            else
    	        stream_info.is_pts_correct = 0;

            stream_info.video_width    = stream_info_oal->width;
            stream_info.video_height   = stream_info_oal->height;
            stream_info.frame_rate     = stream_info_oal->frame_rate;
            stream_info.frame_duration = stream_info_oal->mic_sec_per_frm;
            stream_info.aspect_ratio   = 1000;
            stream_info.init_data_len  = stream_info_oal->priv_inf_len;
            stream_info.init_data      = stream_info_oal->private_inf;
            result = p->set_vstream_info(p, &stream_info);
            if(result != CEDARV_RESULT_OK)
                return EPDK_FAIL;
            
            return EPDK_OK;
        }
        
        case VDRV_INTERNAL_CMD_DECODE:
        {
            result = p->decode(p);
            switch(result)
            {
                case CEDARV_RESULT_OK:
                case CEDARV_RESULT_FRAME_DECODED:
                {
                    return VDRV_PIC_DEC_ERR_NONE;
                }
                case CEDARV_RESULT_KEYFRAME_DECODED:
                {
                    return VDRV_PIC_DEC_I_FRAME;
                }
                case CEDARV_RESULT_NO_FRAME_BUFFER:
                {
                    return VDRV_PIC_DEC_ERR_NO_FRAMEBUFFER;
                }
                case CEDARV_RESULT_NO_BITSTREAM:
                {
                    return VDRV_PIC_DEC_ERR_VBS_UNDERFLOW;
                }
                case CEDARV_RESULT_ERR_FAIL:
                {
                    return VDRV_PIC_DEC_ERR_FRMNOTDEC;
                }
                default:
                {
                    return VDRV_PIC_DEC_ERR_VFMT_ERR;
                }
            }
        }
        
        case VDRV_INTERNAL_CMD_QUERY_STATUS_CHANGE_PROGRESS:
        {
            return VDRV_TRUE;
        }
        
        case VDRV_INTERNAL_CMD_SET_STATUS_CHANGE_PROGRESS:
        {
            return EPDK_OK;
        }
        
        case VDRV_INTERNAL_CMD_SET_ROTATE:
        {
            result = p->ioctrl(p, CEDARV_COMMAND_ROTATE, aux);
            if(result != CEDARV_RESULT_OK)
            {
                return EPDK_FAIL;
            }
            else
            {
                return EPDK_OK;
            }
        }
        
        case VDRV_INTERNAL_CMD_QUERY_VBSBUFFER_USAGE:
        {
            cedarv_quality_t vq;
            
            vq.vbv_buffer_usage = 0;
            vq.frame_num_in_vbv = 0;
            
            result = p->query_quality(p, &vq);
            
            return vq.vbv_buffer_usage;
        }
        
        case VDRV_INTERNAL_CMD_SET_TOTALMEMSIZE:
        {
            p->ioctrl(p, CEDARV_COMMAND_SET_TOTALMEMSIZE, aux);
            return EPDK_OK;
        }
        case VDRV_INTERNAL_CMD_RESET:
        {
            result = p->ioctrl(p, CEDARV_COMMAND_RESET, 0);
            if(result != CEDARV_RESULT_OK)
            {
                return EPDK_FAIL;
            }
            else
            {
                return EPDK_OK;
            }
        }
        case VDRV_INTERNAL_CMD_GET_STREAM_INFO:
        {
            //vstream_info_t vstream_info;
            cedarv_stream_info_t    stream_info;
            __video_codec_format_t* pvformat = (__video_codec_format_t*)pbuffer;
            result = p->ioctrl(p, CEDARV_COMMAND_GET_STREAM_INFO, (u32)&stream_info);
            if(result != CEDARV_RESULT_OK)
            {
                return EPDK_FAIL;
            }
            else
            {
                set_vformat(pvformat, &stream_info);
                return EPDK_OK;
            }
        }
        case VDRV_INTERNAL_CMD_GET_TAG:
        {   
            __cedar_tag_inf_t *pTag = (__cedar_tag_inf_t*)pbuffer;
            result = p->ioctrl(p, CEDARV_COMMAND_GET_SEQUENCE_INFO, (u32)pTag->decinf);
            return result;
        }
        case VDRV_INTERNAL_CMD_SET_TAG:
        {   
            __cedar_tag_inf_t *pTag = (__cedar_tag_inf_t*)pbuffer;
            result = p->ioctrl(p, CEDARV_COMMAND_SET_SEQUENCE_INFO, (u32)pTag->decinf);
            return result;
        }
        case VDRV_INTERNAL_CMD_CHKSPT:
        {
            //因为在psr_video已经把不支持的编码格式过滤了,而从19开始,解码驱动是allinone的,所以可以不判断了.
            return EPDK_TRUE;
        }
        default:
        {
            WARNING("Unknown cmd[%x]\n", cmd);
            return EPDK_FAIL;
        }
    }
}

int cedarv_f23_ic_version()
{
	return 1;
}

