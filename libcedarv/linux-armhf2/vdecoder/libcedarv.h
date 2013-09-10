
#ifndef LIBCEDARV_H
#define LIBCEDARV_H

#ifdef __cplusplus
extern "C" {
#endif
    
    #include "libve_typedef.h"
    
    //*         Display Time Window
    //* |<---window_width--->|<---window_width--->|
    //* |<--------------------------------------->|
    //*                      ^
    //*                  Frame PTS
    #define DISPLAY_TIME_WINDOW_WIDTH   15      //* when current time fall in one picture's display time window,  this picture 
                                                //* can be show, otherwise it maybe too early or too late to show this picture.
                                   
    #define CEDARV_BVOP_DEC_LATE_THRESHOLD     60      	//* decode B-VOP time late threshold, if the later than this value, the B-VOP
                                                		//* will be dropped without decoded.
    #define CEDARV_VOP_DEC_LATE_THRESHOLD      1000    	//* decode vop too late threshold, if the time of vop decode is later than this
                                                		//* value, the no-key frames before next key frame will be dropped all.
    
    typedef enum CEDARV_STREAM_FORMAT
    {
        CEDARV_STREAM_FORMAT_UNKNOW,
        CEDARV_STREAM_FORMAT_MPEG2,
        CEDARV_STREAM_FORMAT_MPEG4,
        CEDARV_STREAM_FORMAT_REALVIDEO,
        CEDARV_STREAM_FORMAT_H264,
        CEDARV_STREAM_FORMAT_VC1,
        CEDARV_STREAM_FORMAT_AVS,
        CEDARV_STREAM_FORMAT_MJPEG,  
        CEDARV_STREAM_FORMAT_VP8,
        CEDARV_STREAM_FORMAT_NETWORK
    }cedarv_stream_format_e;
    
    typedef enum CEDARV_SUB_FORMAT
    {
        CEDARV_SUB_FORMAT_UNKNOW = 0,
        CEDARV_MPEG2_SUB_FORMAT_MPEG1,
        CEDARV_MPEG2_SUB_FORMAT_MPEG2,
        CEDARV_MPEG4_SUB_FORMAT_XVID,
        CEDARV_MPEG4_SUB_FORMAT_DIVX3,
        CEDARV_MPEG4_SUB_FORMAT_DIVX4,
        CEDARV_MPEG4_SUB_FORMAT_DIVX5,
        CEDARV_MPEG4_SUB_FORMAT_SORENSSON_H263,
        CEDARV_MPEG4_SUB_FORMAT_H263,
        CEDARV_MPEG4_SUB_FORMAT_RMG2,		//* H263 coded video stream muxed in '.rm' file.
        CEDARV_MPEG4_SUB_FORMAT_VP6,
        CEDARV_MPEG4_SUB_FORMAT_WMV1,
        CEDARV_MPEG4_SUB_FORMAT_WMV2,
        CEDARV_MPEG4_SUB_FORMAT_DIVX2,		//MSMPEG4V2
        CEDARV_MPEG4_SUB_FORMAT_DIVX1,		//MSMPEG4V1
    }cedarv_sub_format_e;
    
    typedef enum CEDARV_CONTAINER_FORMAT
    {
    	CEDARV_CONTAINER_FORMAT_UNKNOW,
    	CEDARV_CONTAINER_FORMAT_AVI,
    	CEDARV_CONTAINER_FORMAT_ASF,
    	CEDARV_CONTAINER_FORMAT_DAT,
    	CEDARV_CONTAINER_FORMAT_FLV,
    	CEDARV_CONTAINER_FORMAT_MKV,
    	CEDARV_CONTAINER_FORMAT_MOV,
    	CEDARV_CONTAINER_FORMAT_MPG,
    	CEDARV_CONTAINER_FORMAT_PMP,
    	CEDARV_CONTAINER_FORMAT_RM,
    	CEDARV_CONTAINER_FORMAT_TS,
    	CEDARV_CONTAINER_FORMAT_VOB,
    	CEDARV_CONTAINER_FORMAT_WEBM,
    	CEDARV_CONTAINER_FORMAT_OGM,
    	CEDARV_CONTAINER_FORMAT_RAW,
    }cedarv_container_format_e;

	typedef enum CEDARV_3D_MODE
	{
		//* for 2D pictures.
		CEDARV_3D_MODE_NONE 				= 0,

		//* for double stream video like MVC and MJPEG.
		CEDARV_3D_MODE_DOUBLE_STREAM,

		//* for single stream video.
		CEDARV_3D_MODE_SIDE_BY_SIDE,
		CEDARV_3D_MODE_TOP_TO_BOTTOM,
		CEDARV_3D_MODE_LINE_INTERLEAVE,
		CEDARV_3D_MODE_COLUME_INTERLEAVE

	}cedarv_3d_mode_e;

	typedef enum CEDARV_ANAGLAGH_TRANSFORM_MODE
	{
		//* for transmission from 'size by size' or 'top to bottom' mode to 'anaglagh' modes.
		//* these values are just for output, 3d mode in stream information structure should
		//* not set to these values.
		CEDARV_ANAGLAGH_RED_BLUE,
		CEDARV_ANAGLAGH_RED_GREEN,
		CEDARV_ANAGLAGH_RED_CYAN,
		CEDARV_ANAGLAGH_COLOR,
		CEDARV_ANAGLAGH_HALF_COLOR,
		CEDARV_ANAGLAGH_OPTIMIZED,
		CEDARV_ANAGLAGH_YELLOW_BLUE,
		CEDARV_ANAGLAGH_NONE,
	}cedarv_anaglath_trans_mode_e;

    typedef struct CEDARV_STREAM_INFORMATION
    {
        cedarv_stream_format_e  	format;
        cedarv_sub_format_e	    	sub_format;
        cedarv_container_format_e 	container_format;
        u32                     	video_width;     //* video picture width, if unknown please set it to 0;
        u32                     	video_height;    //* video picture height, if unknown please set it to 0;
        u32                     	frame_rate;      //* frame rate, multiplied by 1000, ex. 29.970 frames per second then frame_rate = 29970;
        u32                     	frame_duration;  //* how many us one frame should be display;
        u32                     	aspect_ratio;    //* pixel width to pixel height ratio, multiplied by 1000;
        u32                     	init_data_len;   //* data length of the initial data for decoder;
        u8*                     	init_data;       //* some decoders may need initial data to start up;
        u32                     	is_pts_correct;  //* used for h.264 decoder current;

        cedarv_3d_mode_e			_3d_mode;
    }cedarv_stream_info_t;
    
    
    #define CEDARV_FLAG_PTS_VALID   		0x2
    #define CEDARV_FLAG_FIRST_PART  		0x8
    #define CEDARV_FLAG_LAST_PART   		0x10
	#define CEDARV_FLAG_MPEG4_EMPTY_FRAME	0x20
	#define CEDARV_FLAG_DECODE_NO_DELAY     0x40000000
    #define CEDARV_FLAG_DATA_INVALID 		0x80000000
    
    typedef struct CEDARV_STREAM_DATA_INFORMATION
    {
        u32 flags;
        u32 lengh;
        u64 pts;
        u32	type;
    }cedarv_stream_data_info_t;
    
    
    typedef enum CEDARV_PIXEL_FORMAT
    {
        CEDARV_PIXEL_FORMAT_1BPP       = 0x0,
        CEDARV_PIXEL_FORMAT_2BPP       = 0x1,
        CEDARV_PIXEL_FORMAT_4BPP       = 0x2,
        CEDARV_PIXEL_FORMAT_8BPP       = 0x3,
        CEDARV_PIXEL_FORMAT_RGB655     = 0x4,
        CEDARV_PIXEL_FORMAT_RGB565     = 0x5,
        CEDARV_PIXEL_FORMAT_RGB556     = 0x6,
        CEDARV_PIXEL_FORMAT_ARGB1555   = 0x7,
        CEDARV_PIXEL_FORMAT_RGBA5551   = 0x8,
        CEDARV_PIXEL_FORMAT_RGB888     = 0x9,
        CEDARV_PIXEL_FORMAT_ARGB8888   = 0xa,
        CEDARV_PIXEL_FORMAT_YUV444     = 0xb,
        CEDARV_PIXEL_FORMAT_YUV422     = 0xc,
        CEDARV_PIXEL_FORMAT_YUV420     = 0xd,
        CEDARV_PIXEL_FORMAT_YUV411     = 0xe,
        CEDARV_PIXEL_FORMAT_CSIRGB     = 0xf,
        CEDARV_PIXEL_FORMAT_MB_UV_COMBINE_YUV420  = 0x10,
        CEDARV_PIXEL_FORMAT_MB_UV_COMBINE_YUV422  = 0x11,
        CEDARV_PIXEL_FORMAT_MB_UV_COMBINE_YUV411  = 0x12,
        CEDARV_PIXEL_FORMAT_PLANNER_YUV420        = 0x13,
        CEDARV_PIXEL_FORMAT_PLANNER_YVU420        = 0x14
    }cedarv_pixel_format_e;
	
	#define CEDARV_PIXEL_FORMAT_AW_YUV420 CEDARV_PIXEL_FORMAT_MB_UV_COMBINE_YUV420
	#define CEDARV_PIXEL_FORMAT_AW_YUV422 CEDARV_PIXEL_FORMAT_MB_UV_COMBINE_YUV422
	#define CEDARV_PIXEL_FORMAT_AW_YUV411 CEDARV_PIXEL_FORMAT_MB_UV_COMBINE_YUV411
    
	#define CEDARV_PICT_PROP_NO_SYNC   0x1
        
    typedef struct CEDARV_PICTURE_INFORMATION
    {
        u32                     id;                     //* picture id assigned by outside, decoder do not use this field;
                                
		u32						width;					//* width of picture content;
		u32						height;					//* height of picture content;
        u32                     top_offset;				//* display region top offset;
        u32                     left_offset;			//* display region left offset;
        u32                     display_width;			//* display region width;
        u32                     display_height;			//* display region height;
        u32                     store_width;            //* stored picture width;
        u32                     store_height;           //* stored picture height;

        u8                      rotate_angle;           //* how this picture has been rotated, 0: no rotate, 1: 90 degree (clock wise), 2: 180, 3: 270, 4: horizon flip, 5: vertical flig;
        u8                      horizontal_scale_ratio; //* what ratio this picture has been scaled down at horizon size, 0: 1/1, 1: 1/2, 2: 1/4, 3: 1/8;
        u8                      vertical_scale_ratio;   //* what ratio this picture has been scaled down at vetical size, 0: 1/1, 1: 1/2, 2: 1/4, 3: 1/8;
        u32                     frame_rate;             //* frame_rate, multiplied by 1000;
        u32                     aspect_ratio;           //* pixel width to pixel height ratio, multiplied by 1000;
        u32                     pict_prop;              //* picture property flags
        u8                      is_progressive;         //* progressive or interlace picture;
        u8                      top_field_first;        //* display top field first;
        u8                      repeat_top_field;       //* if interlace picture, whether repeat the top field when display;
        u8                      repeat_bottom_field;    //* if interlace picture, whether repeat the bottom field when display;
        cedarv_pixel_format_e   pixel_format;
        u64                     pts;                    //* presentation time stamp, in unit of milli-second;
        u64                     pcr;                    //* program clock reference;

        cedarv_3d_mode_e		_3d_mode;
        cedarv_anaglath_trans_mode_e anaglath_transform_mode;

        u32             		size_y;
        u32             		size_u;
        u32             		size_v;
        u32             		size_alpha;
        
        u8*                     y;                      //* pixel data, it is interpreted based on pixel_format;
        u8*                     u;                      //* pixel data, it is interpreted based on pixel_format;
        u8*                     v;                      //* pixel data, it is interpreted based on pixel_format;
        u8*                     alpha;                  //* pixel data, it is interpreted based on pixel_format;

        u32						size_y2;
        u32						size_u2;
        u32						size_v2;
        u32						size_alpha2;

        u8 *					y2;
        u8 *					u2;
        u8 *					v2;
        u8 *					alpha2;

        u32						display_3d_mode;		//* this value has nothing to do with decoder, it is used for video render to
        												//* pass display mode to overlay module. cedarx_display_3d_mode_e
        u32                     flag_addr;//dit maf flag address
        u32                     flag_stride;//dit maf flag line stride
        u8                      maf_valid;
        u8                      pre_frame_valid;
    }cedarv_picture_t;
    
    typedef enum CEDARV_RESULT
    {
        CEDARV_RESULT_OK                      = 0x0,      //* operation success;
        CEDARV_RESULT_FRAME_DECODED           = 0x1,      //* decode operation decodes one frame;
        CEDARV_RESULT_KEYFRAME_DECODED        = 0x3,      //* decode operation decodes one key frame;
        CEDARV_RESULT_NO_FRAME_BUFFER         = 0x4,      //* fail when try to get an empty frame buffer;
        CEDARV_RESULT_NO_BITSTREAM            = 0x5,      //* fail when try to get bitstream frame;
        CEDARV_RESULT_MULTI_PIXEL			  = 0x6,      //* support multi_pixel;
        
        CEDARV_RESULT_ERR_FAIL                = -1,       //* operation fail;
        CEDARV_RESULT_ERR_INVALID_PARAM       = -2,       //* failure caused by invalid function parameter;
        CEDARV_RESULT_ERR_INVALID_STREAM      = -3,       //* failure caused by invalid video stream data;
        CEDARV_RESULT_ERR_NO_MEMORY           = -4,       //* failure caused by memory allocation fail;
        CEDARV_RESULT_ERR_UNSUPPORTED         = -5,       //* failure caused by not supported stream content;
    }cedarv_result_e;
    
    
    typedef enum CEDARV_STATUS
    {
        CEDARV_STATUS_STOP,
        CEDARV_STATUS_PAUSE,
        CEDARV_STATUS_PLAY,
        CEDARV_STATUS_FORWARD,
        CEDARV_STATUS_BACKWARD,
        CEDARV_STATUS_PREVIEW
    }cedarv_status_e;
    
    typedef enum CEDARV_IO_COMMAND
    {
        CEDARV_COMMAND_PLAY,
        CEDARV_COMMAND_PAUSE,
        CEDARV_COMMAND_FORWARD,
        CEDARV_COMMAND_BACKWARD,
        CEDARV_COMMAND_STOP,
        CEDARV_COMMAND_JUMP,
        CEDARV_COMMAND_ROTATE,  //static rotate?
        CEDARV_COMMAND_SET_TOTALMEMSIZE,

        CEDARV_COMMAND_DROP_B_FRAME,
        CEDARV_COMMAND_DISABLE_3D,
        CEDARV_COMMAN_SET_SYS_TIME,
        //* for preview application.
        CEDARV_COMMAND_PREVIEW_MODE,		//* aux = 0, pbuffer = NULL,
        									//* return value always CEDARV_RESULT_OK, should be called before the 'open' fuction.
        //reset vdeclib
        CEDARV_COMMAND_RESET,

        //* set max output size for scale mode.
        CEDARV_COMMAND_SET_MAX_OUTPUT_HEIGHT,
        CEDARV_COMMAND_SET_MAX_OUTPUT_WIDTH,

        CEDARV_COMMAND_GET_STREAM_INFO, 	//* param = cedarv_stream_info_t*

        //* for mpeg2 tag play.
        CEDARV_COMMAND_GET_SEQUENCE_INFO,
        CEDARV_COMMAND_SET_SEQUENCE_INFO,

        //* for transforming 3d 'size by size' or 'top to bottom' pictures to 'anaglagh' pictures.
        CEDARV_COMMAND_SET_STREAM_3D_MODE,			//* reset the 3d mode of input stream.
        CEDARV_COMMAND_SET_ANAGLATH_TRANS_MODE,		//* select an output 3d mode, this will be used only to decide which analagh transform mode is used.
        CEDARV_COMMAND_OPEN_ANAGLATH_TRANSFROM, 	//* open ve anaglath transformation
        CEDARV_COMMAND_CLOSE_ANAGLATH_TRANSFROM,	//* close ve anaglath transformation
        CEDARV_COMMAND_GET_CHIP_VERSION,

        CEDARV_COMMAND_FLUSH,
        CEDARV_COMMAND_CLOSE_MAF,
        CEDARV_COMMAND_SET_DEMUX_TYPE,
        CEDARV_COMMAND_DECODE_NO_DELAY,
		CEDARV_COMMAND_SET_DYNAMIC_ROTATE_ANGLE,
        CEDARV_COMMAND_DYNAMIC_ROTATE,      //dynamic rotate
        CEDARV_COMMAND_SET_VBV_SIZE,
        CEDARV_COMMAND_SET_PIXEL_FORMAT,			//1633
        CEDARV_COMMAND_MULTI_PIXEL,
        CEDARV_COMMAND_OPEN_YV32_TRANSFROM,
        CEDARV_COMMAND_CLOSE_YV32_TRANSFROM,
        CEDARV_COMMAND_SET_DISPLAYFRAME_REQUESTMODE,

		//* for scale
		CEDARV_COMMAND_ENABLE_SCALE_DOWN,
		CEDARV_COMMAND_SET_HORIZON_SCALE_RATIO,
		CEDARV_COMMAND_SET_VERTICAL_SCALE_RATIO,
    }cedarv_io_cmd_e;
    
    
    typedef struct CEDARV_QUALITY
    {
        u32 vbv_buffer_usage;       //* persentage of the VBV buffer;
        u32 frame_num_in_vbv;       //* bitstream frame number in vbv;
    }cedarv_quality_t;
    
    typedef enum CDX_DECODE_VIDEO_STREAM_TYPE
	{
    	CDX_VIDEO_STREAM_MAJOR = 0,
    	CDX_VIDEO_STREAM_MINOR,
    	CDX_VIDEO_STREAM_NONE,
    }CDX_VIDEO_STREAM_TYPE;
	
    typedef struct CEDARV_DECODER   cedarv_decoder_t;
    struct CEDARV_DECODER
    {
        CDX_VIDEO_STREAM_TYPE video_stream_type;

        s32 (*open)(cedarv_decoder_t* p);
        s32 (*close)(cedarv_decoder_t* p);
        s32 (*decode)(cedarv_decoder_t* p);
        s32 (*ioctrl)(cedarv_decoder_t* p, u32 cmd, u32 param);
        s32 (*request_write)(cedarv_decoder_t*p, u32 require_size, u8** buf0, u32* size0, u8** buf1, u32* size1);
        s32 (*update_data)(cedarv_decoder_t* p, cedarv_stream_data_info_t* data_info);
        
        s32 (*display_request)(cedarv_decoder_t* p, cedarv_picture_t* picture);
        s32 (*display_release)(cedarv_decoder_t* p, u32 frame_index);
        s32 (*picture_ready)(cedarv_decoder_t* p);
        s32 (*display_dump_picture)(cedarv_decoder_t* p, cedarv_picture_t* picture);
        s32 (*set_vstream_info)(cedarv_decoder_t* p, cedarv_stream_info_t* info);
        
        s32 (*query_quality)(cedarv_decoder_t* p, cedarv_quality_t* vq);
        
        void (*release_frame_buffer_sem)(void* cookie);
        void (*free_vbs_buffer_sem)(void* cookie);
        
        void *cedarx_cookie;
    };


cedarv_decoder_t* libcedarv_init(s32 *ret);
s32 libcedarv_exit(cedarv_decoder_t* p);
#ifdef __cplusplus
}
#endif



#endif

