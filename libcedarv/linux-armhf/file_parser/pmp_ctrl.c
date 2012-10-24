
#include <malloc.h>
#include <memory.h>
#include <stdio.h>
#include "pmp_ctrl.h"

s32 OpenMediaFile(void** ppCtrl, const char* file_path)
{
	s32 			retVal;
	FILE* 			fp;
	__PMPCTRLBLK* 	pCtrl;

	retVal  = 0;
	fp 	    = NULL;
	pCtrl   = NULL;
    *ppCtrl = NULL;

    fp = fopen(file_path, "rb");
    if (!fp){
		printf("media file open fail\n");
        goto _err_open_media_file;
    }
    //* create controler
    pCtrl = (__PMPCTRLBLK*) malloc(sizeof(__PMPCTRLBLK));
    if(!pCtrl)
        goto _err_open_media_file;

    memset(pCtrl, 0, sizeof(__PMPCTRLBLK));

    //* create pmp file handler
    retVal = pmp_create(&pCtrl->pmpHdlr);
    if (retVal != PMP_PARSER_OK)
        goto _err_open_media_file;

    //* pass the file pointer to pmp file handler
    retVal = pmp_set_input_media_file(pCtrl->pmpHdlr, fp);
    if (retVal != PMP_PARSER_OK)
        goto _err_open_media_file;

    fp = NULL;

    *ppCtrl = pCtrl;
    return 0;

_err_open_media_file:

    if (fp)
    {
        fclose(fp);
        fp = NULL;
    }

    if(pCtrl)
    {
        if (pCtrl->pmpHdlr)
        {
            pmp_destroy(&pCtrl->pmpHdlr);
        }

        free(pCtrl);
        pCtrl = 0;
    }

    return -1;
}


s32 CloseMediaFile(void** ppCtrl)
{
    __PMPCTRLBLK* p;

    if (ppCtrl)
    {
        p = *((__PMPCTRLBLK**)ppCtrl);
        if (p)
        {
            if(p->pmpHdlr)
                pmp_destroy(&p->pmpHdlr);

            free(p);
            *ppCtrl  = 0;
        }
    }

    return 0;
}


s32 GetNextChunkInfo(void* pCtrl, u32* pChunkType, u32* pChunkLen)
{
    s32            	  retVal;
    __PMPCTRLBLK*	  p;
    __PMPFILEHANDLER* pHdlr;
    s32				  pkt_type;
    s32				  pkt_size;

    if (!pCtrl)
        return -1;

    p     = (__PMPCTRLBLK*) pCtrl;
    pHdlr = p->pmpHdlr;
    if (!pHdlr)
        return -1;

    //* Get media type of new data.
    retVal = pmp_get_chunk_info(pHdlr, &pkt_type, &pkt_size);
    if (retVal != PMP_PARSER_OK)
    {
    	return -1;
    }

    *pChunkType = pkt_type;	//* VIDEO_PACKET_TYPE or AUDIO_PACKET_TYPE
    *pChunkLen  = pkt_size;

    return 0;
}


s32 GetChunkData(void* pCtrl, u8* buffer, u32 buffer_len, u8 * buf1, u32 buf_size1, cedarv_stream_data_info_t* data_info)
{
    s32               retVal;
    u32               ctrlBits;
    __PMPCTRLBLK*     p;
    __PMPFILEHANDLER* pHdlr;

    if (pCtrl == NULL || buffer == NULL || data_info == NULL)
        return PMP_PARSER_E_POINTER;

    p     = (__PMPCTRLBLK*) pCtrl;
    pHdlr = p->pmpHdlr;
    if(!pHdlr)
        return -1;

    //* Read data to buffer.
    retVal = pmp_get_chunk_data(pHdlr, buffer, buffer_len, &data_info->lengh, &ctrlBits);
    if (retVal != PMP_PARSER_OK)
        return -1;

	data_info->flags = 0;
    //* ctrlBits store some information of the media data.
    if (ctrlBits & PMP_PARSER_IS_VEDEO_DATA)
    {
    	data_info->pts = pmp_get_pts(pHdlr, 0);		//* get video pts
    	data_info->flags |= CEDARV_FLAG_PTS_VALID;
    }
    else if (ctrlBits & PMP_PARSER_IS_AUDIO_DATA)
    {
    	data_info->pts = pmp_get_pts(pHdlr, 1);		//* get video pts
    	data_info->flags |= CEDARV_FLAG_PTS_VALID;
    }

    if (ctrlBits & PMP_PARSER_IS_FIRST_PART)
    	data_info->flags |= CEDARV_FLAG_FIRST_PART;

    if (ctrlBits & PMP_PARSER_IS_LAST_PART)
    	data_info->flags |= CEDARV_FLAG_LAST_PART;

    if (ctrlBits & PMP_PARSER_DATA_NOT_FINISHED)
    {
        //* only part of media data is copied to buffer for the
        //* buffer size is not enough.
        p->bSendingFrm = 1;
        return 0;
    }
    else
    {
        p->bSendingFrm = 0;
        return 1;	//* send one frame finish.
    }
}


s32 SkipChunkData(void* pCtrl)
{
    __PMPCTRLBLK*		p;
    __PMPFILEHANDLER*	pHdlr;

    p     = (__PMPCTRLBLK*) pCtrl;
    pHdlr = p->pmpHdlr;

	pmp_skip_chunk_data(pHdlr);

	return 0;
}


s32 GetVideoStreamInfo(void* pCtrl, cedarv_stream_info_t* vstream_info)
{
	s32						retVal;
    __PMPCTRLBLK*			p;
    __PMPFILEHANDLER*		pHdlr;
    cedarv_stream_info_t*	tmp_stream_info;

    if(!pCtrl)
        return -1;

    p     = (__PMPCTRLBLK*)pCtrl;
    pHdlr = p->pmpHdlr;
    if(!pHdlr)
        return -1;

    retVal = pmp_get_video_format(pHdlr, &tmp_stream_info);
    if(retVal == PMP_PARSER_OK)
    {
    	memcpy(vstream_info, tmp_stream_info, sizeof(cedarv_stream_info_t));
    	if(tmp_stream_info->init_data)
    	{
    		vstream_info->init_data = (u8*)malloc(tmp_stream_info->init_data_len);
    		if(vstream_info->init_data == NULL)
    		{
    			printf("allocate memory for video stream initial data fail.\n");
    			return -1;
    		}

    		memcpy(vstream_info->init_data, tmp_stream_info->init_data, tmp_stream_info->init_data_len);
    		vstream_info->init_data_len = tmp_stream_info->init_data_len;
    	}
    	else
    	{
    		vstream_info->init_data = NULL;
    		vstream_info->init_data_len = 0;
    	}

    	return 0;
    }
    else
    {
    	printf("get video format fail.\n");
    	return -1;
    }
}


