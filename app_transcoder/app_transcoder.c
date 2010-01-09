/*
 * Asterisk -- An open source telephony toolkit.
 *
 * Sergio Garcia Murillo <sergio.garcia@fontventa.com>
 *
 * See http://www.asterisk.org for more information about
 * the Asterisk project. Please do not directly contact
 * any of the maintainers of this project for assistance;
 * the project provides a web site, mailing lists and IRC
 * channels for your use.
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License Version 2. See the LICENSE file
 * at the top of the source tree.
 */

/*! \file
 *
 * \brief Video transcoding
 * 
 * \ingroup applications
 */

#include <asterisk.h>

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include <asterisk/lock.h>
#include <asterisk/file.h>
#include <asterisk/logger.h>
#include <asterisk/channel.h>
#include <asterisk/pbx.h>
#include <asterisk/module.h>
#include <asterisk/causes.h>
#include <asterisk/version.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>

#ifndef AST_FORMAT_AMR
#define AST_FORMAT_AMR		(1 << 13)
#define AST_FORMAT_MPEG4 	(1 << 22)
#endif

#define PKT_PAYLOAD     1450
#define PKT_SIZE        (sizeof(struct ast_frame) + AST_FRIENDLY_OFFSET + PKT_PAYLOAD)
#define PKT_OFFSET      (sizeof(struct ast_frame) + AST_FRIENDLY_OFFSET)

#if ASTERISK_VERSION_NUM>10600
#define AST_FRAME_GET_BUFFER(fr)        ((unsigned char*)((fr)->data.ptr))
#else
#define AST_FRAME_GET_BUFFER(fr)        ((unsigned char*)((fr)->data))
#endif


struct VideoTranscoder
{
	int end;
	struct ast_channel *channel;

	/* Decoder */
	AVCodec         *decoder;
        AVCodecContext  *decoderCtx;
        AVFrame         *decoderPic;
	int		decoderOpened;

	uint8_t		*pictures[2];
	int	picIndex;
	int	width;
	int 	height;
	int 	newPic;

	uint8_t		*frame;
	uint32_t	frameSize;
	uint32_t	frameLen;

	/* Encoder */
	AVCodec         *encoder;
        AVCodecContext  *encoderCtx;
        AVFrame         *encoderPic;
	int		encoderFormat;
	int		encoderOpened;
	
	uint8_t		*buffer;
	uint32_t	bufferSize;
	uint32_t	bufferLen;
	int 	mb;
	int	mb_total;
	int 	sent_bytes;

	/* Encoder Params */
	int	bitrate;
	int	fps;
	int	format;
	int	qMin;
	int	qMax;
	int	encoderWidth;
	int 	encoderHeight;
	int 	gop_size;

	/* Encoder thread */
	pthread_t encoderThread;

	/* Resize */
	struct SwsContext* resizeCtx;
	int	resizeWidth;
	int	resizeHeight;
	uint8_t	*resizeBuffer;
	int	resizeSrc[3];
	int	resizeDst[3];
	int	resizeFlags;
};

struct RFC2190H263HeadersBasic
{
        //F=0             F=1
        //P=0   I/P frame       I/P mode b
        //P=1   B frame         B fame mode C
        uint32_t trb:9;
        uint32_t tr:3;
        uint32_t dbq:2;
        uint32_t r:3;
        uint32_t a:1;
        uint32_t s:1;
        uint32_t u:1;
        uint32_t i:1;
        uint32_t src:3;
        uint32_t ebits:3;
        uint32_t sbits:3;
        uint32_t p:1;
        uint32_t f:1;
};


void * VideoTranscoderEncode(void *param);



static void SendH263VideoFrame(struct VideoTranscoder *vtc)
{
	uint8_t frameBuffer[PKT_SIZE];
	struct ast_frame *send = (struct ast_frame *) frameBuffer;
	uint8_t *frameData = NULL;

	uint8_t *data;
	int first = 1;
	int last  = 0;
	uint32_t sent  = 0;
	uint32_t len   = 0;
	uint32_t size  = 0;
			
	/* Send */
	while(sent<vtc->bufferLen)
	{
		/* Set data values */
		data = vtc->buffer+sent;

		/* clean */
		memset(send,0,PKT_SIZE);

		/* Set frame data */
		AST_FRAME_SET_BUFFER(send,send,PKT_OFFSET,0);

		/* Get the frame pointer */
		frameData = AST_FRAME_GET_BUFFER(send);

		/* Check remaining */
		if (sent+1400>vtc->bufferLen)
		{
			/* last */
			last = 1;
			/* send the rest */
			len = vtc->bufferLen-sent;
		} else 
			/* Fill */
			len = 1400;


		/* Debug */
		ast_log(LOG_DEBUG,"Send video frame [%p,%d,%d,%d,0x%.2x,0x%.2x,0x%.2x,0x%.2x]\n",send,len,first,last,data[0],data[1],data[2],data[3]);

		/* Check size */
		if (len+2>PKT_PAYLOAD)
		{
			/* Error */
			ast_log(LOG_ERROR,"Send video frame too large [%d]\n",len);
			/* Exit */
			return ;
		}

		/* if it¡s first */
		if (first)
		{
			/* Set frame len*/
			send->datalen = len;
			/* Copy */
			memcpy(frameData+2, data+2, len-2);
			/* Set header */
			frameData[0] = 0x04;
			frameData[1] = 0x00; 
			/* Set timestamp */
			send->samples = 90000/vtc->fps;
		} else {
			/* Set frame len */
			send->datalen = len+2;
			/* Copy */
			memcpy(frameData+2, data, len);
			/* Set header */
			frameData[0] = 0x00;
			frameData[1] = 0x00;
			/* Set timestamp */
			send->samples = 0;
		}

		/* Set video type */
		send->frametype = AST_FRAME_VIDEO;
		/* Set codec value */
		send->subclass = vtc->encoderFormat | last;
		/* Rest of values*/
		send->src = "transcoder";
		send->delivery = ast_tv(0, 0);
		/* Don't free the frame outrside */
		send->mallocd = 0;

		/* Send */
		ast_write(vtc->channel, send);

		/* Unset first */
		first = 0;
		/* Increment size */
		sent += len;
	}
}

static uint32_t h264_get_nal_size (uint8_t *pData, uint32_t sizeLength)
{
	uint32_t i ;
	for (i=0;i<sizeLength-4;i++)
		if (pData[i]==0 && pData[i+1]==0 && pData[i+2]==0 && pData[i+3]==1)
			return i;
	return sizeLength;
	/*if (sizeLength == 1) {
		return *pData;
	} else if (sizeLength == 2) {
		return (pData[0] << 8) | pData[1];
	} else if (sizeLength == 3) {
		return (pData[0] << 16) |(pData[1] << 8) | pData[2];
	}
	return (pData[0] << 24) |(pData[1] << 16) |(pData[2] << 8) | pData[3];*/
}

static void SendH264VideoFrame(struct VideoTranscoder *vtc)
{
	uint8_t frameBuffer[PKT_SIZE];
	struct ast_frame *send = (struct ast_frame *) frameBuffer;
	uint8_t *frameData = NULL;

	uint8_t *data;
	uint32_t dataLen;
	int first = 1;
	int lastNal  = 0;


	/* Get data */
	data = vtc->buffer;
	dataLen = vtc->bufferLen;

	ast_log(LOG_DEBUG,"[0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x]\n",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7]);


	while (dataLen>0)
	{
		/* Skip the start code */
		if (data[0]==0 && data[1]==0 && data[2]==0 && data[3]==1)
		{
			/*Increase start*/
			data+=4;
			dataLen-=4;
			/* Debug */
			ast_log(LOG_DEBUG,"Skipping header\n");
		}

		/* Get NAL header */
		uint8_t nalHeader = data[0];

		/* Get NAL type */
		uint8_t nalType = nalHeader& 0x1f;

		/* Get NAL size */
		uint32_t nalSize = h264_get_nal_size(data,dataLen);

		ast_log(LOG_DEBUG,"[0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x,0x%x]\n",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7]);
		/* Debug */
		ast_log(LOG_DEBUG,"Got %d NAL type of size %d remaining %d\n",nalType,nalSize,dataLen);

		/* Check if it is the last NAL */
		if (dataLen==nalSize)
			/* Yes */
			lastNal = 1;

		if (nalSize==0)
			return;
			
		/* Check if it fits in a udp packet */
		if (nalSize<1400)
		{
			/* clean */
			memset(send,0,PKT_SIZE);
			/* Set frame data */
			AST_FRAME_SET_BUFFER(send,send,PKT_OFFSET,0);
			/* Get the frame pointer */
			frameData = AST_FRAME_GET_BUFFER(send);
			/* Set frame len */
			send->datalen = nalSize;
			/* Copy NAL */
			memcpy(frameData, data, nalSize);
			/* if it¡s first */
			if (first)
				/* Set timestamp */
				send->samples = 90000/vtc->fps;
			else 
				/* Set timestamp */
				send->samples = 0;
			/* Set video type*/
			send->frametype = AST_FRAME_VIDEO;
			/* Set codec value */
			send->subclass = vtc->encoderFormat | lastNal;
			/* Rest of values*/
			send->src = "transcoder";
			send->delivery = ast_tv(0, 0);
			/* Don't free the frame outrside */
			send->mallocd = 0;
			/* Send */
			ast_write(vtc->channel, send);
			/* Unset first */
			first = 0;
			/* Debug */
			ast_log(LOG_DEBUG,"Sending h264 NAL [%d,%d,%d,%d,%d]\n",send->datalen,nalSize,dataLen,first,lastNal);
		} else {
			/*  RTP payload format for FU-As. 
			 *  An FU-A consists of a fragmentation unit indicator of one octet,
			 *  a fragmentation unit header of one octet, and a fragmentation unit payload.
			 *
 			 *  A fragment of a NAL unit consists of an integer number of consecutive octets of that NAL unit.
			 *  Each octet of the NAL unit MUST be part of exactly one fragment of that NAL unit.

			       0                   1                   2                   3
			       0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
			      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			      | FU indicator  |   FU header   |                               |
			      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+                               |
			      |                                                               |
			      |                         FU payload                            |
			      |                                                               |
			      |                               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			      |                               :...OPTIONAL RTP padding        |
			      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
			**/

			uint32_t sent  = 1;
			uint32_t len   = 0;
			int last  = 0;

			ast_log(LOG_DEBUG, "NAL Unit DOES NOT fit in one packet datasize=%d\n", nalSize);

			/* Send */
			while(!last)
			{
				/* clean */
				memset(send,0,PKT_SIZE);

				/* Set frame data */
				AST_FRAME_SET_BUFFER(send,send,PKT_OFFSET,0);

				/* Get the frame pointer */
				frameData = AST_FRAME_GET_BUFFER(send);
				
				/* Check sizes */
				if (nalSize-sent<1400)
				{
					/* Set remaining size */
					len = nalSize-sent;
					/* It is the last one */
					last = 1;
				} else {
					/* Set Max size */
					len = 1400;
				}

				ast_log(LOG_DEBUG, "Inside  FU-A fragmentation len=%d\n last=%d", len, last);


				/* FU indicator 
				 * The FU indicator octet has the following format:

				      +---------------+
				      |0|1|2|3|4|5|6|7|
				      +-+-+-+-+-+-+-+-+
				      |F|NRI|  Type   |
				      +---------------+
				**/
				frameData[0] = (nalHeader & 0x60) | 28;

				/* FU Header 
				 * The FU header has the following format:

				      +---------------+
				      |0|1|2|3|4|5|6|7|
				      +-+-+-+-+-+-+-+-+
				      |S|E|R|  Type   |
				      +---------------+

				   S: 1 bit
				      When set to one, the Start bit indicates the start of a fragmented
				      NAL unit.  When the following FU payload is not the start of a
				      fragmented NAL unit payload, the Start bit is set to zero.

				   E: 1 bit
				      When set to one, the End bit indicates the end of a fragmented NAL
				      unit, i.e., the last byte of the payload is also the last byte of
				      the fragmented NAL unit.  When the following FU payload is not the
				      last fragment of a fragmented NAL unit, the End bit is set to
				      zero.

				   R: 1 bit
				      The Reserved bit MUST be equal to 0 and MUST be ignored by the
				      receiver.

				   Type: 5 bits
				      The NAL unit payload type as defined in table 7-1 of [1].
				**/
				frameData[1] = (first << 7) | (last << 6) | (nalHeader & 0x1f);
				
				/* Copy the rest of the data skipping NAL header*/
				memcpy (frameData+2, data+sent, len);

				/* Set frame len */
				send->datalen = len + 2;

				/* if it¡s first */
				if (first)
					/* Set timestamp */
					send->samples = 90000/vtc->fps;
				else 
					/* Set timestamp */
					send->samples = 0;

				/* Set video type */
				send->frametype = AST_FRAME_VIDEO;
				/* Set codec value */
				send->subclass = vtc->encoderFormat | (last & lastNal);
				/* Rest of values*/
				send->src = "transcoder";
				send->delivery = ast_tv(0, 0);
				/* Don't free the frame outrside */
				send->mallocd = 0;
				/* Send */
				ast_write(vtc->channel, send);
				/* Unset first */
				first = 0;
				/* Increment size */
				sent += len;
				/* Debug */
				ast_log(LOG_DEBUG,"Sending h264 NAL FU [%d,%d,%d,%d,%d,%d,%d,%d]\n",send->datalen,nalSize,dataLen,first,lastNal,last,len,sent);
			}
		}
		/* Next NAL */
		data+=nalSize;
		dataLen-=nalSize;
	}
}

static int VideoTranscoderSetResize(struct VideoTranscoder *vtc,int width,int height)
{
	/* If already resizing that size */
	if (width==vtc->resizeWidth && height==vtc->resizeHeight)
		/* Nothing to do */
		return 1;

	/* if got contex */
	if (vtc->resizeCtx)
		/* Free it */
		sws_freeContext(vtc->resizeCtx);

	/* Get new context */
	vtc->resizeCtx = sws_getContext(vtc->width, vtc->height, PIX_FMT_YUV420P, vtc->encoderWidth, vtc->encoderHeight, PIX_FMT_YUV420P, vtc->resizeFlags, NULL, NULL, NULL);

	/* Check */
	if (!vtc->resizeCtx)
		/* Exit */
		return 0;

	/* Set values */
	vtc->resizeWidth = width;
	vtc->resizeHeight = height;

	/* Set values */
	vtc->resizeSrc[0] = vtc->resizeWidth;
	vtc->resizeSrc[1] = vtc->resizeWidth/2;
	vtc->resizeSrc[2] = vtc->resizeWidth/2;
	vtc->resizeDst[0] = vtc->encoderWidth;
	vtc->resizeDst[1] = vtc->encoderWidth/2;
	vtc->resizeDst[2] = vtc->encoderWidth/2;

	/* If already alloc */
	if (vtc->resizeBuffer)
		/* Free */
		free(vtc->resizeBuffer);

	/* Malloc buffer for resized image */
	vtc->resizeBuffer = malloc(vtc->encoderWidth*vtc->encoderHeight*3/2);

	/* exit */
	return 1;
}

void * VideoTranscoderEncode(void *param)
{
        struct timeval tv;

	/* Get transcoder context */
	struct VideoTranscoder *vtc = (struct VideoTranscoder*) param;

	/* Until stoped */
	while (!vtc->end)
	{
		/* Calculate sleep time */
		tv.tv_sec  = 0;
		tv.tv_usec = 1000000/vtc->fps;

		/* Sleep */
        	select(0,0,0,0,&tv);

		/* If there are new pic*/
		if (vtc->newPic)
		{
			/* Get buyffer */
			uint8_t* buffer =  vtc->pictures[vtc->picIndex];

			/* Change picture decoding index */
			vtc->picIndex = !vtc->picIndex;


			/* Recalc fps */
			//ctx->frame_rate = (int)ctx->fps*ctx->frame_rate_base;

			/* Do we need to resize the image */
			if ( vtc->width!=vtc->encoderWidth || vtc->height!=vtc->encoderHeight)
			{
				/* Set size */
				if (!VideoTranscoderSetResize(vtc, vtc->width, vtc->height))
					/* Next frame */
					continue;

				/* src & dst */
				uint8_t* src[3];
				uint8_t* dst[3];

				/* Set input picture data */
				int numPixels = vtc->width*vtc->height;
				int resPixels = vtc->encoderWidth*vtc->encoderHeight;

				/* Set pointers */
				src[0] = buffer;
				src[1] = buffer+numPixels;
				src[2] = buffer+numPixels*5/4;
				dst[0] = vtc->resizeBuffer;
				dst[1] = vtc->resizeBuffer+resPixels;
				dst[2] = vtc->resizeBuffer+resPixels*5/4;

				/* Resize frame */
				sws_scale(vtc->resizeCtx, src, vtc->resizeSrc, 0, vtc->height, dst, vtc->resizeDst);

				/* Set resized buffer */
				buffer = vtc->resizeBuffer;
			} 

			/* Set counters */
			vtc->mb = 0;
			vtc->mb_total = ((vtc->encoderWidth+15)/16)*((vtc->encoderHeight+15)/16);
			vtc->sent_bytes = 0;

			/* Set input picture data */
			int numPixels = vtc->encoderWidth*vtc->encoderHeight;

			/* Set image data */
			vtc->encoderPic->data[0] = buffer;
			vtc->encoderPic->data[1] = buffer+numPixels;
			vtc->encoderPic->data[2] = buffer+numPixels*5/4;
			vtc->encoderPic->linesize[0] = vtc->encoderWidth;
			vtc->encoderPic->linesize[1] = vtc->encoderWidth/2;
			vtc->encoderPic->linesize[2] = vtc->encoderWidth/2;

			/* Encode */
			vtc->bufferLen = avcodec_encode_video(vtc->encoderCtx,vtc->buffer,vtc->bufferSize,vtc->encoderPic);

			/* Debug */
			ast_log(LOG_DEBUG,"Encoded frame [%d,0x%.2x,0x%.2x,0x%.2x,0x%.2x]\n",vtc->bufferLen,vtc->buffer[0],vtc->buffer[1],vtc->buffer[2],vtc->buffer[3]);
			

			/* Send frame */
			if (vtc->encoderFormat == AST_FORMAT_H263_PLUS) 
				/*Send h263 rfc 2429 */
				SendH263VideoFrame(vtc);
			else if (vtc->encoderFormat == AST_FORMAT_H264)
				/*Send h264 */
				SendH264VideoFrame(vtc);


			/* Reset new pic flag */
			vtc->newPic = 0;
		}
	}

	/* Exit */
	return 0;
		
}

static int VideoTranscoderDestroy(struct VideoTranscoder *vtc)
{
	/* End encoder */
	vtc->end = 1;

	ast_log(LOG_WARNING,"-joining thread\n");

	/* Wait encoder thread to stop */
	pthread_join(vtc->encoderThread,0);

	ast_log(LOG_WARNING,"-joined thread\n");

	/* Free pictures */
	free(vtc->pictures[0]);
	free(vtc->pictures[1]);

	/* Free frames */
	free(vtc->frame);
	free(vtc->buffer);

	/* Free decoder */
	if (vtc->decoderCtx)
	{
		/*If already open */
		if (vtc->decoderOpened)
			/* Close */
			avcodec_close(vtc->decoderCtx);
		/* Free */
        	free(vtc->decoderCtx);
	}
	/* Free pic */
        if (vtc->decoderPic)
		free(vtc->decoderPic);

	/* Free encoder */
	if (vtc->encoderCtx)
	{
		/* If encoder opened */
		if (vtc->encoderOpened)
			/* Close */
			avcodec_close(vtc->encoderCtx);
		free(vtc->encoderCtx);
	}
	/* Free pic */
	if (vtc->encoderPic)
        	free(vtc->encoderPic);

	/* if got contex */
	if (vtc->resizeCtx)
		/* Free it */
		sws_freeContext(vtc->resizeCtx);


	/* Free resize buffer*/
	if (vtc->resizeBuffer)
		/* Free */
		free(vtc->resizeBuffer);

	/* Free */
	free(vtc);

	/* Exit */
	return 1;
}

static struct VideoTranscoder * VideoTranscoderCreate(struct ast_channel *channel,char *format)
{
	char *i;

	/* Check params */
	if (strncasecmp(format,"h263",4) && strncasecmp(format,"h264",4))
		/* Only h263 or h264 output by now*/
		return NULL;

	/* Create transcoder */
	struct VideoTranscoder *vtc = (struct VideoTranscoder *) malloc(sizeof(struct VideoTranscoder));

	/* Set channel */
	vtc->channel	= channel;

	/* Set default parameters */
	vtc->format 	= 0;
	vtc->fps	= -1;
	vtc->bitrate 	= -1;
	vtc->qMin	= -1;
	vtc->qMax	= -1;
	vtc->gop_size	= -1;

	/* Get first parameter */
	i = strchr(format,'@');

	/* Parse param */
	while (i)
	{
		/* skip separator */
		i++;

		/* compare */
		if (strncasecmp(i,"qcif",4)==0)
		{
			/* Set qcif */
			vtc->format = 0;
		} else if (strncasecmp(i,"cif",3)==0) {
			/* Set cif */
			vtc->format = 1;
		} else if (strncasecmp(i,"fps=",4)==0) {
			/* Set fps */
			vtc->fps = atoi(i+4);
		} else if (strncasecmp(i,"kb=",3)==0) {
			/* Set bitrate */
			vtc->bitrate = atoi(i+3)*1024;
		} else if (strncasecmp(i,"qmin=",5)==0) {
			/* Set qMin */
			vtc->qMin = atoi(i+5);
		} else if (strncasecmp(i,"qmax=",5)==0) {
			/* Set qMax */
			vtc->qMax = atoi(i+5);
		} else if (strncasecmp(i,"gs=",3)==0) {
			/* Set gop size */
			vtc->gop_size = atoi(i+3);
		}

		/* Find next param*/
		i = strchr(i,'/');
	}


	/* Depending on the format */
	switch(vtc->format)
	{
		case 0:
			vtc->encoderWidth  = 176;
			vtc->encoderHeight = 144;
			break;
		case 1:
			vtc->encoderWidth  = 352;
			vtc->encoderHeight = 288;
			break;
	}	

	/* Malloc input frame */
	vtc->frameSize	= 65535;
	vtc->frameLen	= 0;
	vtc->frame 	= (uint8_t *)malloc(65535);

	/* Malloc output frame */
	vtc->bufferSize	= 65535;
	vtc->bufferLen	= 0;
	vtc->buffer 	= (uint8_t *)malloc(65535);

	/* Malloc decodec pictures */
	vtc->pictures[0] = (uint8_t *)malloc(1179648); /* Max YUV 1024x768 */
	vtc->pictures[1] = (uint8_t *)malloc(1179648); /* 1204*768*1.5 */

	/* First input frame */
	vtc->picIndex	= 0;
	vtc->newPic	= 0;
	vtc->end 	= 0;

	/* Alloc context */
        vtc->decoderCtx = avcodec_alloc_context();
        vtc->encoderCtx = avcodec_alloc_context();

	/* Allocate pictures */
        vtc->decoderPic = avcodec_alloc_frame();
        vtc->encoderPic = avcodec_alloc_frame();

	/* Find encoder */
	if (!strncasecmp(format,"h263",4))
	{
		/* H263 encoder */
		vtc->encoder = avcodec_find_encoder(CODEC_ID_H263); 
		/* Set rfc 2490 payload */
		vtc->encoderFormat = AST_FORMAT_H263_PLUS;
		/* Flags */
		vtc->encoderCtx->mb_decision = FF_MB_DECISION_SIMPLE;
		vtc->encoderCtx->flags |= CODEC_FLAG_PASS1;                 //PASS1
		vtc->encoderCtx->flags &= ~CODEC_FLAG_H263P_UMV;            //unrestricted motion vector
		vtc->encoderCtx->flags &= ~CODEC_FLAG_4MV;                  //advanced prediction
		vtc->encoderCtx->flags &= ~CODEC_FLAG_H263P_SLICE_STRUCT;
	} else if (!strncasecmp(format,"h264",4)) {
		/* H264 encoder */
		vtc->encoder = avcodec_find_encoder(CODEC_ID_H264); 
		/* Set rfc payload */
		vtc->encoderFormat = AST_FORMAT_H264;
		/* Add x4->params.i_slice_max_size     = 1350; in X264_init function of in libavcodec/libx264.c */
		/* Fast encodinf parameters */
		vtc->encoderCtx->refs = 1;
		vtc->encoderCtx->scenechange_threshold = 0;
		vtc->encoderCtx->me_subpel_quality = 0;
		vtc->encoderCtx->partitions = X264_PART_I8X8 | X264_PART_I8X8;
		vtc->encoderCtx->me_method = ME_EPZS;
		vtc->encoderCtx->trellis = 0;
	}	

	ast_log(LOG_DEBUG,"-Transcoder [c=%d,f=%d,fps=%d,kb=%d,qmin=%d,qmax=%d,gs=%d]\n",vtc->encoderFormat,vtc->format,vtc->fps,vtc->bitrate,vtc->qMin,vtc->qMax,vtc->gop_size);

	/* No decoder still */
	vtc->decoder = NULL;
	vtc->decoderOpened = 0;

	/* No resize */
	vtc->resizeCtx		= NULL;
	vtc->resizeWidth	= 0;
	vtc->resizeHeight	= 0;
	vtc->resizeBuffer	= NULL;
	/* Bicubic by default */
	vtc->resizeFlags	= SWS_BICUBIC;

	/* Picture data */
	vtc->encoderCtx->pix_fmt 	= PIX_FMT_YUV420P;
	vtc->encoderCtx->width		= vtc->encoderWidth;
	vtc->encoderCtx->height 	= vtc->encoderHeight;

	/* fps*/
	if (vtc->fps>0)
		/* set encoder params*/
        	vtc->encoderCtx->time_base    	    = (AVRational){1,vtc->fps};/* frames per second */

        /* Bitrate */
	if (vtc->bitrate>0 && vtc->fps>0)
	{
		/* Set encoder params */
		vtc->encoderCtx->bit_rate           = vtc->bitrate;
        	vtc->encoderCtx->bit_rate_tolerance = vtc->bitrate*av_q2d(vtc->encoderCtx->time_base) + 1;
	}


	/* gop size */
	if (vtc->gop_size>0)
		/* set encoder params*/
        	vtc->encoderCtx->gop_size           = vtc->gop_size; // about one Intra frame per second

	/* Bitrate */
	if (vtc->bitrate>0)
	{
		/* set encoder params*/
		vtc->encoderCtx->rc_min_rate        = vtc->bitrate;
		vtc->encoderCtx->rc_max_rate        = vtc->bitrate;
	}	

	/* qMin */
	if (vtc->qMin>0)
		vtc->encoderCtx->mb_qmin = vtc->encoderCtx->qmin= vtc->qMin;
	/* qMax */
	if (vtc->qMax>0)
		vtc->encoderCtx->mb_qmax = vtc->encoderCtx->qmax= vtc->qMax;

        /* Video quality */
        vtc->encoderCtx->rc_buffer_size     = vtc->bufferSize;
        vtc->encoderCtx->rc_qsquish         = 0; //ratecontrol qmin qmax limiting method.
        vtc->encoderCtx->max_b_frames       = 0;
	vtc->encoderCtx->me_range 	    = 24;
	vtc->encoderCtx->max_qdiff 	    = 31;
        vtc->encoderCtx->i_quant_factor     = (float)-0.6;
        vtc->encoderCtx->i_quant_offset     = (float)0.0;
	vtc->encoderCtx->qcompress	    = 0.6;
	/* Open encoder */
	vtc->encoderOpened = avcodec_open(vtc->encoderCtx, vtc->encoder) != -1;

	/* If not opened correctly */
	if (!vtc->encoderOpened)
	{
		/* Error */
		ast_log(LOG_ERROR,"Error opening encoder\n");
		/* Destroy it */
		VideoTranscoderDestroy(vtc);
		/* Exit */
		return NULL;	
	}

	/* Start encoder thread */
	pthread_create(&vtc->encoderThread,NULL,VideoTranscoderEncode,vtc);

	/* Return encoder */
	return vtc;
}

static void VideoTranscoderCleanFrame(struct VideoTranscoder *vtc)
{
	/* Reset length*/
	vtc->frameLen = 0;
}

static int VideoTranscoderDecodeFrame(struct VideoTranscoder *vtc)
{
	uint8_t *bufDecode;
	int got_picture;
	int i;

	/* Decode */
	avcodec_decode_video(vtc->decoderCtx,vtc->decoderPic,&got_picture,vtc->frame,vtc->frameLen);

	/* If it can be decoded */
	if (got_picture)
	{
		/* Check size */
		if(vtc->decoderCtx->width==0 || vtc->decoderCtx->height==0)
			/* Exit */
			return 0;

		/* Get pointer to frame */
		bufDecode = vtc->pictures[vtc->picIndex];

		/* Save size */
		vtc->width  = vtc->decoderCtx->width;
		vtc->height = vtc->decoderCtx->height;

		/* Get sizes */
		int h = vtc->decoderCtx->height;
		int w = vtc->decoderCtx->width;
		int t = vtc->decoderCtx->width/2;
		int u = w*h;
		int v = w*h*5/4;

		/* Copy Y */
		for(i=0;i<h;i++)
			memcpy(&bufDecode[i*w],&vtc->decoderPic->data[0][i*vtc->decoderPic->linesize[0]],w);

		/* Copy U & V */
		for(i=0;i<h/2;i++)
		{
			memcpy(&bufDecode[i*t+u],&vtc->decoderPic->data[1][i*vtc->decoderPic->linesize[1]],t);
			memcpy(&bufDecode[i*t+v],&vtc->decoderPic->data[2][i*vtc->decoderPic->linesize[2]],t);
		}

		/* Set new frame flag */
		vtc->newPic = 1;
	}

	/* Got frame */
	return got_picture;
}

static void VideoTranscoderSetDecoder(struct VideoTranscoder *vtc,int codec)
{
	/* If already opened that codec */
	if (vtc->decoder && vtc->decoderCtx->codec_id == codec)
		/* Exit */
		return;

	/*If already open */
	if (vtc->decoderOpened)
		/* Close previous one */
		avcodec_close(vtc->decoderCtx);

	/* Get decoder */
	vtc->decoder = avcodec_find_decoder(codec);

	/* Clean frame */
	VideoTranscoderCleanFrame(vtc);

        /* Set context parameters*/
        vtc->decoderCtx->workaround_bugs    = 1;
        vtc->decoderCtx->error_concealment  = FF_EC_GUESS_MVS | FF_EC_DEBLOCK;
	vtc->decoderCtx->flags |= CODEC_FLAG_PART;

        /* Open */
        avcodec_open(vtc->decoderCtx, vtc->decoder);

	/* We are open*/
	vtc->decoderOpened = 1;
}


static uint32_t rfc2190_append(uint8_t *dest, uint32_t destLen, uint8_t *buffer, uint32_t bufferLen)
{

	/* Check length */
	if (bufferLen<sizeof(struct RFC2190H263HeadersBasic))
		/* exit */
		return 0;

	/* Get headers */
	uint32_t x = ntohl(*(uint32_t *)buffer);

	/* Set headers */
	struct RFC2190H263HeadersBasic *headers = (struct RFC2190H263HeadersBasic *)&x;

	/* Get ini */
	uint8_t* in = buffer + sizeof(struct RFC2190H263HeadersBasic);
	uint32_t  len = sizeof(struct RFC2190H263HeadersBasic);

	/* If C type */
	if (headers->f)
	{
		/* Skip rest of header */
		in+=4;
		len-=4;
	}

	/* Skip first bits */
	in[0] &= 0xff >> headers->sbits;

	/* Skip end bits */
	if (len>0)
		in[len-1] &= 0xff << headers->ebits;

	/* Mix first and end byte */
	if(headers->sbits!=0 && destLen>0)
	{
		/* Append to previous byte */
		dest[destLen-1] |= in[0];
		/* Skip first */
		in++;
		len--;
	}

	/* Copy the rest */
	memcpy(dest+destLen,in,len);

	/* Return added */
	return len;
}

static uint32_t rfc2429_append(uint8_t *dest, uint32_t destLen, uint8_t *buffer, uint32_t bufferLen)
{
	/* Debug */
	ast_log(LOG_DEBUG,"RFC2429 appending [%d:0x%.2x,0x%.2x]\n",bufferLen,buffer[0],buffer[1]);

	/* Check length */
	if (bufferLen<2)
		/* exit */
		return 0;

	 /* Get header */
	uint8_t p = buffer[0] & 0x04;
	uint8_t v = buffer[0] & 0x02;
	uint8_t plen = ((buffer[0] & 0x1 ) << 5 ) | (buffer[1] >> 3);
	uint8_t pebit = buffer[0] & 0x7;

	/* Get ini */
	uint8_t* in = buffer+2+plen;
	uint32_t  len = bufferLen-2-plen;

	/* Check */
	if (v)
	{
		/* Increase ini */
		in++;
		len--;
	}

	/* Check p bit */
	if (p)
	{
		/* Decrease ini */
		in -= 2;
		len += 2;
		/* Append 0s */
		buffer[0] = 0;
		buffer[1] = 0;
	}

	/* Copy the rest */
	memcpy(dest+destLen,in,len);

	/* Return added */
	return len;
}
/* 3 zero bytes syncword */
static const uint8_t sync_bytes[] = { 0, 0, 0, 1 };


static uint32_t h264_append(uint8_t *dest, uint32_t destLen, uint8_t *buffer, uint32_t bufferLen)
{
	uint8_t nal_unit_type;
	unsigned int header_len;
	uint8_t nal_ref_idc;
	unsigned int nalu_size;

	uint32_t payload_len = bufferLen;
	uint8_t *payload = buffer;
	uint8_t *outdata = dest+destLen;
	uint32_t outsize = 0;


	/* +---------------+
	 * |0|1|2|3|4|5|6|7|
	 * +-+-+-+-+-+-+-+-+
	 * |F|NRI|  Type   |
	 * +---------------+
	 *
	 * F must be 0.
	 */
	nal_ref_idc = (payload[0] & 0x60) >> 5;
	nal_unit_type = payload[0] & 0x1f;

	/* at least one byte header with type */
	header_len = 1;

	ast_log(LOG_DEBUG, "h264 receiving %d bytes nal unit type %d", payload_len, nal_unit_type);

	switch (nal_unit_type) 
	{
		case 0:
		case 30:
		case 31:
			/* undefined */
			return 0;
		case 25:
			/* STAP-B		Single-time aggregation packet		 5.7.1 */
			/* 2 byte extra header for DON */
			header_len += 2;
			/* fallthrough */
		case 24:
		{
			/* strip headers */
			payload += header_len;
			payload_len -= header_len;

			/**rtph264depay->wait_start = FALSE;*/

			/* STAP-A Single-time aggregation packet 5.7.1 */
			while (payload_len > 2) 
			{
				/*                      1					
				 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 
				 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
				 * |  ALU Size                     |
				 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
				 */
				nalu_size = (payload[0] << 8) | payload[1];

				/* strip NALU size */
				payload += 2;
				payload_len -= 2;

				if (nalu_size > payload_len)
					nalu_size = payload_len;

				outsize += nalu_size + sizeof (sync_bytes);

				// Check size
				if (outsize>destLen)
					return 0;

				memcpy (outdata, sync_bytes, sizeof (sync_bytes));
				outdata += sizeof (sync_bytes);
				memcpy (outdata, payload, nalu_size);
				outdata += nalu_size;

				payload += nalu_size;
				payload_len -= nalu_size;
			}

			return outsize;
		}
		case 26:
			/* MTAP16 Multi-time aggregation packet	5.7.2 */
			header_len = 5;
			return 0;
			break;
		case 27:
			/* MTAP24 Multi-time aggregation packet	5.7.2 */
			header_len = 6;
			return 0;
			break;
		case 28:
		case 29:
		{
			/* FU-A	Fragmentation unit	 5.8 */
			/* FU-B	Fragmentation unit	 5.8 */
			uint8_t S, E;

			/* +---------------+
			 * |0|1|2|3|4|5|6|7|
			 * +-+-+-+-+-+-+-+-+
			 * |S|E|R| Type	   |
			 * +---------------+
			 *
			 * R is reserved and always 0
			 */
			S = (payload[1] & 0x80) == 0x80;
			E = (payload[1] & 0x40) == 0x40;

			ast_log(LOG_DEBUG, "S %d, E %d", S, E);

			/*if (rtph264depay->wait_start && !S)*/
				/*goto waiting_start;*/

			if (S) 
			{
				/* NAL unit starts here */
				uint8_t nal_header;

				/*rtph264depay->wait_start = FALSE;*/

				/* reconstruct NAL header */
				nal_header = (payload[0] & 0xe0) | (payload[1] & 0x1f);

				/* strip type header, keep FU header, we'll reuse it to reconstruct
				 * the NAL header. */
				payload += 1;
				payload_len -= 1;

				nalu_size = payload_len;
				outsize = nalu_size + sizeof (sync_bytes);
				memcpy (outdata, sync_bytes, sizeof (sync_bytes));
				outdata += sizeof (sync_bytes);
				memcpy (outdata, payload, nalu_size);
				outdata[0] = nal_header;
				outdata += nalu_size;
				return outsize;

			} else {
				/* strip off FU indicator and FU header bytes */
				payload += 2;
				payload_len -= 2;

				outsize = payload_len;
				memcpy (outdata, payload, outsize);
				outdata += nalu_size;
				return outsize;
			}

			/* if NAL unit ends, flush the adapter */
			if (E) 
			{
				ast_log(LOG_DEBUG, "output %d bytes", outsize);

				return 0;
			}

			return outsize;
			break;
		}
		default:
		{
			/*rtph264depay->wait_start = FALSE;*/

			/* 1-23	 NAL unit	Single NAL unit packet per H.264	 5.6 */
			/* the entire payload is the output buffer */
			nalu_size = payload_len;
			outsize = nalu_size + sizeof (sync_bytes);
			memcpy (outdata, sync_bytes, sizeof (sync_bytes));
			outdata += sizeof (sync_bytes);
			memcpy (outdata, payload, nalu_size);
			outdata += nalu_size;

			return outsize;
		}
	}

	return 0;
}


static uint32_t mpeg4_append(uint8_t *dest, uint32_t destLen, uint8_t *buffer, uint32_t bufferLen)
{
	/* Just copy */
	memcpy(dest+destLen,buffer,bufferLen);
	/* Return added */
	return bufferLen;
}

static uint32_t VideoTranscoderWrite(struct VideoTranscoder *vtc, int codec, uint8_t *buffer, uint32_t bufferLen, int mark)
{
	/* Debug */
	ast_log(LOG_DEBUG,"Received video [%x,%d,%d]\n",codec,bufferLen,mark);

	/* If not enougth */
	if (bufferLen + vtc->frameLen > vtc->frameSize);
		/* Clean frame */

	/* Depending on the code */
	if (codec & AST_FORMAT_H263)
	{
		/* Check codec */
		VideoTranscoderSetDecoder(vtc,CODEC_ID_H263);
		/* Depacketize */
		vtc->frameLen += rfc2190_append(vtc->frame,vtc->frameLen,buffer,bufferLen);

	} else if (codec & AST_FORMAT_H263_PLUS) {
		/* Check codec */
		VideoTranscoderSetDecoder(vtc,CODEC_ID_H263);
		/* Depacketize */
		vtc->frameLen += rfc2429_append(vtc->frame,vtc->frameLen,buffer,bufferLen);

	} else if (codec & AST_FORMAT_H264) {
		/* Check codec */
		VideoTranscoderSetDecoder(vtc,CODEC_ID_H264);
		/* Depacketize */
		vtc->frameLen += h264_append(vtc->frame,vtc->frameLen,buffer,bufferLen);

	} else if (codec & AST_FORMAT_MPEG4) {
		/* Check codec */
		VideoTranscoderSetDecoder(vtc,CODEC_ID_MPEG4);
		/* Depacketize */
		vtc->frameLen += mpeg4_append(vtc->frame,vtc->frameLen,buffer,bufferLen);

	}else{
		ast_log(LOG_ERROR,"-Unknown codec [%d]\n",codec);
		return 0;
	}

	/* If mark set */
	if (mark)
	{
		/* Decode frame */
		VideoTranscoderDecodeFrame(vtc);
		/* Clean frame */
		VideoTranscoderCleanFrame(vtc);
	}

	return 1;
}


static int app_transcode(struct ast_channel *chan, void *data)
{
	struct ast_frame *f;
	struct ast_module_user *u;
	int    reason = 0;
	int    ms;
	struct ast_channel *channels[2];
	struct ast_channel *pseudo;
	struct ast_channel *where;
	struct VideoTranscoder *fwd = NULL;
	struct VideoTranscoder *rev = NULL;

	char *fwdParams;
	char *revParams;
	char *local;
	char *a;
	char *b;

	/* Find fwd params */
	if (!(a=strchr((char*)data,'|')))
		return 0;

	/* Find local channel params */
	if (!(b=strchr(a+1,'|')))
		return 0;

	/* Set local params */
	fwdParams = strndup((char*)data,a-(char*)data);
	local 	  = strndup(a+1,b-a-1);
	revParams = strndup(b+1,strlen((char*)data)-(b-(char*)data)-1);

	/* Lock module */
	u = ast_module_user_add(chan);

	/* Request new channel */
	pseudo = ast_request("Local", AST_FORMAT_H263 | AST_FORMAT_MPEG4 | AST_FORMAT_H263_PLUS | AST_FORMAT_H264 | chan->rawwriteformat, local, &reason);
 
	/* If somthing has gone wrong */
	if (!pseudo)
		/* goto end */
		goto end; 

	/* Copy global variables from incoming channel to local channel */
	ast_channel_inherit_variables(chan, pseudo);

	/* Set caller id */
	ast_set_callerid(pseudo, chan->cid.cid_num, chan->cid.cid_name, chan->cid.cid_num);

	/* Place call */
	if (ast_call(pseudo,data,0))
		/* if fail goto clean */
		goto clean_pseudo;

	/* while not setup */
	while (pseudo->_state!=AST_STATE_UP) {
		/* Wait for data */
		if (ast_waitfor(pseudo, 0)<0)
			/* error, timeout, or done */
			break;
		/* Read frame */
		f = ast_read(pseudo);
		/* If not frame */
		if (!f)
			/* done */ 
			break;
		/* If it's a control frame */
		if (f->frametype == AST_FRAME_CONTROL) {
			/* Dependinf on the event */
			switch (f->subclass) {
				case AST_CONTROL_RINGING:       
					break;
				case AST_CONTROL_BUSY:
				case AST_CONTROL_CONGESTION:
					/* Save cause */
					reason = pseudo->hangupcause;
					/* exit */
					goto hangup_pseudo;
					break;
				case AST_CONTROL_ANSWER:
					/* Set UP*/
					reason = 0;	
					break;
			}
		}
		/* Delete frame */
		ast_frfree(f);
	}

	/* If no answer */
	if (pseudo->_state != AST_STATE_UP)
		/* goto end */
		goto clean_pseudo; 

	/* Log */
	ast_log(LOG_WARNING,">Transcoding [%s,%s,%s]\n",fwdParams,local,revParams);

	/* Create contexts */
	fwd = VideoTranscoderCreate(pseudo,fwdParams);
	rev = VideoTranscoderCreate(chan,revParams);


	/* Answer channel */
	ast_answer(chan);

	/* Set up array */
	channels[0] = chan;
	channels[1] = pseudo;

	/* No timeout */
	ms = -1;

	/* Wait for data avaiable on any channel */
	while (!reason && (where = ast_waitfor_n(channels, 2, &ms)) != NULL) 
	{
		/* Read frame from channel */
		f = ast_read(where);

		/* if it's null */
		if (f == NULL)
			break;

		/* Depending on the channel */
		if (where == chan) 
		{
			/* if it's video */
			if (f->frametype == AST_FRAME_VIDEO) {
				/* If transcode forwdward */
				if (fwd)
				{
					/* Transcode */
					VideoTranscoderWrite(fwd,f->subclass,AST_FRAME_GET_BUFFER(f),f->datalen,f->subclass & 1);
					/* Delete frame */
					ast_frfree(f);
				} else {
					/* Just copy */
					ast_write(pseudo,f);
				}
			} else if (f->frametype == AST_FRAME_CONTROL)  {
				/* Check for hangup */
				if (f->subclass == AST_CONTROL_HANGUP)
				{
					/* Hangup */
					reason = AST_CAUSE_NORMAL_CLEARING;
					ast_log(LOG_WARNING,"-AST_CONTROL_HANGUP\n"); 
				}
				/* delete frame */
				ast_frfree(f);
			} else {
				/* Fordward */
				ast_write(pseudo,f);
			}
		} else {
			/* if it's video */
			if (f->frametype == AST_FRAME_VIDEO) {
				/* If transcode backward */
				if (rev)
				{
					/* Transcode */
					VideoTranscoderWrite(rev,f->subclass,AST_FRAME_GET_BUFFER(f),f->datalen,f->subclass & 1);
					/* Delete frame */
					ast_frfree(f);
				} else {
					/* Just copy */
					ast_write(chan,f);
				}
			} else if (f->frametype == AST_FRAME_CONTROL)  {
				/* Check for hangup */
				if (f->subclass == AST_CONTROL_HANGUP)
					/* Hangup */
					reason = AST_CAUSE_NORMAL_CLEARING;
				/* delete frame */
				ast_frfree(f);
			} else {
				/* Fordward */
				ast_write(chan,f);
			}
		}
	}

	/* Log */
	ast_log(LOG_WARNING,"-end loop");

	/* Destroy transcoders */
	if (fwd)
		VideoTranscoderDestroy(fwd);
	if (rev)
		VideoTranscoderDestroy(rev);

	ast_log(LOG_WARNING,"-Hanging up \n");

hangup_pseudo:
	/* Hangup pseudo channel if needed */
	ast_softhangup(pseudo, reason);

clean_pseudo:
	/* Destroy pseudo channel */
	ast_hangup(pseudo);

	/* Log */
	ast_log(LOG_WARNING,"<Transcoding\n");

end:
	/* Free params */
	free(fwdParams);
	free(local);
	free(revParams);


	/* Unlock module*/
	ast_module_user_remove(u);

	/* Exit without hangup*/
	return 0;
}

static char *name_transcode = "transcode";
static char *syn_transcode = "Video transcode";
static char *des_transcode = "  transcode(informat|channel|outformat):  Estabish connection and transcode video.\n";

static int unload_module(void)
{
	int res;
	res = ast_unregister_application(name_transcode);
	ast_module_user_hangup_all();

	return res;
}

static void av_log_asterisk_callback(void* ptr, int level, const char* fmt, va_list vl)
{
	char msg[1024];

	vsnprintf(msg,1024,fmt,vl);

	AVClass* avc= ptr ? *(AVClass**)ptr : NULL;
	if(avc)
		ast_log(LOG_DEBUG,"[%s @ %p] %s",avc->item_name(ptr), avc, msg);
	else 
		ast_log(LOG_DEBUG, msg);
}

static int load_module(void)
{
	/* Set log */
	av_log_set_callback(av_log_asterisk_callback);

	/* Init avcodec */
	avcodec_init();
	
	/* Register all codecs */	
	avcodec_register_all();

	return ast_register_application(name_transcode, app_transcode, syn_transcode, des_transcode);
}

AST_MODULE_INFO_STANDARD(ASTERISK_GPL_KEY, "Video transcoder application");

