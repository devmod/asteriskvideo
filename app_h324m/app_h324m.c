/*
 * Asterisk -- An open source telephony toolkit.
 *
 * Sergio Garcia Murillo <sergio.garcia@ydilo.com>
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
 * \brief H324M stack
 * 
 * \ingroup applications
 */

#include <asterisk.h>

#include <h324m.h>
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
#include <asterisk/time.h>
#include <asterisk/cli.h>
#include <asterisk/version.h>

#ifndef AST_FORMAT_AMRNB
#define AST_FORMAT_AMRNB 	(1 << 13)
#endif
#ifndef AST_FRAME_DIGITAL
#define AST_FRAME_DIGITAL 13
#endif

#define PKT_PAYLOAD     1450
#define PKT_SIZE        (sizeof(struct ast_frame) + AST_FRIENDLY_OFFSET + PKT_PAYLOAD)
#define PKT_OFFSET      (sizeof(struct ast_frame) + AST_FRIENDLY_OFFSET)

#if ASTERISK_VERSION_NUM>10600
#define AST_FRAME_GET_BUFFER(fr)	((unsigned char*)((fr)->data.ptr))
#else
#define AST_FRAME_GET_BUFFER(fr)	((unsigned char*)((fr)->data))
#endif


static char *name_h324m_loopback = "h324m_loopback";
static char *syn_h324m_loopback = "H324m loopback mode";
static char *des_h324m_loopback = "  h324m_loopback([options]):  Establish H.324M connection and loopback media.\n"
	"\n"
	"Note: By default this function loops audio and video. Looping audio can cause\n"
	"acoustic feedback. To prevent this you can turn it off using the 'a' option.\n"
	"\n"
	"Available options:\n"
	" 'a': deactivate loopback of audio\n"
	" 'v': deactivate loopback of video\n"
	"\n"
	"Note: This function just loops the h324m audio and video frames. Thus, there \n"
	"is no conversion from h324m frames into Asterisk internal frame format. If  \n"
	"you want to test the conversion from h324m frames to Asterisk frames and    \n"
	"vice versa too, do not use h324m_loopback() but use h324m_gw() and the Echo()\n"
	"application.\n"
	"\n"
	"Examples:\n"
	" h324m_loopback(): loopback audio and video\n"
	" h324m_loopback(a): deactivate loopback of audio\n"
	" h324m_loopback(v): deactivate loopback of video\n"
	" h324m_loopback(av): deactivate loopback of audio and video\n";

static char *name_h324m_gw = "h324m_gw";
static char *syn_h324m_gw = "H324m gateway";
static char *des_h324m_gw = "  h324m_gw(extension@context):  Creates a pseudo channel for an incoming h324m call.\n"
	"This function decodes the received H.324M data (usually via a ISDN connection\n"
	"and extracts the video and voice frames into Asterisk's internal frame format\n"
	"and vice versa.\n"
	"A pseudo channel will be created to continue dialplan execution at another\n"
	"extension/context.\n"
	"\n"
	"Examples:\n"
	" [frompstn]\n"
	" exten => 111,1,h324m_gw(britney@3gp_videos)\n"
	" exten => 112,1,h324m_gw(justin@3gp_videos)\n"
	" [3gp_videos]\n"
	" exten => britney,1,h324m_gw_answer()\n"
	" exten => britney,2,mp4_play(/var/videos/britney.3gp)\n"
	" exten => justin,1,h324m_gw_answer()\n"
	" exten => justin,2,mp4_play(/var/videos/justin.3gp)\n";

static char *name_h324m_call = "h324m_call";
static char *syn_h324m_call = "H324m call";
static char *des_h324m_call = "  h324m_call(extension@context):  Creates a pseudo channel for an outgoing h324m call.\n"
	"This function encodes the video and voice frames from Asterisk's internal\n"
	" frame format into H.324M data and vice versa.\n"
	"A pseudo channel will be created to continue dialplan execution at another\n"
	"extension/context.\n"
	"\n"
	"Examples:\n"
	" [fromsip]\n"
	" ;prefix 0 means PSTN with normal audio call\n"
	" ;prefix 1 means PSTN with 3G video calls\n"
	" exten => _0X.,1,Dial(Zap/${EXTEN:1}\n"
	" exten => _1X.,1,h324m_call(0${EXTEN:1}@fromsip)\n";

static char *name_h324m_gw_answer = "h324m_gw_answer";
static char *syn_h324m_gw_answer = "H324m Answer incoming call";
static char *des_h324m_gw_answer = "  h324m_gw_answer():  Answer an incomming call from h324m_gw and waits for 3G negotiation.\n";

static char *name_video_loopback = "video_loopback";
static char *syn_video_loopback = "video_loopback";
static char *des_video_loopback = "  video_loopback():  Video loopback.\n"
	"This function just loops the Asterisk video frames. Thus, it is similar\n"
	"to the Echo() application but only loops video (the Echo application loops\n"
	"audio and video). To use this function with H324M calls you first have to use\n"
	"the h324m_gw() function.\n";

/* Commands */
static char debug_usage[] =
"Usage: h324m debug level {0-9}\n"
"       Enables debug messages in app_h324m\n"
"        0 - No debug\n"
"        1 - Error messages\n"
"        2 - Log messages\n"
"        3 - Warning messages\n"
"        4 - Debug messages\n"
"        5 - File dumps\n";

#if ASTERISK_VERSION_NUM>10600
static char *h324m_do_debug(struct ast_cli_entry *e, int cmd, struct ast_cli_args *a)
{
	static char *choices[] = { "0", "1", "2", "3", "4", "5", NULL };
	int level;

	switch (cmd) 
	{
		case CLI_INIT:
			e->command = "h324m debug level";
			e->usage = debug_usage;
			return NULL;

		case CLI_GENERATE:
			if (a->pos > e->args)
				return NULL;
			return ast_cli_complete(a->word, choices, a->n);

		default:

			/* Check number of arguments */
			if (a->argc != 4)
				return RESULT_SHOWUSAGE;
			/* Get level */
			int level = atoi(a->argv[3]);

			/* Check it's correct */
			if((level < 0) || (level > 9))
				return RESULT_SHOWUSAGE;

			/* Print result */	
			if(level)
				ast_cli(a->fd, "app_h324m Debugging enabled level: %d\n", level);
			else
				ast_cli(a->fd, "app_h324m Debugging disabled\n");

			/* Set log level */
			H324MLoggerSetLevel(level);

			return CLI_SUCCESS;
	}
}

static struct ast_cli_entry  cli_debug = AST_CLI_DEFINE(h324m_do_debug,"Set app_h324m debug log level");

#else
/* Commands */
static int h324m_do_debug(int fd, int argc, char *argv[])
{
	int level;

	/* Check number of arguments */
	if (argc != 4)
		return RESULT_SHOWUSAGE;

	/* Get level */
	level = atoi(argv[3]);

	/* Check it's correct */
	if((level < 0) || (level > 9))
		return RESULT_SHOWUSAGE;

	/* Print result */	
	if(level)
		ast_cli(fd, "app_h324m Debugging enabled level: %d\n", level);
	else
		ast_cli(fd, "app_h324m Debugging disabled\n");

	/* Set log level */
	H324MLoggerSetLevel(level);

	/* Exit */
	return RESULT_SUCCESS;
}

static struct ast_cli_entry  cli_debug =
	{ { "h324m", "debug", "level" }, h324m_do_debug, "Set app_h324m debug log level", debug_usage };
#endif

/*

These are the different AMR modes (Table 1 from RFC 3267)

                                     Class A   total speech
                   Index   Mode       bits       bits
                   ----------------------------------------
                     0     AMR 4.75   42         95
                     1     AMR 5.15   49        103
                     2     AMR 5.9    55        118
                     3     AMR 6.7    58        134
                     4     AMR 7.4    61        148
                     5     AMR 7.95   75        159
                     6     AMR 10.2   65        204
                     7     AMR 12.2   81        244
                     8     AMR SID    39         39

           Table 1.  The number of class A bits for the AMR codec.

Asterisk's internal AMR format:
===============================

Asterisk internally use the "octed-aligned" RTP format in ast_frame.
(see section 4.4 in RFC 3267)
This allows to have multiple AMR frames in one Asterisk frame. This
means, the payload of an ast_frame wich contains N AMR frames consists
of (se also section 4.4.5.1 of RFC 3267):

   1. 1 byte CMR (codec mode request)
   2. N byte TOC (the TOC contains the AMR mode of the respecitve frame
      and one bit which tells us if it is the last TOC or if there are
      some more)
   3. blocksize(AMR frame 0) + ..... + blocksize(AMR frame N)

We define the blocksize of an AMR frame os the number of bytes needed
to contain an AMR frame in the respective mode. Thus, it is the number
of total speech bits divided by 8 and rounded upwards.
E.g. an AMR frame in mode 5 has 159 bits. To store this frame we need
20 bytes. Thus, the blocksize of an AMR frame in mode 5 is 20 bytes.

H324M AMR format:
=================
In H324M, the AMR frames received from are in if2 format from libh324m.
(see Annex A in TS 26.101). This means that there are 4 bits for the AMR
mode followed by the speech bits. E.g. an AMR frame in mode 5 in if format
needs 159+4 => 21 bytes.
There is always only one AMR frame in an if2 packet.

Extended Table 1:
==================

                         Class A   total speech   block   if2 frame
       Index   Mode       bits       bits          size     size
       -------------------------------------------------------------
         0     AMR 4.75   42         95            12        13
         1     AMR 5.15   49        103            13        14
         2     AMR 5.9    55        118            15        16
         3     AMR 6.7    58        134            17        18
         4     AMR 7.4    61        148            19        19
         5     AMR 7.95   75        159            20        21
         6     AMR 10.2   65        204            26        26
         7     AMR 12.2   81        244            31        31
         8     AMR SID    39         39             5         6

*/



static short blockSize[16] = { 12, 13, 15, 17, 19, 20, 26, 31,  5, -1, -1, -1, -1, -1, -1, -1};
static short if2stuffing[16] = {5,  5,  6,  6,  0,  5,  0,  0,  5,  1,  6,  7, -1, -1, -1,  4};

/* 1st dummy AMR-SID frame (comfort noise) */
static unsigned char last_amr_sti[6] = { 0x78, 0x46, 0x00, 0x94, 0xA4, 0x07 };

struct video_creator
{
	struct timeval tv;
	struct timeval tvnext;
	unsigned int samples;
	unsigned char first;
	unsigned char buffer[PKT_PAYLOAD];
	unsigned int bufferLength;
	unsigned char frame[PKT_SIZE];
};

static struct ast_frame* create_ast_frame(void *frame, struct video_creator *vt)
{
	int mark = 0;
	unsigned int i = 0;
	short j = 0;
	int found = 0;
	unsigned int len = 0;
	struct ast_frame* send;
	unsigned char* data = 0;

	/* Get data & size */
	unsigned char * framedata = FrameGetData(frame);
	unsigned int framelength = FrameGetLength(frame);

	/* Get frame */
	send = (struct ast_frame *) vt->frame;

	/* Clear */
	memset(send,0,PKT_SIZE);

	/* Set data */
	AST_FRAME_SET_BUFFER(send,send,PKT_OFFSET,0);
	/* Get Data pointer */
	data = AST_FRAME_GET_BUFFER(send);

	/* Depending on the type */
	switch(FrameGetType(frame))
	{
		case MEDIA_AUDIO:
			/*Check it's AMR */
			if (FrameGetCodec(frame)!=CODEC_AMR)
				/* exit */
				return NULL;

			ast_log(LOG_DEBUG, "create_ast_frame: received AMR frame with %d bytes\n",framelength);

			/*Get header*/
			unsigned char header = framedata[0];
			/*Get mode*/
			unsigned char mode = header & 0x0F;
	
			/* Check fom AMR No-Data packe */	
			if (mode==15) 
			{ 
				/* AMR No-Data packet --> replace with last AMR-SID */
				framelength = 6;
				framedata = last_amr_sti;     			        
			}
			
			/*Get number of stuffing bits*/
			unsigned int stuf = if2stuffing[mode];

			/*Get Block Size*/
			short bs = blockSize[mode];

			/* Check correct mode and length */
			if (bs==-1 || framelength<(unsigned)bs)
				/* Exit */
				return NULL;

			/* Set data len*/
			send->datalen = framelength + 1; /* +1 because the the octet with the CMR */

			/* Set header cmr */
			data[0] = 0xF0;
			/* Increase pointer to match frame */
			data++;
			/* Copy */
			memcpy(data, framedata, framelength);
			
			/*Convert IF2 into AMR MIME format*/

			/*Reverse bytes*/
			TIFFReverseBits(data, framelength);

			/*If amr has a byte more than if2 */
			if(stuf < 4)
			{
				/* Set last byte */
				data[bs] = data[bs - 1] << 4;
				/*Increase size of frame*/
				send->datalen++;
			} else {
				/* Set last byte */
				data[bs] = data[bs] >> 4 | data[bs-1] << 4;
			}
			
			/* For each byte */
			for(j=bs-1; j>0; j--)
				data[j] = data[j] >> 4 | data[j-1] << 4;

			/* Calculate first byte */
			data[0] = mode << 3 | 0x04;

			/* Set video type */
			send->frametype = AST_FRAME_VOICE;
			/* Set codec value */
			send->subclass = AST_FORMAT_AMRNB;
			/* Rest of values*/
			send->src = "h324m";
			send->samples = 160;
			send->delivery.tv_usec = 0;
			send->delivery.tv_sec = 0;
			/* Don't free */
			send->mallocd = 0;
			/* Send */
			return send;
		case MEDIA_VIDEO:
			/*Check it's H263 */
			if (FrameGetCodec(frame)!=CODEC_H263)
				/* exit */
				return NULL;
			/* Search from the begining */
			i = 0;
			found = 0;

			/* Check length*/
			if(framelength>3)
			{
				/* Try to find begining of frame */
				while (!found && i<framelength-4)
				{
					/* Check if we found start code */
					if (framedata[i] == 0 && framedata[i+1] == 0  &&  (framedata[i+2] & 0xFC) == 0x80)
						/* We found it */
						found = 1;
					else
						/* Increase pointer */
						i++;
				}
			}

			/* If still in the same frame */
			if (found)
			{
				/* Send what was on the buffer plus the beggining of the packet*/
				len = vt->bufferLength + i;
				/* Last packet */
				mark = 1;
				/* Update recived ts */
				vt->tvnext = ast_tvnow();
			} else {
				/* Send only what was on the buffer */
				len =  vt->bufferLength;
				/* Not last packet */
				mark = 0;
			}
				
			/* if its first pcaket of a frame */
			if (vt->first)
			{
				/* If it's not empty */
				if (vt->bufferLength)
				{
					/* Set data len */
					send->datalen = vt->bufferLength;
					/* Copy */
					memcpy(data+2, vt->buffer+2, vt->bufferLength-2);
				} else {
					/* Only header part by bow */
					send->datalen = 2;
				}
				/* Set header */
				data[0] = 0x04;
				data[1] = 0x00; 
			} else {
				/* Set data len */
				send->datalen =  vt->bufferLength + 2  ;
				/* If it's not empty */
				if (vt->bufferLength)
					/* Copy */
					memcpy(data+2, vt->buffer, vt->bufferLength);
				/* Set header */
				data[0] = 0x00;
				data[1] = 0x00;
			}
			
			/* Assertion test */
			if (i>framelength)
			{
				/* Never should happen */
				ast_log(LOG_ERROR, "Counter past of frame [%d,%d]\n",i,framelength);
				/* Empty */
				vt->bufferLength = 0;
			/* If we have to send the begging of this frame */
			} else if (i>0 && found) {
				/* Copy the begining to the packet to send*/
				memcpy(data+send->datalen,framedata,i);
				/* Increase size */
				send->datalen += i;
				/* Copy the rest to the buffer */
				memcpy(vt->buffer,framedata+i,framelength-i);
				/* Set buffer length */
				vt->bufferLength = framelength-i;
			} else {
				/* Copy the whole packet to the buffer */
				memcpy(vt->buffer,framedata,framelength);
				/* Set buffer length */
				vt->bufferLength = framelength;
			}

			/* Set video type */
			send->frametype = AST_FRAME_VIDEO;
			/* Set codec value */
			send->subclass = AST_FORMAT_H263_PLUS | mark;
			/* Set samples for the first packet of the frame */
			if (vt->first)
			       	send->samples = vt->samples;
			else
				send->samples = 0;	
			/* Rest of values*/
			send->src = "h324m";
			send->delivery.tv_usec = 0;
			send->delivery.tv_sec = 0;
			/* Don't free */
			send->mallocd = 0;

			/* If the next packet is from a different frame */
			if (mark)
			{
				/* Calculate ms */
				int ms = 0;
				/* If it's not first*/
				if (!ast_tvzero(vt->tv))
					/* Get the difference in ms */
					ms = ast_tvdiff_ms(vt->tvnext,vt->tv);
				/* Change tr */
				vt->tv = vt->tvnext;
				/* Update samles */
				vt->samples = ms*90;
				/* Set it's the first */
				vt->first = 1;
			} else {
				/* Next it's not first */
				vt->first = 0;
				/* Reset samles */
				vt->samples = 0;
			}
			/* Send */
			return send;
	}

	/* NOthing */
	return NULL;
}

struct h324m_packetizer
{
	unsigned char *framedata;
	unsigned char *offset;
        int framelength;
	int num;
	int max;
};

static int init_h324m_packetizer(struct h324m_packetizer *pak,struct ast_frame* f)
{
	int i;

	/* Empty data */
	memset(pak,0,sizeof(struct h324m_packetizer));

	/* Depending on the type */
	switch (f->frametype)
	{
		case AST_FRAME_VOICE:
			/* Check audio type */
			if (!(f->subclass & AST_FORMAT_AMRNB))
				/* exit */
				return 0;
			/* Get data & length */
			pak->framedata = AST_FRAME_GET_BUFFER(f);
			pak->framelength = f->datalen;
			/* Read toc until no mark found, skip first byte */
			while ((++pak->max < pak->framelength) && (pak->framedata[pak->max] & 0x80)) {}
			/* Check lenght */
			if (pak->max >= pak->framelength)
				/* Exit */	
				return 0;
			ast_log(LOG_DEBUG, "init_h324m_packetizer: found %d AMR frames inside ast_frame\n",pak->max);
			/* Set offset */
			pak->offset = pak->framedata + pak->max + 1; /* +1 because of CMR octet */
			/* Move toc to the beginning so we can overwrite the byte before the frame */
			/* This overwrites the CMR but it is not needed at all */
			for (i=0;i < pak->max;i++)
				/* copy */
				pak->framedata[i] = pak->framedata[i+1];
			/* Good one */
			return 1;
		case AST_FRAME_VIDEO:
			/* Depending on the codec */
			if (f->subclass & AST_FORMAT_H263) 
			{
				/* Get data & length without rfc 2190 (only A packets ) */
				pak->framedata = AST_FRAME_GET_BUFFER(f)+4;
				pak->framelength = f->datalen-4;
			} else if (f->subclass & AST_FORMAT_H263_PLUS) {
				/* Get initial data */
				pak->framedata = AST_FRAME_GET_BUFFER(f);
				pak->framelength = f->datalen;
				/* Get header */
				unsigned char p = pak->framedata[0] & 0x04;
				unsigned char v = pak->framedata[0] & 0x02;
				unsigned char plen = ((pak->framedata[0] & 0x1 ) << 5 ) | (pak->framedata[1] >> 3);
				unsigned char pebit = pak->framedata[0] & 0x7;
				/* skip header*/
				pak->framedata += 2+plen;
				pak->framelength -= 2+plen;
				/* Check */
				if (v)
				{
					/* Increase ini */
					pak->framedata++;
					pak->framelength--;
				}
				/* Check p bit */
				if (p)
				{
					/* Decrease ini */
					pak->framedata -= 2;
					pak->framelength += 2;
					/* Append 0s */	
					pak->framedata[0] = 0;
					pak->framedata[1] = 0;
				}
			} else
				break;
			/* Only 1 packet */
			pak->max = 1;
			/* Exit */
			return 1;
		default:
			/* dummy statement to make compiler happy */
			;
	}
	/* Nothing to do */
	return 0;
}

static void* create_h324m_frame(struct h324m_packetizer *pak,struct ast_frame* f)
{
	int i = 0;

	/* if not more */
	if (pak->num == pak->max) {
		/* Exit */
		return NULL;
	}
	pak->num++;
	ast_log(LOG_DEBUG, "create_h324m_frame: processing AMR frame #%d inside ast_frame\n",pak->num);

	/* Depending on the type */
	switch (f->frametype)
	{
		case AST_FRAME_VOICE:
			/* Check audio type */
			if (!(f->subclass & AST_FORMAT_AMRNB))
				/* exit */
				break;
			/* Convert to if2 */
			/* Get header: pak->framedata starts with ToC as CMR was 
			   overwritten in init_h324m_packetizer() */
			unsigned char header = pak->framedata[pak->num-1];
			/* Get mode */
			unsigned char mode = (header >> 3 ) & 0x0f;
			/* Get blockSize */
			signed bs = blockSize[mode];
			if (bs < 0) {
				ast_log(LOG_DEBUG, "create_h324m_frame: error decoding AMR structure - AMR frame #%d has block size %d\n",
					pak->num,bs);
				/* exit */
				break;
			}
			/*Get Stuffing bits*/
			int stuf = if2stuffing[mode];

			ast_log(LOG_DEBUG, "create_h324m_frame: Creatubg frame bs %d,stuf %di,mode %d\n",bs,stuf,mode);

			if (pak->offset + bs > pak->framedata + pak->framelength) {
				ast_log(LOG_DEBUG, "create_h324m_frame: error decoding AMR structure - block exceeds buffer\n");
				ast_log(LOG_DEBUG, "create_h324m_frame: pak->offset=%p (%d);bs=%d, pak->framedata=%p,pak->framelength=%d\n",
					pak->offset,(pak->framedata-pak->offset),bs,pak->framedata,pak->framelength);

				/* exit */
				break;
			}

			/*If amr and if2 frames has same size*/
			if(!(stuf < 4))
				pak->offset[bs - 1] = 0x00;
			
			/* For each byte */
			for (i=bs-1; i>0; i--)
				/*Move bits*/
				pak->offset[i] = (pak->offset[i] >> 4) | (pak->offset[i-1] <<  4);
			
			/*Set first*/
			pak->offset[0] = pak->offset[0] >> 4;
			/*Reverse bits*/
			TIFFReverseBits(pak->offset, bs);
			/*Set mode*/
			pak->offset[0] |= mode;
			/* Inc offset first */
			pak->offset += bs;
			/* Create frame */	
			return FrameCreate(MEDIA_AUDIO, CODEC_AMR, pak->offset - bs, bs);
		case AST_FRAME_VIDEO:
			/* Create frame */
			return FrameCreate(MEDIA_VIDEO, CODEC_H263,
pak->framedata, pak->framelength); default:
			break;
	}
	/* NOthing */
	return NULL;
}

static int app_h324m_loopback(struct ast_channel *chan, void *data)
{
	struct ast_frame *f;
	struct ast_module_user *u;
	void*  frame;
	int loop_audio=1, loop_video=1;

	ast_log(LOG_DEBUG, "h324m_loopback\n");

	/* Lock module */
	u = ast_module_user_add(chan);

	/* Check input paramaters */
	if (strchr(data,'a')) 
		/* deactivate audio loopback */
		loop_audio=0;

	/* Check input paramaters */
	if (strchr(data,'v'))
		/* deactivate video loopback */
		loop_video=0;

	/* Create session */
	void* id = H324MSessionCreate();

	/* Init session */
	H324MSessionInit(id);

	/* Wait for data avaiable on channel */
	while (ast_waitfor(chan, -1) > -1) 
	{
		/* Read frame from channel */
		f = ast_read(chan);

		/* if it's null */
		if (f == NULL)
			break;

		/* Check frame type */
		if (f->frametype == AST_FRAME_VOICE) 
		{
			/* read data */
			H324MSessionRead(id, AST_FRAME_GET_BUFFER(f), f->datalen);
			/* Get frames */
			while ((frame=H324MSessionGetFrame(id))!=NULL)
			{
				if (FrameGetType(frame)==MEDIA_VIDEO) 
				{
					/* If video loopback is activated */
					if (loop_video) 
						/* Send it back */
						H324MSessionSendFrame(id,frame);

				} else if (FrameGetType(frame)==MEDIA_AUDIO) {
					/* If audio loopback is activated */
					if (loop_audio)
						/* Send it back. Note: this can cause loopback/echo problems */
						H324MSessionSendFrame(id,frame);
				}
				/* Delete frame */
				FrameDestroy(frame);
			}
			/* write data */
			H324MSessionWrite(id, AST_FRAME_GET_BUFFER(f), f->datalen);
			/* deliver now */
			f->delivery.tv_usec = 0;
			f->delivery.tv_sec = 0;
			/* write frame */
			ast_write(chan, f);
		} 
	}

	/* Destroy session */
	H324MSessionEnd(id);

	/* Destroy session */
	H324MSessionDestroy(id);

	ast_log(LOG_DEBUG, "exit");

	/* Unlock module*/
	ast_module_user_remove(u);

	//Exit
	return 0;
}

static int app_h324m_gw(struct ast_channel *chan, void *data)
{
	struct ast_frame *f;
	struct ast_frame *send;
	struct ast_module_user *u;
	struct h324m_packetizer pak;
	struct video_creator vt;
	void*  frame;
	char*  input;
	char*  src = 0;
	int    reason = 0;
	int    state = 0;
	int    ms;
	struct ast_channel *channels[2];
	struct ast_channel *pseudo;
	struct ast_channel *where;

	/* Initial values of vt */
	vt.bufferLength = 0;
	vt.samples = 0;
	vt.tv.tv_sec = 0;
	vt.tv.tv_usec = 0;
	vt.tvnext.tv_sec = 0;
	vt.tvnext.tv_usec = 0;
	vt.first = 1;

	ast_log(LOG_DEBUG, "h324m_gw\n");

	/* Lock module */
	u = ast_module_user_add(chan);

	/* Request new channel */
	pseudo = ast_request("Local", AST_FORMAT_H263 | AST_FORMAT_H263_PLUS | AST_FORMAT_AMRNB , data, &reason);
 
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

	/* Set up array */
	channels[0] = chan;
	channels[1] = pseudo;

	/* No timeout */
	ms = -1;

	/* while not setup */
	while (pseudo->_state!=AST_STATE_UP) 
	{
		/* Wait for data */
		if ((where = ast_waitfor_n(channels, 2, &ms))<0)
			/* error, timeout, or done */
			break;
		/* Read frame */
		f = ast_read(where);
		/* If not frame */
		if (!f)
			/* done */ 
			break;
		/* Check channel */
		if (where==pseudo)
		{
			/* If it's a control frame */
			if (f->frametype == AST_FRAME_CONTROL) 
			{
				/* Dependinf on the event */
				switch (f->subclass) 
				{
					case AST_CONTROL_RINGING:
						ast_indicate(chan, AST_CONTROL_RINGING);
						break;
					case AST_CONTROL_BUSY:
					case AST_CONTROL_CONGESTION:
						/* Delete frame */
						ast_frfree(f);
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
		} else {
			/* If it's a control frame */
			if (f->frametype == AST_FRAME_CONTROL) 
			{
				/* Depending on the event */
				switch (f->subclass) 
				{
					case AST_CONTROL_HANGUP:
						/* Delete frame */
						ast_frfree(f);
						/* Save cause */
						reason = pseudo->hangupcause;
						/* exit */
                                                goto hangup_pseudo;
                                                break;
				}
			}
		}
		/* Delete frame */
		ast_frfree(f);
	}

	/* If no answer */
	if (pseudo->_state != AST_STATE_UP)
		/* goto end */
		goto clean_pseudo; 

	/* Create session */
	void* id = H324MSessionCreate();

	/* Init session */
	H324MSessionInit(id);

	/* Answer call */
	ast_answer(chan);

	/* Wait for data avaiable on any channel */
	while (!reason && (where = ast_waitfor_n(channels, 2, &ms)) != NULL) 
	{
		/* Read frame from channel */
		f = ast_read(where);

		/* if it's null */
		if (f == NULL)
			break;

		/* If it's on h324m channel */
		if (where==chan) 
		{
			/* Check frame type */
			if ((f->frametype == AST_FRAME_DIGITAL) || (f->frametype == AST_FRAME_VOICE)) 
			{
				/* read data */
				H324MSessionRead(id, AST_FRAME_GET_BUFFER(f), f->datalen);
				/* If state changed */
				if (state!=H324MSessionGetState(id))
				{
					/* Update state */
					state = H324MSessionGetState(id);

					/* Log */
					ast_log(LOG_DEBUG, "H324M changed state %d\n", state);
					
					/* If connected */	
					if (state==CALLSTATE_STABLISHED)
					{
						/* Log */
						ast_log(LOG_DEBUG, "Connected, sending VIDUPDATE\n");
						/* Indicate Video Update */
						ast_indicate(pseudo, AST_CONTROL_VIDUPDATE);
					}
				}
				/* Get frames */
				while ((frame=H324MSessionGetFrame(id))!=NULL)
				{
					/* Packetize outgoing frame */
					if ((send=create_ast_frame(frame,&vt))!=NULL)
						/* Send frame */
						ast_write(pseudo,send);
					/* Delete frame */
					FrameDestroy(frame);
				}
				/* Get user input */
				while((input=H324MSessionGetUserInput(id))!=NULL)
				{
					/* Send digit begin */
					ast_senddigit_begin(pseudo,input[0]);
					/* Send digit end */
					ast_senddigit_end(pseudo,input[0],100);
					/* free data */
					free(input);
				}

				/* write data */
				H324MSessionWrite(id, AST_FRAME_GET_BUFFER(f), f->datalen);
				/* deliver now */
				f->delivery.tv_usec = 0;
				f->delivery.tv_sec = 0;
				/* write frame */
				ast_write(chan, f);

			} else if (f->frametype == AST_FRAME_CONTROL) {
				/* Check for hangup */
				if (f->subclass == AST_CONTROL_HANGUP)
					/* exit */
					reason = AST_CAUSE_NORMAL_CLEARING;
			}

			/* Delete frame */
			ast_frfree(f);
		} else {
			/* Check type */
			if (f->frametype == AST_FRAME_CONTROL) 
			{
				/* Check subtype */
				switch(f->subclass)
				{
					case AST_CONTROL_HANGUP:
						/* exit */
						reason = AST_CAUSE_NORMAL_CLEARING;
						break;
					case AST_CONTROL_VIDUPDATE:
						/* Send it */
						H324MSessionSendVideoFastUpdatePicture(id);
						break;
					default:
						break;
				}

			} else if (f->frametype == AST_FRAME_DTMF) {
				/* DTMF */

			} else {
				/* Check src: only use one type of AST_FRAME for src change detection
				 * as video and voice may have different src (e.g. video-src="RTP" 
				 * and audio-src="lintoamr")
				 */
				if (f->frametype == AST_FRAME_VIDEO) 
				{
					if (!src && f->src) {
						/* Store it */
						src = strdup(f->src);
					} else if (src && !f->src) {
						/* Delete old one */
						free(src);
						/* Store it */
						src = NULL;
						/* Reset media */
						H324MSessionResetMediaQueue(id);
					} else if (src && f->src && strcmp(src,f->src)!=0) {
						/* Delete old one */
						free(src);
						/* Store it */
						src = strdup(f->src);
						/* Reset media */
						H324MSessionResetMediaQueue(id);
					}
				}
				/* Init packetizer */
				if (init_h324m_packetizer(&pak,f))
					/* Create frame */
					while ((frame=create_h324m_frame(&pak,f))!=NULL) {
						/* Send frame */
						H324MSessionSendFrame(id,frame);
						/* Delete frame */
						FrameDestroy(frame);
					}
			}
			/* Delete frame */
			ast_frfree(f);
		}
	}

	/* End session */
	H324MSessionEnd(id);

	/* Destroy session */
	H324MSessionDestroy(id);

hangup_pseudo:
	/* Hangup pseudo channel if needed */
	ast_softhangup(pseudo, reason);

clean_pseudo:
	/* Destroy pseudo channel */
	ast_hangup(pseudo);

end:
	/* Hangup channel if needed */
	ast_softhangup(chan, reason);

	/* Unlock module*/
	ast_module_user_remove(u);

	/* Free src */	
	if (src) 
		free(src);

	/*Exit*/
	return -1;
}

static int app_h324m_call(struct ast_channel *chan, void *data)
{
	struct ast_frame *f;
	struct ast_frame *send;
	struct ast_module_user *u;
	struct h324m_packetizer pak;
	struct video_creator vt;
	void*  frame;
	char*  input;
	int    reason = 0;
	int    state = 0;
	int    ms;
	struct ast_channel *channels[2];
	struct ast_channel *pseudo;
	struct ast_channel *where;

	/* Initial values of vt */
	vt.bufferLength = 0;
	vt.samples = 0;
	vt.tv.tv_sec = 0;
	vt.tv.tv_usec = 0;
	vt.tvnext.tv_sec = 0;
	vt.tvnext.tv_usec = 0;
	vt.first = 1;

	ast_log(LOG_DEBUG, "h324m_call\n");

	/* Lock module */
	u = ast_module_user_add(chan);

	/* we only do support AMR as voice codec - thus make sure that
	   Asterisk's core transcode voice frames to/from AMR */
	if (ast_set_read_format(chan, AST_FORMAT_AMRNB))
		ast_log(LOG_WARNING, "app_h324m_call: Unable to set read format to AMR-NB!\n");
	if (ast_set_write_format(chan, AST_FORMAT_AMRNB))
		ast_log(LOG_WARNING, "app_h324m_call: Unable to set read format to AMR-NB!\n");

	/* Request new channel */
	/* sometimes Asterisk uses internally a differnt LAW then chan_zap/zaptel and
	 * performs ALAW/ULAW conversion. Is is deadly as we transmit digital data inside
	 * LAW-frames (we have to do this as Asterisk does not support digital ISDN calls).
	 *
	 * If you have problems on outgoing 3G calls please specify exactly the LAW used 
	 * by your ISDN line. Usually in Europe you have ALAW, in USA ULAW.
	 *
	 * Example for Austria(Europe):
	 *     pseudo = ast_request("Local", AST_FORMAT_ALAW , data, &reason);
	 */
	pseudo = ast_request("Local", AST_FORMAT_ALAW , data, &reason);
 
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

	/* Set up array */
	channels[0] = chan;
	channels[1] = pseudo;

	/* No timeout */
	ms = -1;

	/* while not setup */
	while (pseudo->_state!=AST_STATE_UP) 
	{
		/* Wait for data */
		if ((where = ast_waitfor_n(channels, 2, &ms))<0)
			/* error, timeout, or done */
			break;
		/* Read frame */
		f = ast_read(where);
		/* If not frame */
		if (!f)
			/* done */ 
			break;
		/* Check channel */
		if (where==pseudo)
		{
			/* If it's a control frame */
			if (f->frametype == AST_FRAME_CONTROL) 
			{
				/* Depending on the event */
				switch (f->subclass) 
				{
					case AST_CONTROL_RINGING:
						ast_indicate(chan, AST_CONTROL_RINGING);
						break;
					case AST_CONTROL_BUSY:
						ast_log(LOG_DEBUG, "h324m_call: pseudo channel: BUSY\n");
						ast_log(LOG_DEBUG, "h324m_call: pseudo channel: hangupcause=%d\n",pseudo->hangupcause);
						/* Delete frame */
						ast_frfree(f);
						/* Save cause */
						reason = AST_CAUSE_BUSY;
						/* exit */
						goto hangup_pseudo;
						break;
					case AST_CONTROL_CONGESTION:
						ast_log(LOG_DEBUG, "h324m_call: pseudo channel: CONGESTION\n");
						ast_log(LOG_DEBUG, "h324m_call: pseudo channel: hangupcause=%d\n",pseudo->hangupcause);
						/* Delete frame */
						ast_frfree(f);
						/* Save cause */
						reason = AST_CAUSE_CONGESTION;
						/* exit */
						goto hangup_pseudo;
						break;
					case AST_CONTROL_ANSWER:
						/* Set UP*/
						reason = 0;	
						break;
					default:
						reason = pseudo->hangupcause;
				}
			}
		} else {
			/* If it's a control frame */
			if (f->frametype == AST_FRAME_CONTROL) 
			{
				/* Depending on the event */
				switch (f->subclass) 
				{
					case AST_CONTROL_HANGUP:
						/* Delete frame */
						ast_frfree(f);
						/* Save cause */
						reason = pseudo->hangupcause;
						/* exit */
                                                goto hangup_pseudo;
                                                break;
				}
			}
		}
		/* Delete frame */
		ast_frfree(f);
	}

	/* If no answer */
	if (pseudo->_state != AST_STATE_UP)
	{	ast_log(LOG_DEBUG, "h324m_call: pseudo channel not up -> hangup\n");
		/* goto end */
		goto clean_pseudo; 
	}

	/* Create session */
	void* id = H324MSessionCreate();

	/* Init session */
	H324MSessionInit(id);
	/* Create enpty packet */
	send = (struct ast_frame *) malloc(PKT_SIZE);
	/* Set frame data buffer */
	AST_FRAME_SET_BUFFER(send,send,PKT_OFFSET,160);
	/* Set DTMF type */
	send->frametype = AST_FRAME_VOICE;
	/* Set DTMF value */
	send->subclass = pseudo->rawwriteformat;
	/* Rest of values*/
	send->src = 0;
	send->samples = 160;
	send->delivery.tv_usec = 0;
	send->delivery.tv_sec = 0;
	/* We will free the frame */
	send->mallocd = 0;
	/* Send */
	ast_write(pseudo,send);

	/* Wait for data avaiable on any channel */
	while (!reason && (where = ast_waitfor_n(channels, 2, &ms)) != NULL) 
	{
		/* Read frame from channel */
		f = ast_read(where);

		/* if it's null */
		if (f == NULL)
			break;

		/* If it's on h324m channel */
		if (where==pseudo) 
		{
			/* Check frame type */
			if (f->frametype == AST_FRAME_VOICE) 
			{
				/* read data */
				H324MSessionRead(id, AST_FRAME_GET_BUFFER(f), f->datalen);

				/* If state changed */
				if (state!=H324MSessionGetState(id))
				{
					/* Update state */
					state = H324MSessionGetState(id);

					/* Log */
					ast_log(LOG_DEBUG, "H324M changed state %d\n", state);
					
					/* If connected */	
					if (state==CALLSTATE_STABLISHED)
					{
						/* Answer call if not done yet */
						ast_answer(chan);
						/* Log */
						ast_log(LOG_DEBUG, "Connected, sending VIDUPDATE\n");
						/* Indicate Video Update */
						ast_indicate(pseudo, AST_CONTROL_VIDUPDATE);
					}
				}
				/* Get frames */
				while ((frame=H324MSessionGetFrame(id))!=NULL)
				{
					/* Packetize outgoing frame */
					if ((send=create_ast_frame(frame,&vt))!=NULL)
						/* Send frame */
						ast_write(chan,send);
					/* Delete frame */
					FrameDestroy(frame);
				}
				/* Get user input */
				while((input=H324MSessionGetUserInput(id))!=NULL)
				{
					/* Send digit begin */
					ast_senddigit_begin(chan,input[0]);
					/* Send digit end */
					ast_senddigit_end(chan,input[0],100);
					/* free data */
					free(input);
				}

				/* write data */
				H324MSessionWrite(id, AST_FRAME_GET_BUFFER(f), f->datalen);
				/* deliver now */
				f->delivery.tv_usec = 0;
				f->delivery.tv_sec = 0;
				/* write frame */
				ast_write(pseudo, f);
			} else if (f->frametype == AST_FRAME_CONTROL) {
				/* Check for hangup */
				if (f->subclass == AST_CONTROL_HANGUP) 
					/* exit */
					reason = AST_CAUSE_NORMAL_CLEARING;
			} else 
				/* Delete frame */
				ast_frfree(f);
		} else {
			/* Check frame type DTMF*/
			if (f->frametype == AST_FRAME_DTMF) 
			{
				char dtmf[2];
				/* Get DTMF */
				dtmf[0] = f->subclass;
				dtmf[1] = 0;
				/* Send DTMF */
				H324MSessionSendUserInput(id,dtmf);	
			/* Check control channel */
			} else if (f->frametype == AST_FRAME_CONTROL) {
				/* Check for hangup */
				if (f->subclass == AST_CONTROL_HANGUP)
					/* exit */
					reason = AST_CAUSE_NORMAL_CLEARING;
				/* Init packetizer */
			} else if (init_h324m_packetizer(&pak,f)) {
				/* Create frame */
				while ((frame=create_h324m_frame(&pak,f))!=NULL) {
					/* Send frame */
					H324MSessionSendFrame(id,frame);
					/* Delete frame */
					FrameDestroy(frame);
				}
			}
			/* Delete frame */
			ast_frfree(f);
		}
	}

	/* End session */
	H324MSessionEnd(id);

	/* Destroy session */
	H324MSessionDestroy(id);

hangup_pseudo:
	/* Hangup pseudo channel if needed */
	ast_softhangup(pseudo, reason);

clean_pseudo:
	/* Destroy pseudo channel */
	ast_hangup(pseudo);

end:
	/* Hangup channel if needed */
//	ast_softhangup(chan, reason);

	/* Unlock module*/
	ast_module_user_remove(u);

	//Exit
//	return -1;
	chan->hangupcause = reason;
	return 0;
}

static int app_h324m_gw_answer(struct ast_channel *chan, void *data)
{
	struct ast_frame *f;
	struct ast_module_user *u;

	ast_log(LOG_DEBUG, ">h324m_gw_answer\n");

	/* Lock module */
	u = ast_module_user_add(chan);

	/* Answer channel */
	ast_answer(chan);

	/* Check it's up */
	if (chan->_state!=AST_STATE_UP)
		/* Exit */
		return -1;

	/* Wait for vidupdate*/
	while (ast_waitfor(chan, -1)>-1) 
	{
		/* Read frame from channel */
		f = ast_read(chan);

		/* if it's null */
		if (f == NULL)
			break;

		/* Check frame type */
		if (f->frametype == AST_FRAME_CONTROL) 
		{
			/* Check subtype */
			switch(f->subclass)
			{
				case AST_CONTROL_HANGUP:
					/* Log */
					ast_log(LOG_DEBUG, "<h324m_gw_answer on HANGUP\n");
					/* Free frame */
					ast_frfree(f);
					/* exit & Hang up */
					return -1;
				case AST_CONTROL_VIDUPDATE:
					ast_log(LOG_DEBUG, "<h324m_gw_answer on VIDUPDATE\n");
					/* Free frame */
					ast_frfree(f);
					/* Exit & continue */
					return 0;
				default:
					break;
			}
		}
		/* Free frame */
		ast_frfree(f);
	}

	ast_log(LOG_DEBUG, "<h324m_gw_answer\n");
	
	/* Exit */
	return -1;
		
}

static int app_video_loopback(struct ast_channel *chan, void *data)
{
	struct ast_frame *f;
	struct ast_module_user *u;

	ast_log(LOG_DEBUG, "video_loopback\n");

	/* Lock module */
	u = ast_module_user_add(chan);

	/* Wait for data avaiable on channel */
	while (ast_waitfor(chan, -1) > -1) 
	{
		/* Read frame from channel */
		f = ast_read(chan);

		/* if it's null */
		if (f == NULL)
			break;

		/* Check frame type */
		if (f->frametype == AST_FRAME_VIDEO) 
		{
			/* deliver now */
			f->delivery.tv_usec = 0;
			f->delivery.tv_sec = 0;
			/* write frame */
			ast_write(chan, f);
		} 
	}

	ast_log(LOG_DEBUG, "exit");

	/* Unlock module*/
	ast_module_user_remove(u);

	/* Exit */
	return 0;
}

static int unload_module(void)
{
	int res;

	res = ast_unregister_application(name_h324m_loopback);
	res &= ast_unregister_application(name_h324m_gw);
	res &= ast_unregister_application(name_h324m_call);
	res &= ast_unregister_application(name_h324m_gw_answer);
	res &= ast_unregister_application(name_video_loopback);

	ast_module_user_hangup_all();

	ast_cli_unregister(&cli_debug);

	return res;
}

static int h324m_log_asterisk_callback(const char* fmt, va_list vl)
{
	char msg[1024];

	vsnprintf(msg,1024,fmt,vl);

	ast_log(LOG_DEBUG, msg);

	return 1;
}


static int load_module(void)
{
	int res;

	res = ast_register_application(name_h324m_loopback, app_h324m_loopback, syn_h324m_loopback, des_h324m_loopback);
	res &= ast_register_application(name_h324m_gw, app_h324m_gw, syn_h324m_gw, des_h324m_gw);
	res &= ast_register_application(name_h324m_call, app_h324m_call, syn_h324m_call, des_h324m_call);
	res &= ast_register_application(name_h324m_gw_answer, app_h324m_gw_answer, syn_h324m_gw_answer, des_h324m_gw_answer);
	res &= ast_register_application(name_video_loopback, app_video_loopback, syn_video_loopback, des_video_loopback);

	ast_cli_register(&cli_debug);
	
	/* No loging by default */
	H324MLoggerSetLevel(1);
	/* Set logger function */
	H324MLoggerSetCallback(h324m_log_asterisk_callback);

	return 0;
}

AST_MODULE_INFO_STANDARD(ASTERISK_GPL_KEY, "H324M stack");

