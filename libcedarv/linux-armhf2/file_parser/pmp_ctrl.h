/*
************************************************************************************************************************
*                                                 The PMP Parser Module
*
*                                                   All Rights Reserved
*
* Name     : pmp_ctrl.h
*
* Author        : XC
*
* Version       : 1.1.0
*
* Date          : 2009.04.22
*
* Description   : This module implement media control of pmp file, such as AVSync, FFRR, Jump
*                 Play and Normal Play. To see how this module work, refer to 'pmp_ctrl.c'.
*
* Others        : None at present.
*
* History       :
*
*  <Author>     <time>      <version>   <description>
*
*  XC         2009.04.23      1.1.0
*
************************************************************************************************************************
*/

#ifndef PMP_CONTROL_H
#define PMP_CONTROL_H

//#include <libcedarv.h>
#include "pmp.h"

#ifdef __cplusplus
extern "C" {
#endif

	typedef struct PMPCTRLBLK
	{
    	__PMPFILEHANDLER*	pmpHdlr;       //* handler to read media data from pmp file;
    	u32					bSendingFrm;
	}__PMPCTRLBLK;


	s32 OpenMediaFile(void** ppCtrl, const char* file_path);
	s32 CloseMediaFile(void** ppCtrl);
	s32 GetNextChunkInfo(void* pCtrl, u32* pChunkType, u32* pChunkLen);
	s32 GetChunkData(void* pCtrl, u8* buffer, u32 buffer_len, u8 *buf1, u32 buf_size1, cedarv_stream_data_info_t* data_info);
	s32 SkipChunkData(void* pCtrl);
	s32 GetVideoStreamInfo(void* pCtrl, cedarv_stream_info_t* vstream_info);


#ifdef __cplusplus
}
#endif

#endif
