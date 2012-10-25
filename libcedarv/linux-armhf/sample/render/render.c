/*******************************************************************************
--                                                                            --
--                    CedarX Multimedia Framework                             --
--                                                                            --
--          the Multimedia Framework for Linux/Android System                 --
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                         Softwinner Products.                               --
--                                                                            --
--                   (C) COPYRIGHT 2011 SOFTWINNER PRODUCTS                   --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
*******************************************************************************/

#include "render.h"
#include "avheap.h"

VIDEO_RENDER_CONTEXT_TYPE *hnd_video_render;

unsigned long args[4];
int image_width,image_height;


static int config_de_parameter(unsigned int width, unsigned int height, __disp_pixel_fmt_t format) {
	__disp_layer_info_t tmpLayerAttr;
	int ret;

	image_width = width;
	image_height = height;

	args[0] = 0;
	args[1] = hnd_video_render->de_layer_hdl;
	args[2] = (unsigned long)(&tmpLayerAttr);
	ioctl(hnd_video_render->de_fd, DISP_CMD_LAYER_GET_PARA,args);
	printf("tmpLayerAttr.scn_win.x %d, tmpLayerAttr.scn_win.y %d, tmpLayerAttr.scn_win.width %d, tmpLayerAttr.scn_win.height %d\n", tmpLayerAttr.scn_win.x, tmpLayerAttr.scn_win.y, tmpLayerAttr.scn_win.width, tmpLayerAttr.scn_win.height);
	//set color space
	if (image_height < 720) {
		tmpLayerAttr.fb.cs_mode = DISP_BT601;
	} else {
		tmpLayerAttr.fb.cs_mode = DISP_BT709;
	}
	//tmpFrmBuf.fmt.type = FB_TYPE_YUV;
	printf("format== DISP_FORMAT_ARGB8888 %d\n", format == DISP_FORMAT_ARGB8888 ? 1 : 0);
	if(format == DISP_FORMAT_ARGB8888){
		tmpLayerAttr.fb.mode 	= DISP_MOD_NON_MB_PLANAR;
		tmpLayerAttr.fb.format 	= format; //
		tmpLayerAttr.fb.br_swap = 0;
		tmpLayerAttr.fb.cs_mode	= DISP_YCC;
		tmpLayerAttr.fb.seq 	= DISP_SEQ_P3210;
	}
	else
	{
#ifdef OUTPUT_IN_YUVPLANNER_FORMAT
		tmpLayerAttr.fb.mode 	= DISP_MOD_NON_MB_PLANAR;
		tmpLayerAttr.fb.format 	= DISP_FORMAT_YUV420;
		tmpLayerAttr.fb.br_swap = 0;
		tmpLayerAttr.fb.seq 	= DISP_SEQ_UVUV;
#else
		tmpLayerAttr.fb.mode 	= DISP_MOD_MB_UV_COMBINED;
		tmpLayerAttr.fb.format 	= format; //DISP_FORMAT_YUV420;
		tmpLayerAttr.fb.br_swap = 0;
		tmpLayerAttr.fb.seq 	= DISP_SEQ_UVUV;
#endif
	}

	tmpLayerAttr.fb.addr[0] = 0;
	tmpLayerAttr.fb.addr[1] = 0;

	tmpLayerAttr.fb.size.width 	= image_width;
	tmpLayerAttr.fb.size.height = image_height;

	//set video layer attribute
	tmpLayerAttr.mode = DISP_LAYER_WORK_MODE_SCALER;
	//tmpLayerAttr.ck_mode = 0xff;
	//tmpLayerAttr.ck_eanble = 0;
	tmpLayerAttr.alpha_en = 1;
	tmpLayerAttr.alpha_val = 0xff;
#ifdef CONFIG_DFBCEDAR
	tmpLayerAttr.pipe = 1;
#else
	tmpLayerAttr.pipe = 0;
#endif
	//tmpLayerAttr.prio = 0xff;
	//screen window information
	tmpLayerAttr.scn_win.x = hnd_video_render->video_window.x;
	tmpLayerAttr.scn_win.y = hnd_video_render->video_window.y;
	tmpLayerAttr.scn_win.width  = hnd_video_render->video_window.width;
	tmpLayerAttr.scn_win.height = hnd_video_render->video_window.height;
	tmpLayerAttr.prio           = 0xff;
	//frame buffer pst and size information
	tmpLayerAttr.src_win.x = 0;//tmpVFrmInf->dst_rect.uStartX;
	tmpLayerAttr.src_win.y = 0;//tmpVFrmInf->dst_rect.uStartY;
	tmpLayerAttr.src_win.width = image_width;//tmpVFrmInf->dst_rect.uWidth;
	tmpLayerAttr.src_win.height = image_height;//tmpVFrmInf->dst_rect.uHeight;
	printf("width %d, height %d\n",tmpLayerAttr.src_win.width ,tmpLayerAttr.src_win.height );
	hnd_video_render->src_frm_rect.x = tmpLayerAttr.src_win.x;
	hnd_video_render->src_frm_rect.y = tmpLayerAttr.src_win.y;
	hnd_video_render->src_frm_rect.width = tmpLayerAttr.src_win.width;
	hnd_video_render->src_frm_rect.height = tmpLayerAttr.src_win.height;
	tmpLayerAttr.fb.b_trd_src		= 0;
	tmpLayerAttr.b_trd_out			= 0;
	tmpLayerAttr.fb.trd_mode 		=  (__disp_3d_src_mode_t)3;
	tmpLayerAttr.out_trd_mode		= DISP_3D_OUT_MODE_FP;
	tmpLayerAttr.b_from_screen 		= 0;
	//set channel
	//tmpLayerAttr.channel = DISP_LAYER_OUTPUT_CHN_DE_CH1;
	//FIOCTRL(hnd_video_render->de_fd, DISP_CMD_LAYER_SET_PARA, hnd_video_render->de_layer_hdl, &tmpLayerAttr);
	printf("set video layer param\n");
	args[0] = 0;
	args[1] = hnd_video_render->de_layer_hdl;
	args[2] = (unsigned long) (&tmpLayerAttr);
	args[3] = 0;
	ioctl(hnd_video_render->de_fd, DISP_CMD_LAYER_SET_PARA, args);

	args[0]							= 0;
	args[1]                 		= hnd_video_render->de_layer_hdl;
	args[2]                 		= 0;
	args[3]                 		= 0;
	ret                     		= ioctl(hnd_video_render->de_fd, DISP_CMD_LAYER_TOP,args);
	if(ret != 0)
	{		//open display layer failed, need send play end command, and exit
		printf("Open video display layer failed!\n");
		return -1;
	}
	return 0;
}

int render_init()
{

	if (hnd_video_render != (VIDEO_RENDER_CONTEXT_TYPE *) 0) {
		printf("Cedar:vply: video play back has been opended already!\n");
		return -1;
	}

	hnd_video_render = (VIDEO_RENDER_CONTEXT_TYPE *) malloc(sizeof(VIDEO_RENDER_CONTEXT_TYPE ));
	if (hnd_video_render == (VIDEO_RENDER_CONTEXT_TYPE *) 0) {
		printf("Cedar:vply: malloc hnd_video_render error!\n");
		return -1;
	}
	memset(hnd_video_render, 0, sizeof(VIDEO_RENDER_CONTEXT_TYPE ));

	hnd_video_render->first_frame_flag = 1;


#ifdef OUTPUT_IN_YUVPLANNER_FORMAT
    hnd_video_render->ybuf = av_heap_alloc(1920*1280);
    hnd_video_render->ubuf = av_heap_alloc(1920*1280);
    hnd_video_render->vbuf = av_heap_alloc(1920*1280);
    if(hnd_video_render->ybuf == NULL || hnd_video_render->ubuf == NULL || hnd_video_render->vbuf == NULL)
    {
        printf("no memory for yuv buffer.\n");
        if(hnd_video_render->ybuf)
            av_heap_free(hnd_video_render->ybuf);
        if(hnd_video_render->ubuf)
            av_heap_free(hnd_video_render->ubuf);
        if(hnd_video_render->vbuf)
            av_heap_free(hnd_video_render->vbuf);
        return -1;
    }
#endif

	hnd_video_render->de_fd = open("/dev/disp", O_RDWR);
	if (hnd_video_render->de_fd < 0) {
		printf("Open display driver failed!\n");
        if(hnd_video_render->ybuf)
            av_heap_free(hnd_video_render->ybuf);
        if(hnd_video_render->ubuf)
            av_heap_free(hnd_video_render->ubuf);
        if(hnd_video_render->vbuf)
            av_heap_free(hnd_video_render->vbuf);
		return -1;
	}

	args[0] = 0;
	args[1] = DISP_LAYER_WORK_MODE_SCALER;
	args[2] = 0;
	args[3] = 0;
	hnd_video_render->de_layer_hdl = ioctl(hnd_video_render->de_fd, DISP_CMD_LAYER_REQUEST,	args);
	if (hnd_video_render->de_layer_hdl == 0) {
		printf("Open display layer failed! de fd:%d \n", hnd_video_render->de_fd);
        if(hnd_video_render->ybuf)
            av_heap_free(hnd_video_render->ybuf);
        if(hnd_video_render->ubuf)
            av_heap_free(hnd_video_render->ubuf);
        if(hnd_video_render->vbuf)
            av_heap_free(hnd_video_render->vbuf);
		return -1;
	}


	//set video window information to default value, full screen
	hnd_video_render->video_window.x = 0;
	hnd_video_render->video_window.y = 0;
	args[0] = 0;
	args[1] = hnd_video_render->de_layer_hdl;
	args[2] = 0;
	args[3] = 0;
	hnd_video_render->video_window.width = ioctl(hnd_video_render->de_fd, DISP_CMD_SCN_GET_WIDTH, args);
	hnd_video_render->video_window.height = ioctl(hnd_video_render->de_fd,DISP_CMD_SCN_GET_HEIGHT, args);

#ifdef OUTPUT_IN_YUVPLANNER_FORMAT
#ifdef USE_MP_TO_TRANSFORM_PIXEL
    {
        unsigned long arg[4] = {0, 0, 0, 0};
        hnd_video_render->scaler_hdl = ioctl(hnd_video_render->de_fd, DISP_CMD_SCALER_REQUEST, arg);
        if(hnd_video_render->scaler_hdl == -1)
        {
            printf("can not allocate the scaler from display engine.\n");
            if(hnd_video_render->ybuf)
                av_heap_free(hnd_video_render->ybuf);
            if(hnd_video_render->ubuf)
                av_heap_free(hnd_video_render->ubuf);
            if(hnd_video_render->vbuf)
                av_heap_free(hnd_video_render->vbuf);
            return -1;
        }
    }
#endif
#endif

	printf("de---w:%d,h:%d\n", hnd_video_render->video_window.width, hnd_video_render->video_window.height);
	return 0;
}

void render_exit(void) {
	if (hnd_video_render == NULL) {
		printf("video playback has been closed already!\n");
		return;
	}
	int			ret;

	//close displayer driver context
	if (hnd_video_render->de_fd) {
		args[0] = 0;
		args[1] = hnd_video_render->de_layer_hdl;
		args[2] = 0;
		args[3] = 0;
		ioctl(hnd_video_render->de_fd, DISP_CMD_VIDEO_STOP, args);

		args[0] = 0;
		args[1] = hnd_video_render->de_layer_hdl;
		args[2] = 0;
		args[3] = 0;
		ioctl(hnd_video_render->de_fd, DISP_CMD_LAYER_RELEASE, args);

		args[0]	= 0;
		ret = ioctl(hnd_video_render->de_fd, DISP_CMD_GET_OUTPUT_TYPE, args);

		if(ret == DISP_OUTPUT_TYPE_HDMI)
		{
			args[0] 					= 0;
			args[1] 					= 0;
			args[2] 					= 0;
			args[3] 					= 0;
			ioctl(hnd_video_render->de_fd,DISP_CMD_HDMI_OFF,(unsigned long)args);

			args[0] 					= 0;
			args[1] 					= hnd_video_render->hdmi_mode;
			args[2] 					= 0;
			args[3] 					= 0;
			ioctl(hnd_video_render->de_fd, DISP_CMD_HDMI_SET_MODE, args);

			args[0] 					= 0;
			args[1] 					= 0;
			args[2] 					= 0;
			args[3] 					= 0;
			ioctl(hnd_video_render->de_fd,DISP_CMD_HDMI_ON,(unsigned long)args);

		}
		close(hnd_video_render->de_fd);
		hnd_video_render->de_fd = 0;
	}

	if (hnd_video_render) {

#ifdef OUTPUT_IN_YUVPLANNER_FORMAT

#ifdef USE_MP_TO_TRANSFORM_PIXEL
        {
            unsigned long arg[4] = {0, 0, 0, 0};

	        arg[1] = hnd_video_render->scaler_hdl;
	        ioctl(hnd_video_render->de_fd, DISP_CMD_SCALER_RELEASE, (unsigned long) arg);
        }
#endif

        if(hnd_video_render->ybuf)
            av_heap_free(hnd_video_render->ybuf);
        if(hnd_video_render->ubuf)
            av_heap_free(hnd_video_render->ubuf);
        if(hnd_video_render->vbuf)
            av_heap_free(hnd_video_render->vbuf);
#endif
		free(hnd_video_render);
		hnd_video_render = NULL;
	}
}

int render_render(void *frame_info, int frame_id)
{
	cedarv_picture_t *display_info = (cedarv_picture_t *) frame_info;
	__disp_video_fb_t tmpFrmBufAddr;
	int ret;

	memset(&tmpFrmBufAddr, 0, sizeof(__disp_video_fb_t ));
	tmpFrmBufAddr.interlace 		= display_info->is_progressive? 0: 1;
	//printf("tmpFrmBufAddr.interlace %d\n", tmpFrmBufAddr.interlace);
	tmpFrmBufAddr.top_field_first 	= display_info->top_field_first;
	tmpFrmBufAddr.frame_rate 		= display_info->frame_rate;
	//tmpFrmBufAddr.first_frame = 0;//first_frame_flg;
	//first_frame_flg = 0;

#ifdef OUTPUT_IN_YUVPLANNER_FORMAT
    TransformPicture(display_info);
	tmpFrmBufAddr.addr[0] = (unsigned int)av_heap_physic_addr(hnd_video_render->ybuf);
	tmpFrmBufAddr.addr[1] = (unsigned int)av_heap_physic_addr(hnd_video_render->ubuf);
	tmpFrmBufAddr.addr[2] = (unsigned int)av_heap_physic_addr(hnd_video_render->vbuf);
#else
	tmpFrmBufAddr.addr[0] = (unsigned int)av_heap_physic_addr(display_info->y);
	tmpFrmBufAddr.addr[1] = (unsigned int)av_heap_physic_addr(display_info->u);
#endif

	tmpFrmBufAddr.id = frame_id;


	if (hnd_video_render->first_frame_flag == 1)
	{
		__disp_layer_info_t         layer_info;
		__disp_pixel_fmt_t			pixel_format;

		pixel_format = display_info->pixel_format==CEDARV_PIXEL_FORMAT_AW_YUV422 ? DISP_FORMAT_YUV422 : DISP_FORMAT_YUV420;

		printf("config de parameter!\n");
		if(display_info->display_width && display_info->display_height)
			config_de_parameter(display_info->display_width, display_info->display_height, pixel_format);
		else
			config_de_parameter(display_info->width, display_info->height,pixel_format);

		//set_display_mode(display_info, &layer_info);

		args[0] = 0;
	    args[1] = hnd_video_render->de_layer_hdl;
	    args[2] = (unsigned long) (&layer_info);
	    args[3] = 0;
	    ioctl(hnd_video_render->de_fd, DISP_CMD_LAYER_GET_PARA, args);

	    layer_info.fb.addr[0] 	= tmpFrmBufAddr.addr[0];
	    layer_info.fb.addr[1] 	= tmpFrmBufAddr.addr[1];

	    args[0] 				= 0;
	    args[1] 				= hnd_video_render->de_layer_hdl;
	    args[2] 				= (unsigned long) (&layer_info);
	    args[3] 				= 0;
	    ret = ioctl(hnd_video_render->de_fd, DISP_CMD_LAYER_SET_PARA, args);

		args[0] = 0;
		args[1] = hnd_video_render->de_layer_hdl;
		args[2] = 0;
		args[3] = 0;
		ret = ioctl(hnd_video_render->de_fd, DISP_CMD_LAYER_OPEN, args);
		printf("layer open hdl:%d,ret:%d w:%d h:%d\n", hnd_video_render->de_layer_hdl, ret,display_info->width,display_info->height);
		if (ret != 0){
			//open display layer failed, need send play end command, and exit
			printf("Open video display layer failed!\n");
			return -1;
		}
		args[0] = 0;
		args[1] = hnd_video_render->de_layer_hdl;
		args[2] = 0;
		args[3] = 0;
		ret = ioctl(hnd_video_render->de_fd, DISP_CMD_VIDEO_START, args);
		if(ret != 0) {
			printf("Start video layer failed!\n");
			return -1;
		}

		hnd_video_render->layer_open_flag = 1;
		hnd_video_render->first_frame_flag = 0;
	}
	else
	{
		args[0] = 0;
		args[1] = hnd_video_render->de_layer_hdl;
		args[2] = (unsigned long) (&tmpFrmBufAddr);
		args[3] = 0;
		ioctl(hnd_video_render->de_fd, DISP_CMD_VIDEO_SET_FB, args);
	}

	return 0;
}

int render_get_disp_frame_id(void)
{
	return ioctl(hnd_video_render->de_fd, DISP_CMD_VIDEO_GET_FRAME_ID, args);
}


int render_set_screen_rect(__disp_rect_t rect)
{

	if(rect.width > hnd_video_render->video_window.width && hnd_video_render->video_window.width > 0)
	{
		printf("invalide width \n");
		return -1;
	}
	if(rect.height > hnd_video_render->video_window.height && hnd_video_render->video_window.height > 0)
	{
		printf("invalide height \n");
		return -1;
	}
	hnd_video_render->video_window.x = rect.x;
	hnd_video_render->video_window.y = rect.y;
	hnd_video_render->video_window.width 	= rect.width;
	hnd_video_render->video_window.height 	= rect.height;
	printf("x:%d, y:%d, width:%d, height: %d\n", rect.x, rect.y, rect.width, rect.height);
	return 0;
}


void render_get_screen_rect(__disp_rect_t *rect)
{
	rect->x = 0;
	rect->y = 0;
	rect->width 	= hnd_video_render->video_window.width;
	rect->height 	= hnd_video_render->video_window.height;
}


#ifdef OUTPUT_IN_YUVPLANNER_FORMAT

#ifdef NO_NEON_OPT //Don't HAVE_NEON
static void map32x32_to_yuv_Y(unsigned char* srcY, unsigned char* tarY, unsigned int coded_width, unsigned int coded_height)
{
	unsigned int i,j,l,m,n;
	unsigned int mb_width,mb_height,twomb_line,recon_width;
	unsigned long offset;
	unsigned char *ptr;

	ptr = srcY;
	mb_width = (coded_width+15)>>4;
	mb_height = (coded_height+15)>>4;
	twomb_line = (mb_height+1)>>1;
	recon_width = (mb_width+1)&0xfffffffe;

	for(i=0;i<twomb_line;i++)
	{
		for(j=0;j<recon_width;j+=2)
		{
			for(l=0;l<32;l++)
			{
				//first mb
				m=i*32 + l;
				n= j*16;
				if(m<coded_height && n<coded_width)
				{
					offset = m*coded_width + n;
					memcpy(tarY+offset,ptr,16);
					ptr += 16;
				}
				else
					ptr += 16;

				//second mb
				n= j*16+16;
				if(m<coded_height && n<coded_width)
				{
					offset = m*coded_width + n;
					memcpy(tarY+offset,ptr,16);
					ptr += 16;
				}
				else
					ptr += 16;
			}
		}
	}
}

static void map32x32_to_yuv_C(int mode,unsigned char* srcC,unsigned char* tarCb,unsigned char* tarCr,unsigned int coded_width,unsigned int coded_height)
{
	unsigned int i,j,l,m,n,k;
	unsigned int mb_width,mb_height,fourmb_line,recon_width;
	unsigned char line[16];
	unsigned long offset;
	unsigned char *ptr;

	ptr = srcC;
	mb_width = (coded_width+7)>>3;
	mb_height = (coded_height+7)>>3;
	fourmb_line = (mb_height+3)>>2;
	recon_width = (mb_width+1)&0xfffffffe;

	for(i=0;i<fourmb_line;i++)
	{
		for(j=0;j<recon_width;j+=2)
		{
			for(l=0;l<32;l++)
			{
				//first mb
				m=i*32 + l;
				n= j*8;
				if(m<coded_height && n<coded_width)
				{
					offset = m*coded_width + n;
					memcpy(line,ptr,16);
					for(k=0;k<8;k++)
					{
						*(tarCb + offset + k) = 0xaa;//line[2*k];
						*(tarCr + offset + k) = 0x55; //line[2*k+1];
					}
					ptr += 16;
				}
				else
					ptr += 16;

				//second mb
				n= j*8+8;
				if(m<coded_height && n<coded_width)
				{
					offset = m*coded_width + n;
					memcpy(line,ptr,16);
					for(k=0;k<8;k++)
					{
						*(tarCb + offset + k) = 0xaa;//line[2*k];
						*(tarCr + offset + k) = 0x55;//line[2*k+1];
					}
					ptr += 16;
				}
				else
					ptr += 16;
			}
		}
	}
}

static void map32x32_to_yuv_C_422(int mode,unsigned char* srcC,unsigned char* tarCb,unsigned char* tarCr,unsigned int coded_width,unsigned int coded_height) {
	;
}

#else

static void map32x32_to_yuv_Y(unsigned char* srcY,unsigned char* tarY,unsigned int coded_width,unsigned int coded_height)
{
	unsigned int i,j,l,m,n;
	unsigned int mb_width,mb_height,twomb_line,recon_width;
	unsigned long offset;
	unsigned char *ptr;
	unsigned char *dst_asm,*src_asm;

	ptr = srcY;
	mb_width = (coded_width+15)>>4;
	mb_height = (coded_height+15)>>4;
	twomb_line = (mb_height+1)>>1;
	recon_width = (mb_width+1)&0xfffffffe;

	for(i=0;i<twomb_line;i++)
	{
		for(j=0;j<mb_width/2;j++)
		{
			for(l=0;l<32;l++)
			{
				//first mb
				m=i*32 + l;
				n= j*32;
				offset = m*coded_width + n;
				//memcpy(tarY+offset,ptr,32);
				dst_asm = tarY+offset;
				src_asm = ptr;
				asm volatile (
				        "vld1.8         {d0 - d3}, [%[src_asm]]              \n\t"
				        "vst1.8         {d0 - d3}, [%[dst_asm]]              \n\t"
				        : [dst_asm] "+r" (dst_asm), [src_asm] "+r" (src_asm)
				        :  //[srcY] "r" (srcY)
				        : "cc", "memory", "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d16", "d17", "d18", "d19", "d20", "d21", "d22", "d23", "d24", "d28", "d29", "d30", "d31"
				        );

				ptr += 32;
			}
		}

		//LOGV("mb_width:%d",mb_width);
		if(mb_width & 1)
		{
			j = mb_width-1;
			for(l=0;l<32;l++)
			{
				//first mb
				m=i*32 + l;
				n= j*16;
				if(m<coded_height && n<coded_width)
				{
					offset = m*coded_width + n;
					//memcpy(tarY+offset,ptr,16);
					dst_asm = tarY+offset;
					src_asm = ptr;
					asm volatile (
					        "vld1.8         {d0 - d1}, [%[src_asm]]              \n\t"
					        "vst1.8         {d0 - d1}, [%[dst_asm]]              \n\t"
					        : [dst_asm] "+r" (dst_asm), [src_asm] "+r" (src_asm)
					        :  //[srcY] "r" (srcY)
					        : "cc", "memory", "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d16", "d17", "d18", "d19", "d20", "d21", "d22", "d23", "d24", "d28", "d29", "d30", "d31"
					        );
				}

				ptr += 16;
				ptr += 16;
			}
		}
	}
}

static void map32x32_to_yuv_C(int mode, unsigned char* srcC,unsigned char* tarCb,unsigned char* tarCr,unsigned int coded_width,unsigned int coded_height)
{
	unsigned int i,j,l,m,n,k;
	unsigned int mb_width,mb_height,fourmb_line,recon_width;
	unsigned long offset;
	unsigned char *ptr;
	unsigned char *dst0_asm,*dst1_asm,*src_asm;
	unsigned char line[16];
	int dst_stride = mode==0 ? (coded_width + 15) & (~15) : coded_width;

	ptr = srcC;
	mb_width = (coded_width+7)>>3;
	mb_height = (coded_height+7)>>3;
	fourmb_line = (mb_height+3)>>2;
	recon_width = (mb_width+1)&0xfffffffe;

	for(i=0;i<fourmb_line;i++)
	{
		for(j=0;j<mb_width/2;j++)
		{
			for(l=0;l<32;l++)
			{
				//first mb
				m=i*32 + l;
				n= j*16;
				if(m<coded_height && n<coded_width)
				{
					offset = m*dst_stride + n;

					dst0_asm = tarCb + offset;
					dst1_asm = tarCr+offset;
					src_asm = ptr;
//					for(k=0;k<16;k++)
//					{
//						dst0_asm[k] = src_asm[2*k];
//						dst1_asm[k] = src_asm[2*k+1];
//					}
					asm volatile (
					        "vld1.8         {d0 - d3}, [%[src_asm]]              \n\t"
							"vuzp.8         d0, d1              \n\t"
							"vuzp.8         d2, d3              \n\t"
							"vst1.8         {d0}, [%[dst0_asm]]!              \n\t"
							"vst1.8         {d2}, [%[dst0_asm]]!              \n\t"
							"vst1.8         {d1}, [%[dst1_asm]]!              \n\t"
							"vst1.8         {d3}, [%[dst1_asm]]!              \n\t"
					         : [dst0_asm] "+r" (dst0_asm), [dst1_asm] "+r" (dst1_asm), [src_asm] "+r" (src_asm)
					         :  //[srcY] "r" (srcY)
					         : "cc", "memory", "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d16", "d17", "d18", "d19", "d20", "d21", "d22", "d23", "d24", "d28", "d29", "d30", "d31"
					         );
				}

				ptr += 32;
			}
		}

		if(mb_width & 1)
		{
			j= mb_width-1;
			for(l=0;l<32;l++)
			{
				m=i*32 + l;
				n= j*8;

				if(m<coded_height && n<coded_width)
				{
					offset = m*dst_stride + n;
					memcpy(line,ptr,16);
					for(k=0;k<8;k++)
					{
						*(tarCb + offset + k) = line[2*k];
						*(tarCr + offset + k) = line[2*k+1];
					}
				}

				ptr += 16;
				ptr += 16;
			}
		}
	}
}

static void map32x32_to_yuv_C_422(int mode, unsigned char* srcC,unsigned char* tarCb,unsigned char* tarCr,unsigned int coded_width,unsigned int coded_height)
{
	unsigned int i,j,l,m,n,k;
	unsigned int mb_width,mb_height,twomb_line,recon_width;
	unsigned long offset;
	unsigned char *ptr;
	unsigned char *dst0_asm,*dst1_asm,*src_asm;
	unsigned char line[16];

	ptr = srcC;
	mb_width = (coded_width+7)>>3;
	mb_height = (coded_height+7)>>3;
	twomb_line = (mb_height+1)>>1;
	recon_width = (mb_width+1)&0xfffffffe;

	for(i=0;i<twomb_line;i++)
	{
		for(j=0;j<mb_width/2;j++)
		{
			for(l=0;l<16;l++)
			{
				//first mb
				m=i*16 + l;
				n= j*16;
				if(m<coded_height && n<coded_width)
				{
					offset = m*coded_width + n;

					dst0_asm = tarCb + offset;
					dst1_asm = tarCr+offset;
					src_asm = ptr;
//					for(k=0;k<16;k++)
//					{
//						dst0_asm[k] = src_asm[2*k];
//						dst1_asm[k] = src_asm[2*k+1];
//					}
					asm volatile (
					        "vld1.8         {d0 - d3}, [%[src_asm]]              \n\t"
							"vuzp.8         d0, d1              \n\t"
							"vuzp.8         d2, d3              \n\t"
							"vst1.8         {d0}, [%[dst0_asm]]!              \n\t"
							"vst1.8         {d2}, [%[dst0_asm]]!              \n\t"
							"vst1.8         {d1}, [%[dst1_asm]]!              \n\t"
							"vst1.8         {d3}, [%[dst1_asm]]!              \n\t"
					         : [dst0_asm] "+r" (dst0_asm), [dst1_asm] "+r" (dst1_asm), [src_asm] "+r" (src_asm)
					         :  //[srcY] "r" (srcY)
					         : "cc", "memory", "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d16", "d17", "d18", "d19", "d20", "d21", "d22", "d23", "d24", "d28", "d29", "d30", "d31"
					         );
				}

				ptr += 32;
				ptr += 32;
			}
		}

		if(mb_width & 1)
		{
			j= mb_width-1;
			for(l=0;l<16;l++)
			{
				m=i*32 + l;
				n= j*8;

				if(m<coded_height && n<coded_width)
				{
					offset = m*coded_width + n;
					memcpy(line,ptr,16);
					for(k=0;k<8;k++)
					{
						*(tarCb + offset + k) = line[2*k];
						*(tarCr + offset + k) = line[2*k+1];
					}
				}

				ptr += 32;
				ptr += 32;
			}
		}
	}
}

#endif

/* reference code
static void S32_D565_Blend_Dither_neon(uint16_t *dst, const SkPMColor *src,
                                       int count, U8CPU alpha, int x, int y)
{
    const uint8_t *dstart = &gDitherMatrix_Neon[(y&3)*12 + (x&3)];

    int scale = SkAlpha255To256(alpha);

    asm volatile (
                  "vld1.8         {d31}, [%[dstart]]              \n\t"   // load dither values
                  "vshr.u8        d30, d31, #1                    \n\t"   // calc. green dither values
                  "vdup.16        d6, %[scale]                    \n\t"   // duplicate scale into neon reg
                  "vmov.i8        d29, #0x3f                      \n\t"   // set up green mask
                  "vmov.i8        d28, #0x1f                      \n\t"   // set up blue mask
                  "1:                                                 \n\t"
                  "vld4.8         {d0, d1, d2, d3}, [%[src]]!     \n\t"   // load 8 pixels and split into argb
                  "vshr.u8        d22, d0, #5                     \n\t"   // calc. red >> 5
                  "vshr.u8        d23, d1, #6                     \n\t"   // calc. green >> 6
                  "vshr.u8        d24, d2, #5                     \n\t"   // calc. blue >> 5
                  "vaddl.u8       q8, d0, d31                     \n\t"   // add in dither to red and widen
                  "vaddl.u8       q9, d1, d30                     \n\t"   // add in dither to green and widen
                  "vaddl.u8       q10, d2, d31                    \n\t"   // add in dither to blue and widen
                  "vsubw.u8       q8, q8, d22                     \n\t"   // sub shifted red from result
                  "vsubw.u8       q9, q9, d23                     \n\t"   // sub shifted green from result
                  "vsubw.u8       q10, q10, d24                   \n\t"   // sub shifted blue from result
                  "vshrn.i16      d22, q8, #3                     \n\t"   // shift right and narrow to 5 bits
                  "vshrn.i16      d23, q9, #2                     \n\t"   // shift right and narrow to 6 bits
                  "vshrn.i16      d24, q10, #3                    \n\t"   // shift right and narrow to 5 bits
                  // load 8 pixels from dst, extract rgb
                  "vld1.16        {d0, d1}, [%[dst]]              \n\t"   // load 8 pixels
                  "vshrn.i16      d17, q0, #5                     \n\t"   // shift green down to bottom 6 bits
                  "vmovn.i16      d18, q0                         \n\t"   // narrow to get blue as bytes
                  "vshr.u16       q0, q0, #11                     \n\t"   // shift down to extract red
                  "vand           d17, d17, d29                   \n\t"   // and green with green mask
                  "vand           d18, d18, d28                   \n\t"   // and blue with blue mask
                  "vmovn.i16      d16, q0                         \n\t"   // narrow to get red as bytes
                  // src = {d22 (r), d23 (g), d24 (b)}
                  // dst = {d16 (r), d17 (g), d18 (b)}
                  // subtract dst from src and widen
                  "vsubl.s8       q0, d22, d16                    \n\t"   // subtract red src from dst
                  "vsubl.s8       q1, d23, d17                    \n\t"   // subtract green src from dst
                  "vsubl.s8       q2, d24, d18                    \n\t"   // subtract blue src from dst
                  // multiply diffs by scale and shift
                  "vmul.i16       q0, q0, d6[0]                   \n\t"   // multiply red by scale
                  "vmul.i16       q1, q1, d6[0]                   \n\t"   // multiply blue by scale
                  "vmul.i16       q2, q2, d6[0]                   \n\t"   // multiply green by scale
                  "subs           %[count], %[count], #8          \n\t"   // decrement loop counter
                  "vshrn.i16      d0, q0, #8                      \n\t"   // shift down red by 8 and narrow
                  "vshrn.i16      d2, q1, #8                      \n\t"   // shift down green by 8 and narrow
                  "vshrn.i16      d4, q2, #8                      \n\t"   // shift down blue by 8 and narrow
                  // add dst to result
                  "vaddl.s8       q0, d0, d16                     \n\t"   // add dst to red
                  "vaddl.s8       q1, d2, d17                     \n\t"   // add dst to green
                  "vaddl.s8       q2, d4, d18                     \n\t"   // add dst to blue
                  // put result into 565 format
                  "vsli.i16       q2, q1, #5                      \n\t"   // shift up green and insert into blue
                  "vsli.i16       q2, q0, #11                     \n\t"   // shift up red and insert into blue
                  "vst1.16        {d4, d5}, [%[dst]]!             \n\t"   // store result
                  "bgt            1b                              \n\t"   // loop if count > 0
                  : [src] "+r" (src), [dst] "+r" (dst), [count] "+r" (count)
                  : [dstart] "r" (dstart), [scale] "r" (scale)
                  : "cc", "memory", "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d16", "d17", "d18", "d19", "d20", "d21", "d22", "d23", "d24", "d28", "d29", "d30", "d31"
                  );

    DITHER_565_SCAN(y);

    while((count & 7) > 0)
    {
        SkPMColor c = *src++;

        int dither = DITHER_VALUE(x);
        int sr = SkGetPackedR32(c);
        int sg = SkGetPackedG32(c);
        int sb = SkGetPackedB32(c);
        sr = SkDITHER_R32To565(sr, dither);
        sg = SkDITHER_G32To565(sg, dither);
        sb = SkDITHER_B32To565(sb, dither);

        uint16_t d = *dst;
        *dst++ = SkPackRGB16(SkAlphaBlend(sr, SkGetPackedR16(d), scale),
                             SkAlphaBlend(sg, SkGetPackedG16(d), scale),
                             SkAlphaBlend(sb, SkGetPackedB16(d), scale));
        DITHER_INC_X(x);
        count--;
    }
}


//below code copy from memcpy.S
1:      // The main loop copies 64 bytes at a time
        vld1.8      {d0  - d3},   [r1]!
        vld1.8      {d4  - d7},   [r1]!
        pld         [r1, #(PREFETCH_DISTANCE)]
        subs        r2, r2, #64
        vst1.8      {d0  - d3},   [r0, :128]!
        vst1.8      {d4  - d7},   [r0, :128]!
        bhs         1b

*/


int SoftwarePictureScaler(ScalerParameter *cdx_scaler_para)
{
//	LOGV( "scaler parameter check: w:%d h:%d y_in:0x%x c_in:0x%x out_0:0x%x out_1:0x%x out_2:0x%x ",
//			cdx_scaler_para->width_in,cdx_scaler_para->height_in,cdx_scaler_para->addr_y_in,cdx_scaler_para->addr_c_in,
//			cdx_scaler_para->addr_y_out,cdx_scaler_para->addr_u_out,cdx_scaler_para->addr_v_out);

	map32x32_to_yuv_Y(cdx_scaler_para->addr_y_in, (unsigned char*)cdx_scaler_para->addr_y_out, cdx_scaler_para->width_out, cdx_scaler_para->height_out);
	if (cdx_scaler_para->format_in == CONVERT_COLOR_FORMAT_YUV422MB) {
		map32x32_to_yuv_C_422(cdx_scaler_para->mode,cdx_scaler_para->addr_c_in, (unsigned char*)cdx_scaler_para->addr_u_out, (unsigned char*)cdx_scaler_para->addr_v_out,cdx_scaler_para->width_out / 2, cdx_scaler_para->height_out / 2);
	}
	else {
		map32x32_to_yuv_C(cdx_scaler_para->mode,cdx_scaler_para->addr_c_in, (unsigned char*)cdx_scaler_para->addr_u_out, (unsigned char*)cdx_scaler_para->addr_v_out, cdx_scaler_para->width_out / 2, cdx_scaler_para->height_out / 2);
	}

	return 0;
}


int HardwarePictureScaler(ScalerParameter *cdx_scaler_para)
{
	unsigned long arg[4] = {0,0,0,0};
//	int scaler_hdl;
	__disp_scaler_para_t scaler_para;


//	scaler_hdl = ioctl(hnd_video_render->de_fd, DISP_CMD_SCALER_REQUEST, (unsigned long) arg);
//	if(scaler_hdl == -1){
//		printf("request scaler fail");
//		return -1;
//	}

	memset(&scaler_para, 0, sizeof(__disp_scaler_para_t));
	scaler_para.input_fb.addr[0]      = (unsigned int)av_heap_physic_addr(cdx_scaler_para->addr_y_in);//
	scaler_para.input_fb.addr[1]      = (unsigned int)av_heap_physic_addr(cdx_scaler_para->addr_c_in);//
	scaler_para.input_fb.size.width   = cdx_scaler_para->width_in;//
	scaler_para.input_fb.size.height  = cdx_scaler_para->height_in;//
	scaler_para.input_fb.format       =  DISP_FORMAT_YUV420;
	scaler_para.input_fb.seq          = DISP_SEQ_UVUV;
	scaler_para.input_fb.mode         = DISP_MOD_MB_UV_COMBINED;
	scaler_para.input_fb.br_swap      = 0;
	scaler_para.input_fb.cs_mode      = DISP_BT601;
	scaler_para.source_regn.x         = 0;
	scaler_para.source_regn.y         = 0;
	scaler_para.source_regn.width     = cdx_scaler_para->width_in;//
	scaler_para.source_regn.height    = cdx_scaler_para->height_in;//
	scaler_para.output_fb.addr[0]     = av_heap_physic_addr(cdx_scaler_para->addr_y_out);//
	scaler_para.output_fb.addr[1]     = av_heap_physic_addr(cdx_scaler_para->addr_u_out);//
	scaler_para.output_fb.addr[2]     = av_heap_physic_addr(cdx_scaler_para->addr_v_out);//
	scaler_para.output_fb.size.width  = cdx_scaler_para->width_out;//
	scaler_para.output_fb.size.height = cdx_scaler_para->height_out;//
	scaler_para.output_fb.format      = DISP_FORMAT_YUV420;
	scaler_para.output_fb.seq         = DISP_SEQ_P3210;
	scaler_para.output_fb.mode        = DISP_MOD_NON_MB_PLANAR;
	scaler_para.output_fb.br_swap     = 0;

    if(cdx_scaler_para->height_in >= 720)
	    scaler_para.output_fb.cs_mode = DISP_BT709;
	else
	    scaler_para.output_fb.cs_mode = DISP_BT601;

	arg[1] = hnd_video_render->scaler_hdl;
	arg[2] = (unsigned long) &scaler_para;
	ioctl(hnd_video_render->de_fd, DISP_CMD_SCALER_EXECUTE, (unsigned long) arg);

//	arg[1] = scaler_hdl;
//	ioctl(hnd_video_render->de_fd, DISP_CMD_SCALER_RELEASE, (unsigned long) arg);

	return 0;
}


int TransformPicture(cedarv_picture_t* pict)
{
	ScalerParameter cdx_scaler_para;
	int display_height_align;
	int display_width_align;
	int dst_c_stride;
	int dst_y_size,dst_c_size;
	int alloc_size;

	if(pict == NULL)
		return -1;

	pict->display_height = (pict->display_height + 7) & (~7);
	display_height_align = (pict->display_height + 1) & (~1);
	display_width_align  = (pict->display_width + 15) & (~15);
	dst_y_size           = display_width_align * display_height_align;
	dst_c_stride         = (pict->display_width/2 + 15) & (~15);
	dst_c_size           = dst_c_stride * (display_height_align/2);
	alloc_size           = dst_y_size + dst_c_size * 2;

	cdx_scaler_para.mode       = 0;
	cdx_scaler_para.format_in  = (pict->pixel_format == 0x11) ? CONVERT_COLOR_FORMAT_YUV422MB : CONVERT_COLOR_FORMAT_YUV420MB;
	cdx_scaler_para.format_out = CONVERT_COLOR_FORMAT_YUV420PLANNER;
	cdx_scaler_para.width_in   = pict->width;
	cdx_scaler_para.height_in  = pict->height;
	cdx_scaler_para.addr_y_in  = (void*)pict->y;
	cdx_scaler_para.addr_c_in  = (void*)pict->u;
#if 0
	cedarx_cache_op(cdx_scaler_para.addr_y_in, cdx_scaler_para.addr_y_in+pict->size_y, CEDARX_DCACHE_FLUSH);
	cedarx_cache_op(cdx_scaler_para.addr_c_in, cdx_scaler_para.addr_c_in+pict->size_u, CEDARX_DCACHE_FLUSH);
#endif
	cdx_scaler_para.width_out  = display_width_align;
	cdx_scaler_para.height_out = display_height_align;

	cdx_scaler_para.addr_y_out = (unsigned int)hnd_video_render->ybuf;
	cdx_scaler_para.addr_u_out = (unsigned int)hnd_video_render->ubuf;
	cdx_scaler_para.addr_v_out = (unsigned int)hnd_video_render->vbuf;

#ifndef USE_MP_TO_TRANSFORM_PIXEL
    //* use neon accelarator instruction to transform the pixel format, slow if buffer is not cached(DMA mode).
	if(SoftwarePictureScaler(&cdx_scaler_para) != 0)
		return -1;
#else
	if(HardwarePictureScaler(&cdx_scaler_para) != 0)
		return -1;
#endif

	return 0;
}

#endif
