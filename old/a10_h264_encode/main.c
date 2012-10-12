
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "type.h"
#include "H264encLibApi.h"
#include "capture.h"

VENC_DEVICE *g_pCedarV = NULL;
int g_cur_id = -1;

static __s32 WaitFinishCB(__s32 uParam1, void *pMsg)
{
	return cedarv_wait_ve_ready();
}

static __s32 GetFrmBufCB(__s32 uParam1,  void *pFrmBufInfo)
{
	int ret = -1;
	V4L2BUF_t Buf;
	VEnc_FrmBuf_Info encBuf;

	// get one frame
	ret = GetPreviewFrame(&Buf);
	if (ret != 0)
	{
		printf("GetPreviewFrame failed\n");
		return -1;
	}

	memset((void*)&encBuf, 0, sizeof(VEnc_FrmBuf_Info));
	encBuf.addrY = Buf.addrPhyY;
	encBuf.addrCb = Buf.addrPhyY + 800 * 480;
	encBuf.pts_valid = 1;
	encBuf.pts = Buf.timeStamp;
	encBuf.color_fmt = PIXEL_YUV420;
	encBuf.color_space = BT601;

	memcpy(pFrmBufInfo, (void*)&encBuf, sizeof(VEnc_FrmBuf_Info));

	g_cur_id = Buf.index;

	return 0;
}

static int CedarvEncInit()
{
	int ret = -1;

	VENC_DEVICE *pCedarV = NULL;

	pCedarV = H264EncInit(&ret);
	if (ret < 0)
	{
		printf("H264EncInit failed\n");
		return -1;
	}

	__video_encode_format_t enc_fmt;
	enc_fmt.width = 640;
	enc_fmt.height = 480;
	enc_fmt.frame_rate = 30 * 1000;
	enc_fmt.color_format = PIXEL_YUV420;
	enc_fmt.color_space = BT601;
	enc_fmt.qp_max = 40;
	enc_fmt.qp_min = 20;
	enc_fmt.avg_bit_rate = 1024*1024;
	enc_fmt.maxKeyInterval = 25;
	pCedarV->IoCtrl(pCedarV, VENC_SET_ENC_INFO_CMD, &enc_fmt);

	ret = pCedarV->open(pCedarV);
	if (ret < 0)
	{
		printf("open H264Enc failed\n");
		return -1;
	}
	printf("open H264Enc ok\n");

	pCedarV->GetFrmBufCB = GetFrmBufCB;
	pCedarV->WaitFinishCB = WaitFinishCB;

	g_pCedarV = pCedarV;

	return ret;
}

int main()
{
	int ret = -1;
	unsigned long long lastTime = 0 ;
	FILE * pEncFile = NULL;
	char saveFile[128] = "h264.buf";
	int bFirstFrame = 1;

	ret = cedarx_hardware_init();
	if (ret < 0)
	{
		printf("cedarx_hardware_init failed\n");
	}

	ret = InitCapture();
	if(ret != 0)
	{
		printf("InitCapture failed\n");
		return -1;
	}

	ret = CedarvEncInit();
	if (ret != 0)
	{
		printf("CedarvEncInit failed\n");
		return -1;
	}

	pEncFile = fopen(saveFile, "wb+");
	if (pEncFile == NULL)
	{
		printf("open %s failed\n", saveFile);
		return -1;
	}

	printf("to stream on\n");
	StartStreaming();

	lastTime = gettimeofday_curr();
	while(1)
	{
		__vbv_data_ctrl_info_t data_info;
		unsigned long long curTime = gettimeofday_curr();

		printf("cru: %lld, last: %lld, %lld\n", curTime, lastTime, curTime - lastTime);
		if ((curTime - lastTime) > 1000*1000*5)	// 30s
		{
			ReleaseFrame(g_cur_id);
			goto EXIT;
		}

		ret = g_pCedarV->encode(g_pCedarV);
		if (ret != 0)
		{
			usleep(10000);
			printf("not encode, ret: %d\n", ret);
		}

		ReleaseFrame(g_cur_id);

		memset(&data_info, 0 , sizeof(__vbv_data_ctrl_info_t));
		ret = g_pCedarV->GetBitStreamInfo(g_pCedarV, &data_info);
		if(ret == 0)
		{
			if (1 == bFirstFrame)
			{
				bFirstFrame = 0;
				fwrite(data_info.privateData, data_info.privateDataLen, 1, pEncFile);
			}

			if (data_info.uSize0 != 0)
			{
				fwrite(data_info.pData0, data_info.uSize0, 1, pEncFile);
			}

			if (data_info.uSize1 != 0)
			{
				fwrite(data_info.pData1, data_info.uSize1, 1, pEncFile);
			}
			printf("v GetBS OK\n");
		}

		g_pCedarV->ReleaseBitStreamInfo(g_pCedarV, data_info.idx);
	}

EXIT:
	DeInitCapture();

	if (pEncFile != NULL)
	{
		fclose(pEncFile);
		pEncFile = 0;
	}

	if (g_pCedarV != NULL)
	{
		g_pCedarV->close(g_pCedarV);
		H264EncExit(g_pCedarV);
		g_pCedarV = NULL;
	}

	cedarx_hardware_exit();

	return 0;
}


