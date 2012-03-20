#include <stdio.h>
#include <unistd.h>
#include <malloc.h>
//#include <libcedarv.h>	//* for decoding video
#include <pmp_ctrl.h>	//* for parsing PMP file

int main(int argc, char *argv[])
{
	s32					 		ret;
	u32					 		pkt_type;
	u32					 		pkt_length;

	u8*					 		buf0;
	u32					 		bufsize0;
	u8*					 		buf1;
	u32					 		bufsize1;

	cedarv_stream_data_info_t 	data_info;

	cedarv_stream_info_t 		stream_info;
	cedarv_picture_t			picture;

	void*				 		hpmp;
	cedarv_decoder_t*    		hcedarv;

	FILE*						fpSave;

	const char* 				media_file_path = "./test2.pmp";

	fpSave = NULL;

	//* 1. open pmp file parser.
	ret = OpenMediaFile(&hpmp, media_file_path);
	if(ret < 0 || hpmp == NULL)
	{
		printf("open pmp file parser fail.\n");
		return -1;
	}

	//* 2. get video stream information from parser.
	GetVideoStreamInfo(hpmp, &stream_info);
	if(stream_info.format == CEDARV_STREAM_FORMAT_UNKNOW)
	{
		printf("unknown video format, test program quit.");
		CloseMediaFile(&hpmp);
		return -1;
	}

	//* 3. initialize libcedarv.
	hcedarv = libcedarv_init(&ret);
	if(ret < 0 || hcedarv == NULL)
	{
		printf("libcedarv_init fail, test program quit.");
		if(stream_info.init_data)
			free(stream_info.init_data);

		CloseMediaFile(&hpmp);
		return -1;
	}

	//* 4. set video stream information to libcedarv.
	ret = hcedarv->set_vstream_info(hcedarv, &stream_info);
	if(ret < 0)
	{
		printf("set video stream information to libcedarv fail, test program quit.");

		if(stream_info.init_data)
			free(stream_info.init_data);

		CloseMediaFile(&hpmp);

		libcedarv_exit(hcedarv);

		return -1;
	}

	//* 4.5. before libcedarv is opened, you can use the hcedarv->ioctrl(...) to set libcedarv's working mode(preview mode) or
	//*      open the rotation/scale down function of libvecore.

	//* 5. open libcedarv.
	ret = hcedarv->open(hcedarv);
	if(ret < 0)
	{
		printf("open libcedarv fail, test program quit.");

		if(stream_info.init_data)
			free(stream_info.init_data);

		CloseMediaFile(&hpmp);

		libcedarv_exit(hcedarv);

		return -1;
	}

	//* 6. tell libcedarv to start.
	hcedarv->ioctrl(hcedarv, CEDARV_COMMAND_PLAY, 0);

	//* 7. loop to parse video data from the pmp parser, call libcedarv to decode, and request video pictures from libcedarv.
	do
	{
		//* 7.1. get media type of the next data packet.
		ret = GetNextChunkInfo(hpmp, &pkt_type, &pkt_length);
		if(ret < 0)
		{
			printf("get packet information fail, may be file end.\n");
			break;
		}

		if(pkt_type == VIDEO_PACKET_TYPE)
		{
			//* 7.2 request bitstream data buffer from libcedarv.
			if(pkt_length == 0)	//* for some file format, packet length may be unknown.
				pkt_length = 64*1024;

_read_again:
			ret = hcedarv->request_write(hcedarv, pkt_length, &buf0, &bufsize0, &buf1, &bufsize1);
			if(ret < 0)
			{
				//* request bitstream data buffer fail, may be the vbv bitstream fifo is full.
				//* in this case, we should call hcedarv->decode(...) to decode stream data and release bitstream buffer.
				//* here we just use a single thread to do the data parsing/decoding/picture requesting work, so it is
				//* invalid to see that the vbv bitstream fifo is full.
				printf("request bitstream buffer fail.");
				break;
			}



			//* 7.3 read bitstream data to the buffer.
			ret = GetChunkData(hpmp, buf0, bufsize0, &data_info);
			if(ret < 0)
			{
				printf("read video stream data fail.");
				break;
			}

			//* 7.4 update data to libcedarv.
			hcedarv->update_data(hcedarv, &data_info);

			if(ret == 0)
			{
				//* GetChunkData not finish, some video data left because the buffer given is not enough to store one bitstream frame.
				//* In this case, request buffer again to read the left data.
				goto _read_again;
			}

			//* 7.5 decode bitstream data.
			ret = hcedarv->decode(hcedarv);
			if(ret == CEDARV_RESULT_ERR_NO_MEMORY || ret == CEDARV_RESULT_ERR_UNSUPPORTED)
			{
				printf("bitstream is unsupported.");
				break;
			}

			//* 7.6 request picture from libcedarv.
			ret = hcedarv->display_request(hcedarv, &picture);
			if(ret == 0)
			{
				//* get one picture from decoder.
				printf("get one picture from decoder, picture id = %d, video width = %d, video height = %d, pts = %d.\n",
						picture.id, picture.display_width, picture.display_height, (u32)picture.pts);

				//* you can do some thing to display this picture now.

				//* after one picture is displayed, you should return it to libcedarv to release the picture frame buffer.
				hcedarv->display_release(hcedarv, picture.id);

				//* note: you can request more than one picture.
			}
		}
		else
		{
			//* skip audio or other media packets.
			SkipChunkData(hpmp);
		}
	}while(1);

	//* 8. quit the test program.
	hcedarv->ioctrl(hcedarv, CEDARV_COMMAND_STOP, 0);
	hcedarv->close(hcedarv);
	libcedarv_exit(hcedarv);

	if(stream_info.init_data)
		free(stream_info.init_data);

	CloseMediaFile(&hpmp);

	return 0;
}















