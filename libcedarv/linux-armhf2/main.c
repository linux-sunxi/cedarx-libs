#include <stdio.h>
#include <unistd.h>
#include <malloc.h>
#include <pthread.h>

#include "vdecoder/libcedarv.h"	//* for decoding video
#include "file_parser/pmp_ctrl.h"

static pthread_mutex_t gDecoderMutex = PTHREAD_MUTEX_INITIALIZER;
static const char* gMediaFilePath[4] =
{
	"/mnt/1.pmp",
	"/mnt/2.pmp",
	"/mnt/3.pmp",
	"/mnt/4.pmp"
};

void* decode_thread(void* param)
{
	int            ret;
	int            pkt_type;
	int            pkt_length;
	unsigned char* buf0;
	unsigned int   bufsize0;
	unsigned char* buf1;
	unsigned int   bufsize1;

	cedarv_stream_data_info_t data_info;
	cedarv_stream_info_t      stream_info;
	cedarv_picture_t          picture;

	void*             parser;
	cedarv_decoder_t* decoder;

	char* file_path;
	int   decode_thread_id;

	//*********************************************************************
	decode_thread_id = (int)param;
	file_path = (char*)gMediaFilePath[decode_thread_id];


	//*********************************************************************
	//* 1. open the file parser.
	//*********************************************************************
	ret = OpenMediaFile(&parser, file_path);
	if(ret < 0)
	{
		printf("can not create file parser for media file %s\n", file_path);
		return (void*)-1;
	}

	//*********************************************************************
	//* 2. open the decoder.
	//*********************************************************************

	//* a. initialize libcedarv.
	pthread_mutex_lock(&gDecoderMutex);
	decoder = libcedarv_init(&ret);
	pthread_mutex_unlock(&gDecoderMutex);

	if(ret < 0)
	{
		printf("can not initialize the decoder library.\n");
		CloseMediaFile(&parser);
		return (void*)-1;
	}

	//* b. set video stream information to libcedarv.
	GetVideoStreamInfo(parser, &stream_info);
	decoder->set_vstream_info(decoder, &stream_info);		//* this decoder operation do not use hardware, so need not lock the mutex.

	//* c. open libcedarv.
	pthread_mutex_lock(&gDecoderMutex);
	ret = decoder->open(decoder);
	pthread_mutex_unlock(&gDecoderMutex);
	if(ret < 0)
	{
		printf("can not open decoder.\n");
		if(stream_info.init_data)
			free(stream_info.init_data);
		CloseMediaFile(&parser);

		pthread_mutex_lock(&gDecoderMutex);
		libcedarv_exit(decoder);
		pthread_mutex_unlock(&gDecoderMutex);

		return (void*)-1;
	}

	pthread_mutex_lock(&gDecoderMutex);
	decoder->ioctrl(decoder, CEDARV_COMMAND_PLAY, 0);
	pthread_mutex_unlock(&gDecoderMutex);

	//*********************************************************************
	//* 3. decoding loop.
	//*********************************************************************
	do
	{
		//* a. get media type of the next data packet.
		ret = GetNextChunkInfo(parser, (unsigned int*)&pkt_type, (unsigned int*)&pkt_length);
		if(ret < 0)
		{
			printf("get packet information fail, may be file end.");
			break;
		}

		//* b. read packet to decoder and decode.
		if(pkt_type == VIDEO_PACKET_TYPE)
		{
			//* request bit stream data buffer from libcedarv.
			if(pkt_length == 0)	//* for some file format, packet length may be unknown.
				pkt_length = 64*1024;

_read_again:
			ret = decoder->request_write(decoder, pkt_length, &buf0, &bufsize0, &buf1, &bufsize1);
			if(ret < 0)
			{
				//* request bit stream data buffer fail, may be the bit stream FIFO is full.
				//* in this case, we should call decoder->decode(...) to decode stream data and release bit stream buffer.
				//* here we just use a single thread to do the data parsing/decoding/picture requesting work, so it is
				//* invalid to see that the bit stream FIFO is full.
				printf("request bit stream buffer fail.\n");
				break;
			}

			//* read bit stream data to the buffer.
			GetChunkData(parser, buf0, bufsize0, buf1, bufsize1, &data_info);

			//* tell libcedarv stream data has been added.
			decoder->update_data(decoder, &data_info);		//* this decoder operation do not use hardware, so need not lock the mutex.

			//* decode bit stream data.
			pthread_mutex_lock(&gDecoderMutex);
			ret = decoder->decode(decoder);
			pthread_mutex_unlock(&gDecoderMutex);
			
			printf("decoder %d return %d\n", decode_thread_id, ret);
			if(ret == CEDARV_RESULT_ERR_NO_MEMORY || ret == CEDARV_RESULT_ERR_UNSUPPORTED)
			{
				printf("bit stream is unsupported.\n");
				break;
			}

			//* request picture from libcedarv.
			ret = decoder->display_request(decoder, &picture);		//* this decoder operation do not use hardware, so need not lock the mutex.
			if(ret == 0)
			{
				//* get one picture from decoder success, do some process work on this picture.
				usleep(1000*30);
				
				//* release the picture to libcedarv.
				decoder->display_release(decoder, picture.id);		//* this decoder operation do not use hardware, so need not lock the mutex.
			}
		}
		else
		{
			//* skip audio or other media packets.
			SkipChunkData(parser);
		}

	}while(1);

	//* 4. close the decoder and parser.
	pthread_mutex_lock(&gDecoderMutex);
	decoder->ioctrl(decoder, CEDARV_COMMAND_STOP, 0);
	pthread_mutex_unlock(&gDecoderMutex);

	pthread_mutex_lock(&gDecoderMutex);
	decoder->close(decoder);
	libcedarv_exit(decoder);
	pthread_mutex_unlock(&gDecoderMutex);

	if(stream_info.init_data)
		free(stream_info.init_data);

	CloseMediaFile(&parser);

	return (void*)0;
}


int main(int argc, char** argv)
{
	int ret;
	pthread_t t0;
	pthread_t t1;
	pthread_t t2;
	pthread_t t3;

	printf("begin cedarx hardware init.\n");
	cedarx_hardware_init(0);
		
	printf("create four decode task.\n");

	pthread_create(&t0, NULL, decode_thread, (void*)0);
	pthread_create(&t1, NULL, decode_thread, (void*)1);
	pthread_create(&t2, NULL, decode_thread, (void*)2);
	pthread_create(&t3, NULL, decode_thread, (void*)3);

	pthread_join(t0, (void**)&ret);
	pthread_join(t1, (void**)&ret);
	pthread_join(t2, (void**)&ret);
	pthread_join(t3, (void**)&ret);
	printf("decode task all finish.\n");
	
	printf("begin cedarx hardware exit.\n");
	cedarx_hardware_exit(0);

	printf("success, quit\n");
	return 0;
}



