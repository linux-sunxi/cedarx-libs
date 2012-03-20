/*
********************************************************************************
*                                    eMOD
*                   the Easy Portable/Player Develop Kits
*                               mod_herb sub-system
*                            h264 video encode module
*
*          (c) Copyright 2009-2010, Allwinner Microelectronic Co., Ltd.
*                              All Rights Reserved
*
* File   : H264encLibApi.h
* Version: V1.0
* By     : Eric_wang
* Date   : 2009-12-25
* Description: ����mp4encodeLib�ṩ�ĸ��ⲿģ�����õ�ͷ�ļ���������ΪAPI�ļ�
********************************************************************************
*/
#ifndef _H264ENCLIBAPI_H___
#define _H264ENCLIBAPI_H___

#include "type.h"

#if 0
//#ifdef __CHIP_VERSION_F23
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
#endif

enum VENC_VIDEOINFO_TYPE
{
    VENC_SET_CSI_INFO_CMD = 1,
	VENC_SET_ENC_INFO_CMD,
	//VENC_SET_QUALITY_CMD,	//set picture quality, used by JPEG only, in this case uPara is the quality percent, for example, target quality is 80%, uPara is 80.
	//VENC_SET_FRAMERATE_CMD,	//set picture frame rate, uPara is the actual frame rate multiplied by 1000.
	//VENC_SET_BITRATE_CMD,	//set picture target bit rate,uPara is the target bit rate.
	//VENC_SET_KEYITL_CMD,	//set keyframe interval, after force keyframe flag set, it recounter again. default value is 25,and 0 is forbiden , ����I֮֡���P֡������
	//VENC_SET_COLORINFO_CMD,	//set color format and color space,
							//lowest 16LSBs are color space 0--yuv 4:2:0, 1:3--reserved, 4--bayer(BG/GR) , 5--bayer(RG/GB) , 6--bayer(GB/RG) , 7--bayer(GR/BG)
							//highest MSBs are color space 0--BT601, 1--BT709 but for jpeg always YCC.
	//VENC_SET_QPRANGE_CMD,	//set quant parameter range (lowest 16LSBs are min qp, highest MSBs are max qp)
	//VENC_SET_PICSIZE_CMD,	//set picture size, uPara is the actual size (lowest 16LSBs are width, highest MSBs are height)
	VENC_REGCB_CMD,			//register one call back function, the input parameter is the pointer to the parameter structure VEnc_IOCtrl_CB described below.
	VENC_INIT_CMD,
};

enum VEncCBType {
	VENC_CB_GET_FRMBUF, //register the GetFrmBufCB() function
	VENC_CB_WAIT_FINISH //register the WaitFinishCB() function
};

typedef __s32 (*VEncCBFunc)(__s32 uParam1,void *vParam2);

struct VEnc_IOCtrl_CB
{
	enum VEncCBType eCBType; //call back function type, one of element of ENUm VEncCBType defined below.
	VEncCBFunc pCB; //the call back function pointer defined as: typedef __s32 (*VEncCBFunc)(__s32 uParam1,void *vParam2);
};

typedef enum __PIC_ENC_RESULT
{
	PIC_ENC_ERR_NO_CODED_FRAME = -4,
	PIC_ENC_ERR_NO_MEM_SPACE = -3,
	PIC_ENC_ERR_VE_OVERTIME = -2, // video engine is busy, no picture is encoded
	PIC_ENC_ERR_VBS_UNOVERDERFLOW = -1, // video bitstream overderflow,��û��vbv bufferװ��coded frame.
	PIC_ENC_ERR_NONE = 0, // encode one picture successed
	PIC_ENC_ERR_
} __pic_enc_result;

typedef struct VEnc_FrmBuf_Info
{
	__u8 *addrY;		//the luma component address for yuv format, or the address for bayer pattern format
	__u8 *addrCb;		//the Cb component address for yuv format only.
	__u8 *addrCr;		//the Cr component address for yuv format only
    __u32 color_fmt;
    __u32 color_space;
	__s32 ForceKeyFrame;   //0----current frame may be encoded as non-key frame, 1----current frame must be encoded as key frame
	__s64 pts;        //unit:ms
    __s32 pts_valid;
	__u8 *bayer_y;
    __u8 *bayer_cb;
    __u8 *bayer_cr;
    __u32 *Block_Header;   //block header for olayer
    __u8 *Block_Data;     //block data for olayer
    __u32 *Palette_Data;   //palette data for olayer
    __u8 scale_mode;  //the THUMB scale_down coefficient
}VEnc_FrmBuf_Info;

typedef struct VBV_DATA_CTRL_INFO
{
    __s32       idx;    //��Ԫ���������е��±��,���ڹ黹.
    __u8        *pData0;
    __s32       uSize0;
    __u8        *pData1;
    __s32       uSize1;
    __s64       pts;
    __bool      pts_valid;
    //__bool      isMultipleFrame;
    //__u8        stc_id;
    //__s8        frm_dec_mode;
    //__u32       stream_len;         // G2 need this field.
	__s32       uLen0;       //buffer�е���Ч��ݵĳ���,audioʹ�ã�videoʹ��uSize,��Ϊһ����ȫ�����ꡣ
	__s32       uLen1;       //buffer�е���Ч��ݵĳ���
	__u8		*privateData;
	__s32		privateDataLen;
} __vbv_data_ctrl_info_t;

// typedef struct vdec_vbv_info
// {
//     __u8        *vbv_buf;
//     __u8        *vbv_buf_end;
//     __u8        *read_addr;
//     __u8        *write_addr;
//     __bool      hasPicBorder;
//     __bool      hasSliceStructure;
//     __vbv_data_ctrl_info_t vbv_data_info[MAX_PIC_NUM_IN_VBV];
//     __u16       PicReadIdx;
//     __u16       PicWriteIdx;
//     __u16       PicIdxSize;
//     __u32       valid_size;
//
// } __vdec_vbv_info_t;


typedef struct VENC_DEVICE
{
	void *priv_data;    //Mp4EncCtx
	void *pIsp;
	//struct vdec_vbv_info        vbvInfo;//��ű�����ɵ�֡����ݽṹ
	__s16 (*open)(struct VENC_DEVICE *p);
	__s16 (*close)(struct VENC_DEVICE *p);
	__s16 (*encode)(struct VENC_DEVICE *p);
	__s16 (*IoCtrl)(struct VENC_DEVICE *p, __u32, __u32);
    __s16 (*GetBitStreamInfo)(struct VENC_DEVICE *pDev, __vbv_data_ctrl_info_t *pdatainfo);
    __s16 (*ReleaseBitStreamInfo)(struct VENC_DEVICE *pDev, __s32 node_id);
	__s32 (*GetFrmBufCB)(__s32 uParam1,  void *pFrmBufInfo);
	__s32 (*WaitFinishCB)(__s32 uParam1, void *pMsg);

    //struct VEnc_FrmBuf_Info FrmBufInfo;
}VENC_DEVICE;

extern struct VENC_DEVICE *H264EncInit(int *ret);
extern __s16 H264EncExit(struct VENC_DEVICE *pDev);

//���ϲ�����ṩ
extern __s32 SysSendVEReadyFlag(void);


#endif  /* _H264ENCLIBAPI_H_ */

