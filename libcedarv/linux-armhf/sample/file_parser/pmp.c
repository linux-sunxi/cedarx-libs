
/*
************************************************************************************************************************
*                                            The PMP File Handler Module
*
*                                                All Rights Reserved
*
* Name     		: pmp.c
*
* Author        : XC
*
* Version       : 1.1.0
*
* Date          : 2009.04.22
*
* Description   : This module index and read media data from PMP file.
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
#include <malloc.h>
#include <memory.h>
#include "pmp.h"

/************************************************************************/
/*                           Macro Definitions                          */
/************************************************************************/

//* Max key frame number in pmp file. @Clif.C
#define MAX_KEY_FRM_NUM         (4*3600*2)

//* PMP file version, only support PMP2.0 currently. @XC
#define PMP_VERSION_10  0
#define PMP_VERSION_20  1

//* The PMP file header size of Version 2.0 is 52 bytes. @XC
#define PMP_FILE_HEADER_SIZE    52

//* this table is for AAC audio. @Clif.C
#define NUM_SAMPLE_RATES    12
static const u32 sampRateTab[NUM_SAMPLE_RATES] =
{
    96000, 88200, 64000, 48000, 44100, 32000,
	24000, 22050, 16000, 12000, 11025,  8000
};


/************************************************************************/
/*                           Struct Definitions                         */
/************************************************************************/
typedef struct PMPINDEXREC
{
	u32       bKeyFrame       :1;
	u32       nFrmSize        :31;
}__pmpIndexRec;


/************************************************************************/
/*                     static functions announce                        */
/************************************************************************/

static s32 pmp_read_headers(__PMPFILEHANDLER *pHdlr);

static s32 pmp_check_header(__PMPFILEHANDLER *pHdlr);

static s32 pmp_packet_create(__PMPFILEHANDLER *pHdlr);

static s32 pmp_packet_destroy(__PMPFILEHANDLER *pHdlr);

static s32 pmp_idxtab_build(__PMPFILEHANDLER *pHdlr);

static s32 pmp_idxtab_destroy(__PMPFILEHANDLER *pHdlr);

static void pmp_set_media_format_info(__PMPFILEHANDLER *pHdlr);


/************************************************************************/
/*                    Module Interfaces Definition                      */
/************************************************************************/

/*
*********************************************************************************************************
*                               pmp_create
*
*Description: create pmp file handler;
*
*Arguments  : ppHdlr    - the address to store pmp file handler;
*
*Return     : PMP_PARSER_OK         - successed;
*             PMP_PARSER_E_MEMORY   - memory malloc failed;
*
*Summary    : the pmp file handler is responsed on media reading and indexing;
*
*Author     : XC
*
*********************************************************************************************************
*/
s32 pmp_create(__PMPFILEHANDLER  **ppHdlr)
{
    __PMPFILEHANDLER *pHdlr;

    pHdlr = (__PMPFILEHANDLER*) malloc(sizeof(__PMPFILEHANDLER));
    if (!pHdlr)
        return PMP_PARSER_E_MEMORY;

    memset(pHdlr, 0, sizeof(__PMPFILEHANDLER));


    memset(&pHdlr->vFormat, 0, sizeof(cedarv_stream_info_t));
#if 0
    memset(&pHdlr->aFormat, 0, sizeof(struct AUDIO_CODEC_FORMAT));
#endif

    *ppHdlr = pHdlr;
    return PMP_PARSER_OK;
}


/*
*********************************************************************************************************
*                               pmp_destroy
*
*Description: release resource;
*
*Arguments  : ppHdlr    - the address where the pmp file handler is stored;
*
*Return     : PMP_PARSER_OK         - successed;
*             PMP_PARSER_E_POINTER  - either ppHdlr or *ppHdlr is a null pointer;
*
*Summary    : ;
*
*Author     : XC
*
*********************************************************************************************************
*/
s32 pmp_destroy(__PMPFILEHANDLER **ppHdlr)
{
    __PMPFILEHANDLER *p;

    if (!ppHdlr || !(*ppHdlr))
        return PMP_PARSER_E_POINTER;

    p = *((__PMPFILEHANDLER**)ppHdlr);

    pmp_idxtab_destroy(p);
    pmp_packet_destroy(p);

    if (p->fpMediaFile)
    {
        fclose(p->fpMediaFile);
        p->fpMediaFile = NULL;
    }

    free(p);
    *ppHdlr = 0;
    return PMP_PARSER_OK;
}


/*
*********************************************************************************************************
*                               pmp_set_input_media_file
*
*Description: attach a media file to pmp file handler;
*
*Arguments  : pHdlr    - pmp file handler;
*             fp       - media file pointer;
*
*Return     : PMP_PARSER_OK         - successed;
*             PMP_PARSER_E_POINTER  - pHdlr is a null pointer;
*             PMP_PARSER_IO_ERR     - read file error;
*             PMP_PARSER_E_FORMAT   - file content error;
*             PMP_PARSER_E_MEMORY   - malloc memory fail;
*
*Summary    : when the media file is specified to pmp file handler, outside
*             should never operate the media file, it should better set fp to 0;
*
*Author     : XC
*
*********************************************************************************************************
*/
s32 pmp_set_input_media_file(__PMPFILEHANDLER *pHdlr, FILE *fp)
{
    s32 retVal;

    if (!pHdlr || !fp)
        return PMP_PARSER_E_POINTER;

    pHdlr->fpMediaFile = fp;

    retVal = pmp_read_headers(pHdlr);
    if(retVal != PMP_PARSER_OK)
        return retVal;

    pmp_set_media_format_info(pHdlr);
    return PMP_PARSER_OK;
}


/*
*********************************************************************************************************
*                               pmp_get_chunk_info
*
*Description: get media type of the media data ready to send;
*
*Arguments  : pHdlr      - pmp file handler;
*             pChunkType - address to store media type;
*
*Return     : PMP_PARSER_OK         - successed;
*             PMP_PARSER_E_POINTER  - either pHdlr or pChunkType is a null pointer;
*             PMP_PARSER_IO_ERR     - read file error;
*             PMP_PARSER_E_FORMAT   - file content error;
*             PMP_PARSER_END        - all data is parsed in media file;
*
*Summary    : ;
*
*Author     : XC
*
*********************************************************************************************************
*/
s32 pmp_get_chunk_info(__PMPFILEHANDLER *pHdlr, s32 *pChunkType, s32* pPktSize)
{
    __PMPPACKET*	pkt;
    u32           	byteRead;
    u8*				tmpBuf;
    u32           	audFrmNum;
    u32           	i,j;

    if (!pHdlr)
        return PMP_PARSER_E_POINTER;

    pkt     = &pHdlr->pkt;
    tmpBuf  = pHdlr->pkt.tmpBuf;

    if (pkt->bVideoParsed == 0)
    {
        //* Some video data left since last pmp_get_chunk_data() call,
        //* so we continue to send video data.
        *pChunkType = VIDEO_PACKET_TYPE;
        *pPktSize   = pkt->vdFrmSize;
        return PMP_PARSER_OK;
    }
    else if (pkt->bAudioParsed == 0)
    {
#if BISTREAM_PLAY_AUDIO
        //* Audio data is not read yet, or there is some audio data
        //* left since last pmp_get_chunk_data() call.
        if(pHdlr->bDiscardAudio == 1)
        {
            //* Not play audio, seek over audio data.
            u32 len = 0;
            for (i=0; i<pHdlr->auStrmNum; i++)
            {
                for (j = 0; j<pkt->numAudioFrms; j++)
                {
                    len += (pkt->auSize[i][j]+7) ;
                }
            }
            fseek(pHdlr->fpMediaFile, len, SEEK_CUR);
            pkt->bAudioParsed = 1;
        }
        else
        {
        	u32 i;

            *pChunkType = AUDIO_PACKET_TYPE;
        	*pPktSize = 0;
        	for(i=0; i<pkt->numAudioFrms; i++)
        	{
        		(*pPktSize) += (pkt->auSize[pHdlr->auId][i]+7);
        	}

            return PMP_PARSER_OK;
        }
#else
        //* Not play audio, seek over audio data.
        u32 len = 0;
        for (i=0; i<pHdlr->auStrmNum; i++)
        {
            for (j = 0; j<pkt->numAudioFrms; j++)
            {
                len += (pkt->auSize[i][j] + 7);
            }
        }
        fseek(pHdlr->fpMediaFile, len, SEEK_CUR);
        pkt->bAudioParsed = 1;
#endif
    }

    if (pHdlr->pktCounter >= pHdlr->vdFrmNum)
        return PMP_PARSER_END;

    //* Parse a new packet, read 13 bytes of packet header.
    byteRead = fread(tmpBuf, 1, 13, pHdlr->fpMediaFile);
    if (byteRead != 13)
        return PMP_PARSER_IO_ERR;

    pkt->numAudioFrms = tmpBuf[0]; //* how many audio frames in this packet
    pkt->firstDelay   = tmpBuf[1] | tmpBuf[2]<<8 | tmpBuf[3]<<16 | tmpBuf[4]<<24;
    pkt->lastDelay    = tmpBuf[5] | tmpBuf[6]<<8 | tmpBuf[7]<<16 | tmpBuf[8]<<24;
    pkt->vdFrmSize    = tmpBuf[9] | tmpBuf[10]<<8 | tmpBuf[11]<<16 | tmpBuf[12]<<24;

    //* Calculate video and audio pts in millisecond.
    pkt->vdPts = pHdlr->pktCounter * (u64)pHdlr->vdFrmInterval;
    pkt->auPts = pkt->vdPts + pkt->firstDelay;

    pHdlr->pktCounter++;

    audFrmNum = pkt->numAudioFrms;
    if(audFrmNum > pHdlr->maxAuPerFrame)
        return PMP_PARSER_E_FORMAT;

    //* Get audio frame size, for every audio frame of every audio stream,
    //* there is a dword to tell the frame size.
    byteRead = fread(tmpBuf, 1, audFrmNum * 4 * pHdlr->auStrmNum, pHdlr->fpMediaFile);
    if (byteRead != pkt->numAudioFrms*4)
        return PMP_PARSER_IO_ERR;

    for (j=0; j<pHdlr->auStrmNum; j++)
    {
        for (i=0; i<audFrmNum; i++)
        {
            //* the frame size of the ith frame of the jth audio stream.
            pkt->auSize[j][i] = (tmpBuf[4*i] | tmpBuf[4*i+1]<<8 | tmpBuf[4*i+2]<<16 | tmpBuf[4*i+3]<<24);
        }
    }

    pkt->bAudioParsed = pkt->bVideoParsed = 0;
    if(pkt->vdFrmSize == 0)
        pkt->bVideoParsed = 1;
    if (pkt->numAudioFrms == 0)
        pkt->bAudioParsed = 1;

    *pChunkType = VIDEO_PACKET_TYPE;
    *pPktSize   = pkt->vdFrmSize;

    return PMP_PARSER_OK;
}


/*
*********************************************************************************************************
*                               pmp_get_chunk_data
*
*Description: get media data;
*
*Arguments  : pHdlr        - pmp file handler;
*             pBuf         - buffer to store media data;
*             nBufSize     - size of buffer;
*             pDataSize    - address to store the size of media data;
*             pInformation - address to store data attribute information;
*
*Return     : PMP_PARSER_OK         - successed;
*             PMP_PARSER_E_POINTER  - pHdlr, pBuf, pDataSize or pInformation
                                      is a null pointer;
*             PMP_PARSER_IO_ERR     - read file error;
*             PMP_PARSER_FAIL       - fail;
*
*Summary    : the pInformation store data attribute information bits, to see
*             what these bits mean, refer to the information bits definition
*             at 'pmp.h';
*
*Author     : XC
*
*********************************************************************************************************
*/
s32 pmp_get_chunk_data(__PMPFILEHANDLER *pHdlr, u8 *pBuf, u32 nBufSize, u32 *pDataSize, u32 *pInformation)
{
    s32       		retVal;
    u32       		bytesRead;
    __PMPPACKET*	pkt;

    if (!pHdlr || !pBuf || !pInformation || !pDataSize)
        return PMP_PARSER_E_POINTER;

    pkt             = &pHdlr->pkt;
    *pInformation   = 0;

    if (pkt->bVideoParsed == 0)
    {
        //* Parse Video Data
        *pInformation |= PMP_PARSER_IS_VEDEO_DATA;

        if (pkt->leftDataSize == 0)
            *pInformation |= PMP_PARSER_IS_FIRST_PART;
        else
        {
            //* we have some video data left when this function was called last time.
            if (pkt->leftDataSize <= nBufSize)
            {
                bytesRead = fread(pBuf, 1, pkt->leftDataSize, pHdlr->fpMediaFile);
                if (bytesRead != pkt->leftDataSize)
                    return PMP_PARSER_IO_ERR;

                *pDataSize          = bytesRead;
                pkt->bVideoParsed   = 1;
                *pInformation       |= PMP_PARSER_IS_LAST_PART;
            }
            else
            {
                //* the buffer size is not big enough, read part of the left data.
                bytesRead = fread(pBuf, 1, nBufSize, pHdlr->fpMediaFile);
                if (bytesRead != nBufSize)
                    return PMP_PARSER_IO_ERR;

                *pDataSize = bytesRead;
                *pInformation |= PMP_PARSER_DATA_NOT_FINISHED;
            }

            pkt->leftDataSize -= bytesRead;
            return PMP_PARSER_OK;

        }

        if (pkt->vdFrmSize <= nBufSize)
        {
            bytesRead = fread(pBuf, 1, pkt->vdFrmSize, pHdlr->fpMediaFile);
            if (bytesRead != pkt->vdFrmSize)
                return PMP_PARSER_IO_ERR;

            *pDataSize          = bytesRead;
            pkt->bVideoParsed   = 1;
            *pInformation       |= PMP_PARSER_IS_LAST_PART;
        }
        else
        {
            //* the buffer size is not big enough, read part of the video data.
            bytesRead = fread(pBuf, 1, nBufSize, pHdlr->fpMediaFile);
            if (bytesRead != nBufSize)
                return PMP_PARSER_IO_ERR;

            *pDataSize = bytesRead;
            *pInformation |= PMP_PARSER_DATA_NOT_FINISHED;
            pkt->leftDataSize = pkt->vdFrmSize - nBufSize;
        }

        return PMP_PARSER_OK;
    }
    else if (pkt->bAudioParsed == 0)
    {
        //* Parse Audio Data
        u32 len;
        u32 i;
        u32 j;
        u8* ptr;
        u32 dataSize;
        u32 fillSize = 0;
        u32 leftBufSize;

       *pInformation |= PMP_PARSER_IS_AUDIO_DATA;

        //* check if we have some data left from last get chunk process
        if (pkt->leftAuFrm > 0 || pkt->leftDataSize > 0 || pkt->leftAACHdrBytes > 0)
        {
            if (pHdlr->auCodecID == 1)
            {
                //* AAC Audio Stream
                ptr         = pBuf;
                leftBufSize = nBufSize;

                dataSize = 0;
                if (pkt->leftAACHdrBytes > 0)
                {
                    if (leftBufSize > pkt->leftAACHdrBytes)
                    {
                        for (i = 0; i < pkt->leftAACHdrBytes; i++)
                            *ptr++ = pkt->aac_hdr[i + (7-pkt->leftAACHdrBytes)];

                        dataSize            += pkt->leftAACHdrBytes;
                        leftBufSize         -= pkt->leftDataSize;
                        pkt->leftAACHdrBytes = 0;
                    }
                    else
                    {
                        for (i=0; i<nBufSize; i++)
                            *ptr++ = pkt->aac_hdr[i + (7-pkt->leftAACHdrBytes)];

                        dataSize             += nBufSize;
                        pkt->leftAACHdrBytes -= nBufSize;
                        *pDataSize           =  dataSize;
                        *pInformation        |= PMP_PARSER_DATA_NOT_FINISHED;
                        return PMP_PARSER_OK;
                    }
                }

                if (pkt->leftDataSize > 0)
                {
                    if (leftBufSize > pkt->leftDataSize)
                    {
                        bytesRead = fread(ptr, 1, pkt->leftDataSize, pHdlr->fpMediaFile);
                        if (bytesRead != pkt->leftDataSize)
                        {
                            return PMP_PARSER_IO_ERR;
                        }

                        dataSize            += pkt->leftDataSize;
                        leftBufSize         -= pkt->leftDataSize;
                        ptr                 += pkt->leftDataSize;
                        pkt->leftDataSize   =  0;
                    }
                    else
                    {
                        bytesRead = fread(ptr, 1, leftBufSize, pHdlr->fpMediaFile);
                        if (bytesRead != leftBufSize)
                        {
                            return PMP_PARSER_IO_ERR;
                        }

                        dataSize += leftBufSize;
                        pkt->leftDataSize -= leftBufSize;
                        *pInformation |= PMP_PARSER_DATA_NOT_FINISHED;
                        return PMP_PARSER_OK;
                    }
                }

                pkt->leftAuFrm--;
                goto aac_frame_loop;
            }
            else
            {
                //* MP3 Audio Stream
                fillSize = (nBufSize > pkt->leftDataSize) ? pkt->leftDataSize : nBufSize;
                pkt->leftDataSize -= fillSize;

                dataSize = fread(pBuf, 1, fillSize, pHdlr->fpMediaFile);
                if (dataSize != fillSize)
                {
                    return PMP_PARSER_IO_ERR;
                }

                *pDataSize = fillSize;
                goto audio_parse_finished;
            }
        }

        *pInformation |= PMP_PARSER_IS_FIRST_PART;

        //* skip data of other audio streams
        if (pHdlr->auId > 0)
        {
            len = 0;
            for (i=0; i<pHdlr->auId; i++)
                for (j=0; j<pkt->numAudioFrms; j++)
                    len += pkt->auSize[i][j];

            retVal = fseek(pHdlr->fpMediaFile, len, SEEK_CUR);
            if (retVal != 0)
            {
                printf("seek failed when get audio chunk.\n");
                return PMP_PARSER_IO_ERR;
            }
        }

        if (pHdlr->auCodecID == 1)
        {
            //* AAC audio stream
            ptr         = pBuf;
            fillSize    = 0;
            dataSize    = 0;
            leftBufSize = nBufSize;
            pkt->leftAuFrm = pkt->numAudioFrms;

aac_frame_loop:
            //* parse the audio data of selected stream(p->auId).
            for (; pkt->leftAuFrm > 0; pkt->leftAuFrm--)
            {
                len = pkt->auSize[pHdlr->auId][pkt->numAudioFrms - pkt->leftAuFrm];

	            pkt->aac_hdr[3] = (pkt->aac_hdr[3]&(~0x3)) | ((len+7)>>(13-2) & 0x3);

	            pkt->aac_hdr[4] = ((len+7)>>3) & 0xff;

	            pkt->aac_hdr[5] = ((len+7)&0x07)<<(8-3) | 0x1f;
                if (leftBufSize >= (len + 7))
                {
                    //* read one audio frame
                    *ptr++ = pkt->aac_hdr[0];
                    *ptr++ = pkt->aac_hdr[1];
                    *ptr++ = pkt->aac_hdr[2];
                    *ptr++ = pkt->aac_hdr[3];
                    *ptr++ = pkt->aac_hdr[4];
                    *ptr++ = pkt->aac_hdr[5];
                    *ptr++ = pkt->aac_hdr[6];
                    bytesRead = fread(ptr, 1, len, pHdlr->fpMediaFile);
                    if (bytesRead != len)
                    {
                        return PMP_PARSER_IO_ERR;
                    }

                    ptr         += len;
                    leftBufSize -= (len+7);
                    dataSize    += (len+7);
                }
                else
                {
                    //* buffer not enough to fill one audio frame.
                    if (leftBufSize > 7)
                    {
                        *ptr++ = pkt->aac_hdr[0];
                        *ptr++ = pkt->aac_hdr[1];
                        *ptr++ = pkt->aac_hdr[2];
                        *ptr++ = pkt->aac_hdr[3];
                        *ptr++ = pkt->aac_hdr[4];
                        *ptr++ = pkt->aac_hdr[5];
                        *ptr++ = pkt->aac_hdr[6];

                        bytesRead = fread(ptr, 1, (leftBufSize - 7), pHdlr->fpMediaFile);
                        if (bytesRead != (leftBufSize - 7))
                        {
                            return PMP_PARSER_IO_ERR;
                        }
                        pkt->leftDataSize = len + 7 - leftBufSize;

                        dataSize += leftBufSize;
                    }
                    else
                    {
                        for (i=0; i<leftBufSize; i++)
                            *ptr++ = pkt->aac_hdr[i];

                        pkt->leftAACHdrBytes = 7 - leftBufSize;
                        pkt->leftDataSize = len;
                        dataSize += leftBufSize;
                    }

                    break;
                }
            }

            *pDataSize = dataSize;
        }
        else
        {
            //* MP3 audio stream
            //* parse the audio data of selected stream(p->auId).
            len = 0;
            for (pkt->leftAuFrm = pkt->numAudioFrms; pkt->leftAuFrm > 0; pkt->leftAuFrm--)
                len += pkt->auSize[pHdlr->auId][pkt->numAudioFrms - pkt->leftAuFrm];

            if (nBufSize >= len)
                fillSize = (nBufSize > len) ? len : nBufSize;

            pkt->leftDataSize = len - fillSize;
            dataSize = fread(pBuf, 1, fillSize, pHdlr->fpMediaFile);
            if (dataSize != fillSize)
            {
                return PMP_PARSER_IO_ERR;
            }

            *pDataSize = fillSize;
        }

audio_parse_finished:
        if (pkt->leftDataSize>0 || pkt->leftAuFrm>0)
        {
            //* the audio data of this frame is not finished.
            *pInformation |= PMP_PARSER_DATA_NOT_FINISHED;
            printf("left data = %d, left frame = %d", pkt->leftDataSize, pkt->leftAuFrm);
        }
        else
        {
            len = 0;

            if ((pHdlr->auId+1) < pHdlr->auStrmNum)
            {
                //* skip data of other audio streams
                for (i=pHdlr->auId+1; i<pHdlr->auStrmNum; i++)
                    for (j=0; j<pkt->numAudioFrms; j++)
                        len += pkt->auSize[i][j];

                fseek(pHdlr->fpMediaFile, len, SEEK_CUR);
            }

            *pInformation |= PMP_PARSER_IS_LAST_PART;
            pkt->bAudioParsed = 1;
        }

        return PMP_PARSER_OK;
    }
    else
    {
        //* pmp_parser_get_chunk_info() is not called before this function,
        //* otherwise, there must be some bug in this code.
        return PMP_PARSER_FAIL;
    }
}


s32 pmp_skip_chunk_data(__PMPFILEHANDLER *pHdlr)
{
    __PMPPACKET*	pkt;

    if (!pHdlr)
        return PMP_PARSER_E_POINTER;

    pkt = &pHdlr->pkt;

    if (pkt->bVideoParsed == 0)
    {
        if (pkt->leftDataSize == 0)
        {
        	fseek(pHdlr->fpMediaFile, pkt->vdFrmSize, SEEK_CUR);
        	pkt->bVideoParsed = 1;
        }
        else
        {
            //* we have some video data left when this function was called last time.
            fseek(pHdlr->fpMediaFile, pkt->leftDataSize, SEEK_CUR);
            pkt->bVideoParsed = 1;
            pkt->leftDataSize = 0;
        }

        return PMP_PARSER_OK;
    }
    else if (pkt->bAudioParsed == 0)
    {
        //* Parse Audio Data
        u32 len;
        u32 i;
        u32 j;
        u32 fillSize = 0;

        //* skip data of other audio streams
        if (pHdlr->auId > 0)
        {
            len = 0;
            for (i=0; i<pHdlr->auId; i++)
                for (j=0; j<pkt->numAudioFrms; j++)
                    len += pkt->auSize[i][j];

            fseek(pHdlr->fpMediaFile, len, SEEK_CUR);
        }

        if (pHdlr->auCodecID == 1)
        {
            //* AAC audio stream
            //* parse the audio data of selected stream(p->auId).
            for (pkt->leftAuFrm = pkt->numAudioFrms; pkt->leftAuFrm > 0; pkt->leftAuFrm--)
            {
                len = pkt->auSize[pHdlr->auId][pkt->numAudioFrms - pkt->leftAuFrm];
                fseek(pHdlr->fpMediaFile, len, SEEK_CUR);
            }

            pkt->bAudioParsed = 1;
        }
        else
        {
            //* MP3 audio stream
            //* parse the audio data of selected stream(p->auId).
            len = 0;
            for (pkt->leftAuFrm = pkt->numAudioFrms; pkt->leftAuFrm > 0; pkt->leftAuFrm--)
                len += pkt->auSize[pHdlr->auId][pkt->numAudioFrms - pkt->leftAuFrm];

            fillSize = len;
            fseek(pHdlr->fpMediaFile, fillSize, SEEK_CUR);

            pkt->bAudioParsed = 1;
        }

        return PMP_PARSER_OK;
    }
    else
    {
        //* pmp_parser_get_chunk_info() is not called before this function,
        //* otherwise, there must be some bug in this code.
        return PMP_PARSER_FAIL;
    }
}


/*
*********************************************************************************************************
*                               pmp_get_pts
*
*Description: get audio or video pts of current packet;
*
*Arguments  : pHdlr  - pmp file handler;
*             bAudio - get audio pts;
*
*Return     : video or audio pts in millisecond;
*
*Summary    : if pHdlr is a null pointer, it return 0;
*
*Author     : XC
*
*********************************************************************************************************
*/
u64 pmp_get_pts(__PMPFILEHANDLER *pHdlr, u8 bAudio)
{
    if(!pHdlr)
        return 0;

    if(bAudio)
        return pHdlr->pkt.auPts;
    else
        return pHdlr->pkt.vdPts;
}


/*
*********************************************************************************************************
*                               pmp_get_audio_codec_id
*
*Description: get audio codec id;
*
*Arguments  : pHdlr  - pmp file handler;
*
*Return     : audio codec id;
*
*Summary    : pmp file can contain either MP3 or AAC audio stream;
*
*Author     : XC
*
*********************************************************************************************************
*/
#if 0
s32 pmp_get_audio_codec_id(__PMPFILEHANDLER *pHdlr)
{
    if(!pHdlr)
        return CDX_AUDIO_UNKNOWN;

    if(pHdlr->auCodecID == 0)
        return CDX_AUDIO_MP3;
    else if(pHdlr->auCodecID == 1)
        return CDX_AUDIO_MPEG_AAC_LC;
    else
        return CDX_AUDIO_UNKNOWN;
}
#endif


/*
*********************************************************************************************************
*                               pmp_get_video_codec_id
*
*Description: get video codec id;
*
*Arguments  : pHdlr  - pmp file handler;
*
*Return     : video codec id;
*
*Summary    : pmp file can contain either MP4 or H264 audio stream;
*
*Author     : XC
*
*********************************************************************************************************
*/
s32 pmp_get_video_codec_id(__PMPFILEHANDLER *pHdlr)
{
    if(!pHdlr)
        return CEDARV_STREAM_FORMAT_UNKNOW;

    if(pHdlr->vdCodecID == 0)
        return CEDARV_STREAM_FORMAT_MPEG4;
    else if(pHdlr->vdCodecID == 1)
        return CEDARV_STREAM_FORMAT_H264;
    else
        return CEDARV_STREAM_FORMAT_UNKNOW;
}


/*
*********************************************************************************************************
*                               pmp_get_audio_format
*
*Description: get audio format information;
*
*Arguments  : pHdlr  - pmp file handler;
*             ppAfmt - address to store a pointer to a __audio_codec_format_t
*                      struct;
*
*Return     : PMP_PARSER_OK         - successed;
*             PMP_PARSER_E_POINTER  - either pHdlr or ppAfmt is a null pointer;
*
*Summary    : ;
*
*Author     : XC
*
*********************************************************************************************************
*/
#if 0
s32 pmp_get_audio_format(__PMPFILEHANDLER *pHdlr, CedarXAudioStreamInfo **ppAfmt)
{
    if(!pHdlr || !ppAfmt)
        return PMP_PARSER_E_POINTER;

    *ppAfmt = &pHdlr->aFormat;
    return PMP_PARSER_OK;
}
#endif

/*
*********************************************************************************************************
*                               pmp_get_video_format
*
*Description: get video format information;
*
*Arguments  : pHdlr  - pmp file handler;
*             ppVfmt - address to store a pointer to a __video_codec_format_t
*                      struct;
*
*Return     : PMP_PARSER_OK         - successed;
*             PMP_PARSER_E_POINTER  - either pHdlr or ppVfmt is a null pointer;
*
*Summary    : ;
*
*Author     : XC
*
*********************************************************************************************************
*/
s32 pmp_get_video_format(__PMPFILEHANDLER *pHdlr, cedarv_stream_info_t **ppVfmt)
{
    if(!pHdlr || !ppVfmt)
        return PMP_PARSER_E_POINTER;

    *ppVfmt = &pHdlr->vFormat;
    return PMP_PARSER_OK;
}


/*
*********************************************************************************************************
*                               pmp_seek_to_key_frame
*
*Description: seek media file to a key frame near the time specified;
*
*Arguments  : pHdlr     - pmp file handler;
*             time      - media time to seek to, it is in microsecond;
*             bBackward - whether the key frame' pts is before or after
*                         time;
*
*Return     : PMP_PARSER_OK     - successed;
*             PMP_PARSER_FAIL   - index exceed of pmp file handler has no index
*                                 table;
*             PMP_PARSER_IO_ERR - cdx_seek error;
*
*Summary    : this function seek media file to a key frame near the time
*             specified;
*
*Author     : XC
*
*********************************************************************************************************
*/
s32 pmp_seek_to_key_frame(__PMPFILEHANDLER* pHdlr, u32 time, u32 bBackward)
{
    s32    retVal;
    u32    frmId;
    u32    i;
    u32    pos;

    if(!pHdlr || !pHdlr->keyIdxNum)
        return PMP_PARSER_FAIL;

    frmId = (u32)(((u64)time*1000)/pHdlr->vdFrmInterval);

    for(i = 1; i<pHdlr->keyIdxNum; i++)
    {
        if(pHdlr->pKeyFrmIdx[i].frmID>frmId)
            break;
    }

    i -= bBackward;
    if(i > pHdlr->keyIdxNum)
        return PMP_PARSER_FAIL;

    pos    = pHdlr->pKeyFrmIdx[i].filePos;
    retVal = fseek(pHdlr->fpMediaFile, pos, SEEK_SET);
    if(retVal != 0)
        return PMP_PARSER_IO_ERR;

    pHdlr->pktCounter           = pHdlr->pKeyFrmIdx[i].frmID;
    pHdlr->pkt.bAudioParsed     = 1;
    pHdlr->pkt.bVideoParsed     = 1;
    pHdlr->pkt.leftDataSize     = 0;
    pHdlr->pkt.leftAACHdrBytes  = 0;
    pHdlr->pkt.leftAuFrm        = 0;

    return PMP_PARSER_OK;
}


/*
*********************************************************************************************************
*                               pmp_seek_to_next_key_frame
*
*Description: seek media file to a key frame, the pts difference between the
*             key frame and current packet specified by timeInterval;
*
*Arguments  : pHdlr         - pmp file handler;
*             timeInterval  - pts difference between the key frame and current
*                             packet;
*             bBackward     - seek backward or forward;
*
*Return     : PMP_PARSER_OK     - successed;
*             PMP_PARSER_FAIL   - index exceed of pmp file handler has no index
*                                 table;
*             PMP_PARSER_IO_ERR - cdx_seek error;
*
*Summary    : if you want to seek to next key frame nearby, just set
*             timeInterval to 0;
*
*Author     : XC
*
*********************************************************************************************************
*/
s32 pmp_seek_to_next_key_frame(__PMPFILEHANDLER* pHdlr, u32 timeInterval, u32 bBackward)
{
    s32    retVal;
    s32    frmId;
    u32    frmDelta;
    u32    i;
    u32    pos;

    if(!pHdlr || !pHdlr->keyIdxNum)
        return PMP_PARSER_FAIL;

    if(bBackward)
    {
        if(pHdlr->pktCounter==0)
            return PMP_PARSER_FAIL;
    }
    else
    {
        if(pHdlr->pktCounter >= pHdlr->pKeyFrmIdx[pHdlr->keyIdxNum-1].frmID)
            return PMP_PARSER_FAIL;
    }

    frmId    = pHdlr->pktCounter;
    frmDelta = (u32)(((u64)timeInterval*1000)/pHdlr->vdFrmInterval);

    if(bBackward)
        frmId -= frmDelta;
    else
        frmId += frmDelta;

    if(frmId < 0)
        frmId = 0;
    else if(frmId > (s32)pHdlr->pKeyFrmIdx[pHdlr->keyIdxNum-1].frmID)
        frmId = pHdlr->pKeyFrmIdx[pHdlr->keyIdxNum-1].frmID;

    for(i = 1; i<pHdlr->keyIdxNum; i++)
    {
        if((s32)pHdlr->pKeyFrmIdx[i].frmID > frmId)
            break;
    }

    i -= bBackward;
    if(i > pHdlr->keyIdxNum)
        return PMP_PARSER_FAIL;

    pos    = pHdlr->pKeyFrmIdx[i].filePos;
    retVal = fseek(pHdlr->fpMediaFile, pos, SEEK_SET);
    if(retVal != 0)
        return PMP_PARSER_IO_ERR;

    pHdlr->pktCounter           = pHdlr->pKeyFrmIdx[i].frmID;
    pHdlr->pkt.bAudioParsed     = 1;
    pHdlr->pkt.bVideoParsed     = 1;
    pHdlr->pkt.leftDataSize     = 0;
    pHdlr->pkt.leftAACHdrBytes  = 0;
    pHdlr->pkt.leftAuFrm        = 0;

    return PMP_PARSER_OK;
}


/*
*********************************************************************************************************
*                               pmp_has_index_table
*
*Description: check if the pmp file handler has key frame index table;
*
*Arguments  : pHdlr         - pmp file handler;
*
*Return     : PMP_PARSER_TRUE  - hax key frame index table;
*             PMP_PARSER_FALSE - no index table;
*
*Summary    : ;
*
*Author     : XC
*
*********************************************************************************************************
*/
s32 pmp_has_index_table(__PMPFILEHANDLER* pHdlr)
{
    if(pHdlr && (pHdlr->keyIdxNum))
        return PMP_PARSER_TRUE;

    return PMP_PARSER_FALSE;
}


/*
*********************************************************************************************************
*                               pmp_get_file_duration
*
*Description: get the file duration;
*
*Arguments  : pHdlr         - pmp file handler;
*
*Return     : video duration in microsecond;
*
*Summary    : ;
*
*Author     : XC
*
*********************************************************************************************************
*/
u32 pmp_get_file_duration(__PMPFILEHANDLER* pHdlr)
{
    u64 tmp;
    if(pHdlr && (pHdlr->vdRate>0))
    {
        tmp = pHdlr->vdFrmNum;
        tmp = tmp * pHdlr->vdScale * 1000;
        tmp = tmp/pHdlr->vdRate;

        return (u32)tmp;
    }

    return 0;
}

/*
*********************************************************************************************************
*                               pmp_set_audio_discard_flag
*
*Description: set a flag to deside whether to discard audio data, it is useful when decoder don't support
*             the audio format;
*
*Arguments  : pHdlr         - pmp file handler;
*             bDiscardAudio - flag to deside whether to discard data;
*
*Return     : 0;
*
*Summary    : ;
*
*Author     : Clif.C
*
*********************************************************************************************************
*/
s32 pmp_set_audio_discard_flag(__PMPFILEHANDLER* pHdlr, u32 bDiscardAudio)
{
    if(pHdlr)
    {
        pHdlr->bDiscardAudio = bDiscardAudio;
    }

    return 0;
}


/************************************************************************/
/*                   static function definitons                         */
/************************************************************************/

/*
*********************************************************************************************************
*                               pmp_read_headers
*
*Description: read pmp file header and request resource;
*
*Arguments  : pHdlr    - pmp file handler;
*
*Return     : PMP_PARSER_OK         - successed;
*             PMP_PARSER_E_POINTER  - pHdlris a null pointer;
*             PMP_PARSER_IO_ERR     - read file error;
*             PMP_PARSER_E_FORMAT   - not support format;
*             PMP_PARSER_E_MEMORY   - malloc memory fail;
*
*Summary    : this function read pmp file header and chech it is valid;
*
*Author     : XC
*
*********************************************************************************************************
*/
static s32 pmp_read_headers(__PMPFILEHANDLER *pHdlr)
{
    s32  retVal;
    u32  tmp;
    s32  byteRead;
    u32  readBuf[52];
    u32* bufPtr;

    if (!pHdlr)
        return PMP_PARSER_E_POINTER;

    if (!pHdlr->fpMediaFile)
        return PMP_PARSER_IO_ERR;

    //* read 4 bytes to check whether it is a pmp file,
    //* the first 4 bytes must be "pmpm"
    byteRead = fread((void*) &tmp, 1, 4, pHdlr->fpMediaFile);
    if (byteRead != 4)
        return PMP_PARSER_IO_ERR;

    if (tmp != PMPFILETAG)
        return PMP_PARSER_E_FORMAT;

    memset(readBuf, 0, sizeof(readBuf));
    byteRead = fread(readBuf, 1, PMP_FILE_HEADER_SIZE, pHdlr->fpMediaFile);
    if (byteRead != PMP_FILE_HEADER_SIZE)
        return PMP_PARSER_IO_ERR;

    bufPtr = readBuf;
    pHdlr->pmpVesion     = *bufPtr++;
    pHdlr->vdCodecID     = *bufPtr++;
    pHdlr->vdFrmNum      = *bufPtr++;
    pHdlr->vdWidth       = *bufPtr++;
    pHdlr->vdHeight      = *bufPtr++;
    pHdlr->vdScale       = *bufPtr++;
    pHdlr->vdRate        = *bufPtr++;
    pHdlr->auCodecID     = *bufPtr++;
    pHdlr->auStrmNum     = *bufPtr++;
    pHdlr->maxAuPerFrame = *bufPtr++;
    pHdlr->auScale       = *bufPtr++;
    pHdlr->auRate        = *bufPtr++;
    pHdlr->bStereo       = *bufPtr++;

    if (pmp_check_header(pHdlr) != PMP_PARSER_OK)
        return PMP_PARSER_E_FORMAT;

    pHdlr->vdFrmInterval = (u32)((u64)pHdlr->vdScale*1000*1000/pHdlr->vdRate);

    if (pmp_packet_create(pHdlr) != PMP_PARSER_OK)
        return PMP_PARSER_E_MEMORY;

    retVal = pmp_idxtab_build(pHdlr);
    if (retVal != PMP_PARSER_OK)
        return PMP_PARSER_E_FORMAT;

    return PMP_PARSER_OK;
}


/*
*********************************************************************************************************
*                               pmp_check_header
*
*Description: check pmp file header valid;
*
*Arguments  : pHdlr    - pmp file handler;
*
*Return     : PMP_PARSER_OK         - successed;
*             PMP_PARSER_E_POINTER  - pHdlris a null pointer;
*             PMP_PARSER_E_FORMAT   - not support format;
*
*Summary    : ;
*
*Author     : XC
*
*********************************************************************************************************
*/
static s32 pmp_check_header(__PMPFILEHANDLER *pHdlr)
{
    if (!pHdlr)
        return PMP_PARSER_E_POINTER;

    if (pHdlr->vdScale == 0 || pHdlr->vdRate == 0)
    {
        printf("video frame rate error.\n");
        pHdlr->vdScale = 1000;
        pHdlr->vdRate = 30000;
    }

    if (pHdlr->pmpVesion != PMP_VERSION_20)
    {
    	printf("pmp file version is not 2.0.\n");
        return PMP_PARSER_E_FORMAT;
    }

    if (pHdlr->vdCodecID > 1)
    {
    	printf("unknown video codec id.\n");
        return PMP_PARSER_E_FORMAT;
    }

    if (pHdlr->vdFrmNum == 0)
    {
    	printf("total frame number is zero.\n");
        return PMP_PARSER_E_FORMAT;
    }

    if (pHdlr->vdWidth == 0 || pHdlr->vdHeight == 0)
    {
    	printf("video width and height error.\n");
    }

    if (pHdlr->auCodecID > 1)
    {
    	printf("unknow audio format.\n");
        return PMP_PARSER_E_FORMAT;
    }

    return PMP_PARSER_OK;
}


/*
*********************************************************************************************************
*                               pmp_packet_create
*
*Description: create pmp packet to store packet header;
*
*Arguments  : pHdlr    - pmp file handler;
*
*Return     : PMP_PARSER_OK         - successed;
*             PMP_PARSER_E_MEMORY   - malloc memory fail;
*
*Summary    : if audio is aac, this function initializes aac header;
*
*Author     : XC
*
*********************************************************************************************************
*/
static s32 pmp_packet_create(__PMPFILEHANDLER *pHdlr)
{
    __PMPPACKET* pkt;
    u8*			 tmpBuf;
    u32**		 tmpAuSize;
    u32*		 tmpAuFrameSize;
    u32          i;

    pkt = &pHdlr->pkt;

    tmpBuf = (u8*) malloc(pHdlr->auStrmNum * pHdlr->maxAuPerFrame * 4 + 13);
    if (!tmpBuf)
        return PMP_PARSER_E_MEMORY;

    tmpAuSize = (u32**) malloc(pHdlr->auStrmNum * sizeof(u32*));
    if (!tmpAuSize)
    {
        free(tmpBuf);
        return PMP_PARSER_E_MEMORY;
    }

    tmpAuFrameSize = (u32*) malloc(pHdlr->auStrmNum * pHdlr->maxAuPerFrame * sizeof(u32));
    if (!tmpAuFrameSize)
    {
        free(tmpBuf);
        free(tmpAuSize);
        return PMP_PARSER_E_MEMORY;
    }

    if (pkt->tmpBuf)
        free(pkt->tmpBuf);

    if (pkt->auSize)
    {
        free(pkt->auSize[0]);
        free(pkt->auSize);
        pkt->auSize = 0;
    }

    pkt->tmpBuf = tmpBuf;
    pkt->auSize = tmpAuSize;

    for (i=0; i<pHdlr->auStrmNum; i++)
    {
        pkt->auSize[i] = tmpAuFrameSize + i* pHdlr->maxAuPerFrame*sizeof(u32);
    }

    if(pHdlr->auCodecID == 1)
	{
	    u16 sampRateIdx_4bits             = 0;
	    u16 channelConfig_3bits           = 0;

	    u32 i;
	    for(i = 0; i<NUM_SAMPLE_RATES; i++)
	    {
	        if(pHdlr->auRate == sampRateTab[i])
	            break;
	    }

	    if (i < NUM_SAMPLE_RATES)
	        sampRateIdx_4bits = i;
	    else
	    {
	        printf("unsupport audio sample rate.\n");
	        sampRateIdx_4bits = 0;
	    }

	    if(pHdlr->bStereo)
	        channelConfig_3bits = 2;
	    else
	        channelConfig_3bits = 1;

	    pkt->aac_hdr[0] = 0xff;
	    pkt->aac_hdr[1] = 0xf1;
	    pkt->aac_hdr[2] = 1<<6|sampRateIdx_4bits<<2;
	    pkt->aac_hdr[3] = channelConfig_3bits<<6;
	    pkt->aac_hdr[4] = 0;
	    pkt->aac_hdr[5] = 0x1f;
	    pkt->aac_hdr[6] = 0xfc;
	}

    pkt->bAudioParsed     = 1;
    pkt->bVideoParsed     = 1;

    return PMP_PARSER_OK;
}


/*
*********************************************************************************************************
*                               pmp_packet_destroy
*
*Description: release resource required by pmp_packet_create;
*
*Arguments  : pHdlr    - pmp file handler;
*
*Return     : PMP_PARSER_OK         - successed;
*
*Summary    : ;
*
*Author     : XC
*
*********************************************************************************************************
*/
static s32 pmp_packet_destroy(__PMPFILEHANDLER *pHdlr)
{
    __PMPPACKET *pkt;

    pkt = &pHdlr->pkt;
    if (pkt->tmpBuf)
    {
        free(pkt->tmpBuf);
        pkt->tmpBuf = 0;
    }

    if (pkt->auSize)
    {
        free(pkt->auSize[0]);
        free(pkt->auSize);
        pkt->auSize = 0;
    }

    return PMP_PARSER_OK;
}


/*
*********************************************************************************************************
*                               pmp_idxtab_build
*
*Description: build key frame index table;
*
*Arguments  : pHdlr    - pmp file handler;
*
*Return     : PMP_PARSER_OK          - successed;
*             PMP_PARSER_E_MEMORY    - malloc memory fail;
*             PMP_PARSER_IO_ERR      - cdx_read error;
*             PMP_PARSER_NOT_SUPPORT - too many key frames, it will take too
*                                      much memory, so we do not support it;
*
*Summary    : ;
*
*Author     : XC
*
*********************************************************************************************************
*/
static s32 pmp_idxtab_build(__PMPFILEHANDLER *pHdlr)
{
    s32            retVal;
    u32            bytesRead;
    u32            filePos;
    u32            frmCnt;
    u32            keyFrmCnt;
    u32            keyFrmNum;
    __pmpIndexRec* pIdx;
    __PMPKEYENTRY* pKeyIdx;

    pKeyIdx = 0;

    //* malloc memory for temporarily use.
    pIdx = (__pmpIndexRec*) malloc(pHdlr->vdFrmNum*4);
    if (!pIdx)
        return PMP_PARSER_E_MEMORY;

    //* read all the index entries.
    bytesRead = fread(pIdx, 1, pHdlr->vdFrmNum*4, pHdlr->fpMediaFile);
    if (bytesRead != pHdlr->vdFrmNum*4)
    {
        retVal = PMP_PARSER_IO_ERR;
        goto build_fail;
    }

    //* get key frame number
    keyFrmNum = 0;
    for(frmCnt = 0; frmCnt<pHdlr->vdFrmNum; frmCnt++)
    {
        keyFrmNum += pIdx[frmCnt].bKeyFrame;
    }

    if(keyFrmNum > MAX_KEY_FRM_NUM)
    {
        retVal = PMP_PARSER_NOT_SUPPORT;
        goto build_fail;
    }

    //* build key frame table
    pKeyIdx = (__PMPKEYENTRY*) malloc(keyFrmNum * sizeof(__PMPKEYENTRY));
    if(!pKeyIdx)
    {
        retVal = PMP_PARSER_E_MEMORY;
        goto build_fail;
    }

    keyFrmCnt = 0;
    filePos   = ftell(pHdlr->fpMediaFile);
    for(frmCnt = 0; frmCnt<pHdlr->vdFrmNum; frmCnt++)
    {
        if(pIdx[frmCnt].bKeyFrame)
        {
            pKeyIdx[keyFrmCnt].frmID   = frmCnt;
            pKeyIdx[keyFrmCnt].filePos = filePos;
            keyFrmCnt++;
        }

        filePos += pIdx[frmCnt].nFrmSize;
    }

    free(pIdx);
    pHdlr->pKeyFrmIdx = pKeyIdx;
    pHdlr->keyIdxNum  = keyFrmNum;
    return PMP_PARSER_OK;

build_fail:
    if (pIdx)
        free(pIdx);

    if (pKeyIdx)
        free(pKeyIdx);

    return retVal;

}


/*
*********************************************************************************************************
*                               pmp_idxtab_destroy
*
*Description: release resource required by pmp_idxtab_destroy;
*
*Arguments  : pHdlr - pmp file handler;
*
*Return     : PMP_PARSER_OK - successed;
*
*Summary    : ;
*
*Author     : XC
*
*********************************************************************************************************
*/
static s32 pmp_idxtab_destroy(__PMPFILEHANDLER *pHdlr)
{
    pHdlr->keyIdxNum  = 0;
    if (pHdlr->pKeyFrmIdx)
    {
        free(pHdlr->pKeyFrmIdx);
        pHdlr->pKeyFrmIdx = 0;
    }
    return PMP_PARSER_OK;
}

/*
*********************************************************************************************************
*                               pmp_set_media_format_info
*
*Description: set video and audio format information;
*
*Arguments  : pHdlr - pmp file handler;
*
*Return     : no;
*
*Summary    : ;
*
*Author     : XC
*
*********************************************************************************************************
*/
static void pmp_set_media_format_info(__PMPFILEHANDLER *pHdlr)
{
    if(pHdlr->vdCodecID == 1)
    {
        pHdlr->vFormat.format = CEDARV_STREAM_FORMAT_H264;
        pHdlr->vFormat.sub_format = CEDARV_SUB_FORMAT_UNKNOW;
        pHdlr->vFormat.container_format = CEDARV_CONTAINER_FORMAT_PMP;
    }
    else
    {
        pHdlr->vFormat.format = CEDARV_STREAM_FORMAT_MPEG4;
        pHdlr->vFormat.sub_format = CEDARV_MPEG4_SUB_FORMAT_XVID;
        pHdlr->vFormat.container_format = CEDARV_CONTAINER_FORMAT_PMP;
    }

    pHdlr->vFormat.video_width     		= pHdlr->vdWidth;
    pHdlr->vFormat.video_height       	= pHdlr->vdHeight;
    pHdlr->vFormat.frame_rate        	= pHdlr->vdRate / pHdlr->vdScale;
    pHdlr->vFormat.frame_duration   	= pHdlr->vdFrmInterval / 1000;
    pHdlr->vFormat.aspect_ratio      	= 1000;
    pHdlr->vFormat.init_data     		= NULL;
    pHdlr->vFormat.init_data_len		= 0;

#if 0
    if(pHdlr->auCodecID == 1)
        pHdlr->aFormat.codec_type = CDX_AUDIO_MPEG_AAC_LC;
    else
        pHdlr->aFormat.codec_type = CDX_AUDIO_MP3;

    pHdlr->aFormat.sub_codec_type = 0;

    if(pHdlr->bStereo)
        pHdlr->aFormat.channels = 2;
    else
        pHdlr->aFormat.channels = 1;

    pHdlr->aFormat.bits_per_sample      = 0;     //* no information
    pHdlr->aFormat.sample_per_second    = pHdlr->auRate;
    pHdlr->aFormat.avg_bit_rate         = 0;
    pHdlr->aFormat.max_bit_rate         = 0;
    pHdlr->aFormat.audio_bs_src         = 0;
    pHdlr->aFormat.extra_data_len       = 0;
    pHdlr->aFormat.extra_data           = 0;
#endif

    return;
}
