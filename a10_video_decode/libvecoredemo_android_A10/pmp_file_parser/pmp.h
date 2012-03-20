
/*
************************************************************************************************************************
*                                            The PMP File Handler Module
*
*                                                All Rights Reserved
*
* Name     		: pmp.h
*
* Author        : XC
*
* Version       : 1.1.0
*
* Date          : 2009.04.22
*
* Description   : This module index and read media data from PMP file. To see
*                 how this module work, refer to 'pmp.c'
*
* Others        : None at present.
*
* History       :
*
*  <Author>     <time>      <version>   <description>
*
*     XC       2009.04.23     1.1.0
*
************************************************************************************************************************
*/
#ifndef PMP_H
#define PMP_H

#include <stdio.h>
#include <libcedarv.h>

#ifdef __cplusplus
extern "C" {
#endif

/************************************************************************/
/*                           Macro Definitions                          */
/************************************************************************/
#define VIDEO_PACKET_TYPE		(0)
#define AUDIO_PACKET_TYPE		(1)

//* You can close audio parsing if you set it to 0. @Clif.C
#define BISTREAM_PLAY_AUDIO    (1)

//* The first four bytes of pmp file must be "pmpm".
#define PMPFILETAG  0x6d706d70  //* "pmpm"

//* Define function return values.
#define PMP_PARSER_TRUE          1
#define PMP_PARSER_FALSE         0
#define PMP_PARSER_OK            0   //* no error happen;
#define PMP_PARSER_FAIL          -1  //* operation failed;
#define PMP_PARSER_E_POINTER     -2  //* pointer passed is a null pointer;
#define PMP_PARSER_IO_ERR        -3  //* cdx_read, cdx_tell or cdx_seek failed;
#define PMP_PARSER_E_FORMAT      -4  //* pmp file content error;
#define PMP_PARSER_NOT_SUPPORT   -5  //* operation or file version not support;
#define PMP_PARSER_E_MEMORY      -6  //* malloc fail;
#define PMP_PARSER_END           -7  //* pmp file is all parsed;

//* Define information bits for pmp_get_chunk_data()
#define PMP_PARSER_IS_VEDEO_DATA        0x1   //* media data is video data;
#define PMP_PARSER_IS_AUDIO_DATA        0x2   //* media data is audio data;
#define PMP_PARSER_IS_FIRST_PART        0x4   //* it is the first part of data;
#define PMP_PARSER_IS_LAST_PART         0x8   //* it is the last part of data;
#define PMP_PARSER_DATA_NOT_FINISHED    0x10  //* part of media data left for
                                              //* buffer is not big enough;

/************************************************************************/
/*                           Struct Definitions                         */
/************************************************************************/

//* Packet header struct.
typedef struct PMPPACKET
{
    u32   numAudioFrms;       //* how many audio frames in this packet;
    u32   firstDelay;         //* time delay between the first audio frame
                                //* and the video frame in this packet;
    u32   lastDelay;          //* time delay between the last audio frame
                                //* and the video frame in this packet;
    u32   vdFrmSize;          //* size of video data in this packet;
    u32   **auSize;           //* auSize[j][i] means the size of the ith
                                //* audio frame of the jth audio stream;
    u32   leftDataSize;       //* how many media data was left since last read;
    u32   leftAACHdrBytes;    //* how many bytes of aac header left;
    u32   leftAuFrm;          //* how many audio frames left since last read;

    u8    bAudioParsed;       //* is the audio data in this packet parsed;
    u8    bVideoParsed;       //* is the video data in this packet parsed;

    u64   vdPts;              //* pts of video frame in this packet;
    u64   auPts;              //* pts of first audio frame in this packet;

    u8    aac_hdr[7];         //* for AAC audio use.
    u8    *tmpBuf;            //* temporarily used buffer to read packet header;

}__PMPPACKET;

//* Key frame index entry.
typedef struct PMPKEYENTRY
{
    u32 frmID;                //* the frame number;
    u32 filePos;              //* file position of the key frame;

}__PMPKEYENTRY;

//* pmp file handler.
typedef struct  PMPFILEHANDLER
{
	u32           				pmpVesion;      //* 0 if version 1.0, 1 if version 2.0;
	u32           				vdCodecID;      //* 0-MP4V(xvid, divx), 1-AVC(h264);
	u32           				vdFrmNum;       //* total frame number of this file
	u32           				vdWidth;        //* video frame width;
	u32           				vdHeight;       //* video frame height;
	u32           				vdScale;        //* video fps = rate/scale;
	u32           				vdRate;         //* used with vdScale to de

	u32           				auCodecID;      //* 0-MP3, 1-AAC;
	u32           				auStrmNum;      //* audio stream number in this file;
    u32           				maxAuPerFrame;  //* max audio frame within one packet;
    u32           				auScale;        //* default is 1152;
    u32           				auRate;         //* default is 44100;
    u32           				bStereo;        //* audio is Stereo;
    u32           				auId;           //* which audio stream is to be play;

    u32           				pktCounter;     //* current packet we are going to
                      				            //* parse, start counting from 1
    u32           				vdFrmInterval;  //* time interval between two video frames;

    FILE*						fpMediaFile;   	//* file pointer
    __PMPPACKET     			pkt;            //* packet header information;

    __PMPKEYENTRY*				pKeyFrmIdx;    	//* key frame index table;
    u32           				keyIdxNum;      //* how many entry in key frame index table;

    cedarv_stream_info_t 		vFormat; 		//* video format information;

#if 0
    CedarXAudioStreamInfo 		aFormat; 		//* audio format information;
#endif

    u32           				bDiscardAudio;  //* whether to discard audio data;

}__PMPFILEHANDLER;


/************************************************************************/
/*                   Module Interfaces Definition                       */
/************************************************************************/
extern s32 pmp_create(__PMPFILEHANDLER **ppHdlr);

extern s32 pmp_destroy(__PMPFILEHANDLER **ppHdlr);

extern s32 pmp_set_input_media_file(__PMPFILEHANDLER *pHdlr, FILE* fp);

extern s32 pmp_get_chunk_info(__PMPFILEHANDLER *pHdlr, s32 *pChunkType, s32* pPktSize);

extern s32 pmp_get_chunk_data(__PMPFILEHANDLER *pHdlr, u8 *pBuf, u32 nBufSize, u32 *pDataSize, u32 *pInformation);

extern s32 pmp_skip_chunk_data(__PMPFILEHANDLER *pHdlr);

extern u64 pmp_get_pts(__PMPFILEHANDLER *pHdlr, u8 bAudio);

#if 0
extern s32 pmp_get_audio_codec_id(__PMPFILEHANDLER *pHdlr);
#endif

extern s32 pmp_get_video_codec_id(__PMPFILEHANDLER *pHdlr);

#if 0
extern s32 pmp_get_audio_format(__PMPFILEHANDLER *pHdlr, CedarXAudioStreamInfo **ppAfmt);
#endif

extern s32 pmp_get_video_format(__PMPFILEHANDLER *pHdlr, cedarv_stream_info_t **ppVfmt);

extern s32 pmp_seek_to_key_frame(__PMPFILEHANDLER* pHdlr, u32 time, u32 bBackward);

extern s32 pmp_seek_to_next_key_frame(__PMPFILEHANDLER* pHdlr, u32 timeInterval, u32 bBackward);

extern s32 pmp_has_index_table(__PMPFILEHANDLER* pHdlr);

extern u32 pmp_get_file_duration(__PMPFILEHANDLER* pHdlr);

extern s32 pmp_set_audio_discard_flag(__PMPFILEHANDLER* pHdlr, u32 bDiscardAudio);

#ifdef __cplusplus
}
#endif


#endif /* PMP_H */
