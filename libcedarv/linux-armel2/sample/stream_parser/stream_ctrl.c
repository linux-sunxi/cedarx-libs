
#include <malloc.h>
#include <memory.h>
#include <stdio.h>
#include "stream_ctrl.h"
s32 OpenMediaFile(void** ppCtrl, const char* file_path)
{
	stream_handle * handle;

	handle = (stream_handle *)malloc(sizeof(stream_handle));
	if(handle == NULL)
	{
		__msg("malloc handle failed");
		return -1;
	}

	memset(handle, 0, sizeof(stream_handle));
	handle->fp = fopen(file_path, "rb");
	if(handle->fp == NULL)
	{
		__msg("open file error");
		free(handle);
		handle = NULL;
		return -1;
	}
	handle->stream_info.format 			 = STREAM_FORMAT_H264;
	handle->stream_info.sub_format 		 = CEDARV_SUB_FORMAT_UNKNOW;
	handle->stream_info.container_format = CONTAINER_FORMAT_UNKNOW;
	handle->stream_info.video_width 	 = 640;
	handle->stream_info.video_height	 = 480;
	handle->stream_info.frame_rate		 = 30*1000;
	handle->stream_info.frame_duration	 = 0;
	handle->stream_info.aspect_ratio	 = 800/480*1000;
	handle->stream_info.frame_rate		 = 30*1000;
	handle->stream_info.init_data_len	 = 0;
	handle->stream_info.init_data		 = 0;

	*ppCtrl = handle;
	return 0;
}

s32 CloseMediaFile(void** ppCtrl)
{
	stream_handle * handle;

	handle = *ppCtrl;
	if(handle->fp)
		fclose(handle->fp);
	if(handle)
		free(handle);
	return 0;
}

s32 GetNextChunkInfo(void* pCtrl, u32* pChunkType, u32* pChunkLen)
{
	stream_handle * handle;
	unsigned char buf[6];
	unsigned int  len;
	unsigned int pos;

	int ret;

	handle = pCtrl;
    len = 0;
    ret = -1;

    memset(buf, 0, 6);
    handle->nal_pos = handle->next_nal;
	fseek(handle->fp, handle->next_nal, SEEK_SET);
	fseek(handle->fp, 4, SEEK_CUR);

    do
    {
    	pos = ftell(handle->fp);
    	len = fread(buf, 1, 5, handle->fp);
    	if(len != 5)
    		break;
    	if((buf[0] == 0x00) && (buf[1] == 0x00) && (buf[2] == 0x00) && (buf[3] == 0x01) &&(buf[4] != 0x68))
    	{
//    		__msg("find start code. pos %x, nal %x", pos, buf[4]);
    		handle->next_nal = pos;
    		ret = 0;
    		break;
    	}
    	else
    	{
    		fseek(handle->fp, -4, SEEK_CUR);
    	}

    }while(!feof(handle->fp));

    if(feof(handle->fp))
    {
    	handle->next_nal = ftell(handle->fp);
    	ret = 0;
    }
    if(handle->next_nal == handle->nal_pos)
    	ret = -1;

    if(ret == 0)
    {
    	*pChunkType = VIDEO_PACKET_TYPE;
    	*pChunkLen 	= handle->next_nal - handle->nal_pos;
    	handle->data_len = handle->next_nal - handle->nal_pos;
    }

	return ret;
}
s32 GetChunkData(void* pCtrl, u8* buf0, u32 buf_size0, u8* buf1, u32 buf_size1, cedarv_stream_data_info_t* data_info)
{
	stream_handle *handle;
	int len;
	int tmp;
	int ret;

	handle = pCtrl;
	len = 0;
	tmp = 0;
	ret = 0;

	memset(data_info, 0, sizeof(cedarv_stream_data_info_t));
	fseek(handle->fp, handle->nal_pos, SEEK_SET);
	if(handle->data_len <= buf_size0)
	{
		len = fread(buf0, 1, handle->data_len, handle->fp);
		if(len != handle->data_len)
			ret = -1;
		data_info->lengh = handle->data_len;
	}
	else
	{
		len = fread(buf0, 1, buf_size0, handle->fp);
		if(len != buf_size0)
			ret = -1;
		tmp = handle->data_len - buf_size0;
		if(tmp <= buf_size1)
		{
			len = fread(buf1, 1, tmp, handle->fp);
			if(len != tmp)
				ret = -1;

			data_info->lengh = handle->data_len;
		}
		else
		{
			len = fread(buf1, 1, buf_size1, handle->fp);
			if(len != buf_size1)
				ret = -1;
			data_info->lengh = buf_size0 + buf_size1;
		}
	}
	data_info->flags |= CEDARV_FLAG_FIRST_PART;
	data_info->flags |= CEDARV_FLAG_LAST_PART;

	return ret;
}
s32 GetVideoStreamInfo(void* pCtrl, cedarv_stream_info_t* vstream_info)
{
	stream_handle * handle;
	handle = pCtrl;
	if(handle)
	{
		memcpy(vstream_info, &handle->stream_info, sizeof(cedarv_stream_info_t));
		return 0;
	}
	return -1;
}
s32 SkipChunkData(void* pCtrl)
{
	stream_handle *handle;

	handle = pCtrl;
	handle->nal_pos = handle->next_nal;
	fseek(handle->fp, handle->next_nal, SEEK_SET);
	return 0;
}
