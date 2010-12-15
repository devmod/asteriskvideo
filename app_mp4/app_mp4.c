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
 * \brief MP4 application -- save and play mp4 files
 *
 * \ingroup applications
 */

#include <asterisk.h>
#include <mp4.h> 
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
#include <asterisk/options.h>
#include <asterisk/config.h>
#include <asterisk/utils.h>
#include <asterisk/app.h>
#include <asterisk/version.h>

#ifndef AST_FORMAT_AMRNB
#define AST_FORMAT_AMRNB	(1 << 13)
#endif

#if ASTERISK_VERSION_NUM>10600
#define AST_FRAME_GET_BUFFER(fr)        ((unsigned char*)((fr)->data.ptr))
#else
#define AST_FRAME_GET_BUFFER(fr)        ((unsigned char*)((fr)->data))
#endif


#define PKT_PAYLOAD	1470
#define PKT_SIZE 	(sizeof(struct ast_frame) + AST_FRIENDLY_OFFSET + PKT_PAYLOAD)
#define PKT_OFFSET	(sizeof(struct ast_frame) + AST_FRIENDLY_OFFSET)

static const char *app_play = "mp4play";
static const char *syn_play = "MP4 file playblack";
static const char *des_play = "  mp4play(filename,[options]):  Play mp4 file to user. \n"
        "\n"
        "Available options:\n"
        " 'n(x)': number of digits (x) to wait for \n"
        " 'S(x)': set variable of name x (with DTMFs) rather than go to extension \n"
        " 's(x)': set digits, which should stop playback \n"
        "\n"
        "Examples:\n"
        " mp4play(/tmp/video.mp4)   					play video file to user\n"
        " mp4play(/tmp/test.mp4,'n(3)') 				play video file to user and wait for 3 digits \n"
        " mp4play(/tmp/test.mp4,'n(3)S(DTMF_INPUT)')	play video file to user and wait for 3 digits and \n"
        "							set them as value of variable DTMF_INPUT\n"
        " mp4play(/tmp/test.mp4,'n(3)s(#)') 		play video file, wait for 3 digits or break on '#' \n";


static const char *app_save = "mp4save";
static const char *syn_save = "MP4 file record";
static const char *des_save = "  mp4save(filename,[options]):  Record mp4 file. \n"
        "Note: If you are working with amr it's recommended that you use 3gp\n"
        "as your file extension if you want to play it with a video player.\n"
        "\n"
        "Available options:\n"
        " 'v': activate loopback of video\n"
        " 'V': wait for first video I frame to start recording\n"
	" '0'..'9','#','*': sets dtmf input to stop recording"
        "\n"
        "Note: waiting for video I frame also activate video loopback mode.\n"
        "\n"
        "Examples:\n"
        " mp4save(/tmp/save.3gp)    record video to selected file\n"
        " mp4save(/tmp/save.3gp,#)  record video and stop on '#' dtmf\n"
        " mp4save(/tmp/save.3gp,v)  activate loopback of video\n"
        " mp4save(/tmp/save.3gp,V)  wait for first videoto start recording\n"
        " mp4save(/tmp/save.3gp,V9) wait for first videoto start recording\n"
        "                           and stop on '9' dtmf\n";


struct mp4track {
	MP4FileHandle mp4;
	MP4TrackId track;
	MP4TrackId hint;
	bool first;
	bool intra;
	unsigned char frame[65535];
	int length;
	int sampleId;
};


enum {
        OPT_DFTMINTOVAR 	=	(1 << 0),
        OPT_NOOFDTMF 		=	(1 << 1),
	OPT_STOPDTMF		=	(1 << 2),
} mp4play_exec_option_flags;

enum {
        OPT_ARG_DFTMINTOVAR =          0,
        OPT_ARG_NOOFDTMF,
	OPT_ARG_STOPDTMF,
        /* note: this entry _MUST_ be the last one in the enum */
	OPT_ARG_ARRAY_SIZE,
} mp4play_exec_option_args;

AST_APP_OPTIONS(mp4play_exec_options, {
        AST_APP_OPTION_ARG('S', OPT_DFTMINTOVAR, OPT_ARG_DFTMINTOVAR),
        AST_APP_OPTION_ARG('n', OPT_NOOFDTMF, OPT_ARG_NOOFDTMF),
	AST_APP_OPTION_ARG('s', OPT_STOPDTMF, OPT_ARG_STOPDTMF),
});


#define MAX_DTMF_BUFFER_SIZE 25

//void dump_buffer_hex(const unsigned char * text, const unsigned char * buff, int len)
//{
//        int i;
//        unsigned char *temp;
//        temp = malloc(len*3+20); /* actually only len*3 would be needed, but negative values need more then 2 bytes*/
//        if (temp == NULL) {
//                ast_log(LOG_DEBUG, "app_mp4.c: dump_buffer_hex: failed to allocate buffer!\n");
//                return;
//        }
//        for (i=0;i<len;i++) {
//                sprintf( temp+3*i, "%02x ", buff[i]);
//        }
//        ast_log(LOG_DEBUG, "app_mp4.c: dump_buffer_hex: %s\n%s\n",text,temp);
//        free(temp);
//}


static int mp4_rtp_write_audio(struct mp4track *t, struct ast_frame *f, int payload)
{
	/* Next sample */
	t->sampleId++;

	ast_log(LOG_DEBUG, "Saving #%d:%d:%d %d samples %d size of audio, payload=%d\n", t->sampleId, t->track, t->hint, f->samples, f->datalen,payload);
	/* dump_buffer_hex("AMR buffer",f->data,f->datalen); */

	/* Add hint */
	MP4AddRtpHint(t->mp4, t->hint);

	/* Add rtp packet to hint track */
	MP4AddRtpPacket(t->mp4, t->hint, 0, 0);

	/* Save rtp specific payload header to hint */
	if (payload > 0)
		MP4AddRtpImmediateData(t->mp4, t->hint, AST_FRAME_GET_BUFFER(f), payload);

	/* Set which part of sample audio goes to this rtp packet */
	MP4AddRtpSampleData(t->mp4, t->hint, t->sampleId, 0, f->datalen - payload);

	/* Write rtp hint */
	MP4WriteRtpHint(t->mp4, t->hint, f->samples, 1);

	/* Write audio */
	MP4WriteSample(t->mp4, t->track, AST_FRAME_GET_BUFFER(f) + payload, f->datalen - payload, f->samples, 0, 0);

	return 0;
}

static void mp4_rtp_write_video_frame(struct mp4track *t, int samples)
{
	ast_log(LOG_DEBUG, "Saving #%d:%d:%d %d samples %d size of video\n", t->sampleId, t->track, t->hint, samples, t->length);

	/* Save rtp hint */
	MP4WriteRtpHint(t->mp4, t->hint, samples, t->intra);
	/* Save video frame */
	MP4WriteSample(t->mp4, t->track, t->frame, t->length, samples, 0, t->intra);
}

static int mp4_rtp_write_video(struct mp4track *t, struct ast_frame *f, int payload, bool intra, int skip, const unsigned char * prependBuffer, int prependLength)
{
	/* rtp mark */
	const bool mBit = (f->subclass).integer & 0x1;

	/* If it's the first packet of a new frame */
	if (t->first) {
		/* If we hava a sample */
		if (t->sampleId > 0) {
			/* Save frame */
			mp4_rtp_write_video_frame(t, f->samples);
			/* Reset buffer length */
			t->length = 0;
		}
		/* Reset first mark */
		t->first = 0;

		/* Save intra flag */
		t->intra = intra;

		/* Next frame */
		t->sampleId++;

		ast_log(LOG_DEBUG, "New video hint [%d,%d,%d,%d]\n",intra,payload,skip,prependLength);

		/* Add hint */
		MP4AddRtpHint(t->mp4, t->hint);
	}

	/* Add rtp packet to hint track */
	MP4AddRtpPacket(t->mp4, t->hint, mBit, 0);

	/* Save rtp specific payload header to hint */
	if (payload > 0)
		MP4AddRtpImmediateData(t->mp4, t->hint, AST_FRAME_GET_BUFFER(f), payload);

	/* If we have to prepend */
	if (prependLength)
	{
		/* Prepend data to video buffer */
		memcpy(t->frame + t->length, (char*)prependBuffer, prependLength);

		/* Inc length */
		t->length += prependLength;
	}

	/* Set hint reference to video data */
	MP4AddRtpSampleData(t->mp4, t->hint, t->sampleId, (u_int32_t) t->length, f->datalen - payload - skip);

	/* Copy the video data to buffer */
	memcpy(t->frame + t->length, AST_FRAME_GET_BUFFER(f) + payload + skip, f->datalen - payload - skip);

	/* Increase stored buffer length */
	t->length += f->datalen - payload - skip;

	/* If it's the las packet in a frame */
	if (mBit)
		/* Set first mark */
		t->first = 1;

	return 0;
}

struct mp4rtp {
	struct ast_channel *chan;
	MP4FileHandle mp4;
	MP4TrackId hint;
	MP4TrackId track;
	unsigned int timeScale;
	unsigned int sampleId;
	unsigned short numHintSamples;
	unsigned short packetIndex;
	unsigned int frameSamples;
	int frameSize;
	int frameTime;
	int frameType;
	int frameSubClass;
	char *name;
	char *src;
	unsigned char type;

};

static void mp4_send_h264_sei(struct mp4rtp *p)
{
	unsigned char buffer[PKT_SIZE];
        struct ast_frame *f = (struct ast_frame *) buffer;
	uint8_t **sequenceHeader;
	uint8_t **pictureHeader;
	uint32_t *pictureHeaderSize;
	uint32_t *sequenceHeaderSize;
	uint32_t i;
	uint8_t* data;
	uint32_t dataLen;

	/* Get SEI information */
	MP4GetTrackH264SeqPictHeaders(p->mp4, p->track, &sequenceHeader, &sequenceHeaderSize, &pictureHeader, &pictureHeaderSize);

	/* Unset */
	memset(f, 0, PKT_SIZE);

	/* Let mp4 lib allocate memory */
	AST_FRAME_SET_BUFFER(f,f,PKT_OFFSET,PKT_PAYLOAD);
	f->src = strdup(p->src);

	/* Set type */
	f->frametype = p->frameType;
	(f->subclass).integer = p->frameSubClass;

	f->delivery.tv_usec = 0;
	f->delivery.tv_sec = 0;
	/* Don't free the frame outside */
	f->mallocd = 0;

	/* Get data pointer */
	data = AST_FRAME_GET_BUFFER(f);
	/* Reset length */
	dataLen = 0;

	/* Send sequence headers */
	i=0;

	/* Check we have sequence header */
	if (sequenceHeader)
		/* Loop array */
		while(sequenceHeader[i] && sequenceHeaderSize[i])
		{
			/* Check if it can be handled in a single packeti */
			if (sequenceHeaderSize[i]<1400)
			{
				/* If there is not enought length */
				if (dataLen+sequenceHeaderSize[i]>1400)
				{
					/* Set data length */
					f->datalen = dataLen;
					/* Write frame */
					ast_write(p->chan, f);
					/* Reset data */
					dataLen = 0;
				}
				/* Copy data */
				memcpy(data+dataLen,sequenceHeader[i],sequenceHeaderSize[i]);	
				/* Increase pointer */
				dataLen+=sequenceHeaderSize[i];
			}
			/* Free memory */
			free(sequenceHeader[i]);
			/* Next header */
			i++;
		}

	/* If there is still data */
	if (dataLen>0)
	{
		/* Set data length */
		f->datalen = dataLen;
		/* Write frame */
		ast_write(p->chan, f);
		/* Reset data */
		dataLen = 0;
	}

	/* Send picture headers */
	i=0;

	/* Check we have picture header */
	if (pictureHeader)
		/* Loop array */
		while(pictureHeader[i] && pictureHeaderSize[i])
		{
			/* Check if it can be handled in a single packeti */
			if (pictureHeaderSize[i]<1400)
			{
				/* If there is not enought length */
				if (dataLen+pictureHeaderSize[i]>1400)
				{
					/* Set data length */
					f->datalen = dataLen;
					/* Write frame */
					ast_write(p->chan, f);
					/* Reset data */
					dataLen = 0;
				}
				/* Copy data */
				memcpy(data+dataLen,pictureHeader[i],pictureHeaderSize[i]);	
				/* Increase pointer */
				dataLen+=pictureHeaderSize[i];
			}
			/* Free memory */
			free(pictureHeader[i]);
			/* Next header */
			i++;
		}

	/* If there is still data */
	if (dataLen>0)
	{
		/* Set data length */
		f->datalen = dataLen;
		/* Write frame */
		ast_write(p->chan, f);
		/* Reset data */
		dataLen = 0;
	}

	/* Free data */
	if (pictureHeader)
		free(pictureHeader);
	if (sequenceHeader)
		free(sequenceHeader);
	if (sequenceHeaderSize)
		free(sequenceHeaderSize);
	if (pictureHeaderSize)
		free(pictureHeaderSize);

}

static int mp4_rtp_read(struct mp4rtp *p)
{

	unsigned char buffer[PKT_SIZE];
	struct ast_frame *f = (struct ast_frame *) buffer;
	int next = 0;
	int last = 0;
	int first = 0;
	uint8_t* data;

	/* If it's first packet of a frame */
	if (!p->numHintSamples) {
		/* Get number of rtp packets for this sample */
		if (!MP4ReadRtpHint(p->mp4, p->hint, p->sampleId, &p->numHintSamples)) {
			ast_log(LOG_DEBUG, "MP4ReadRtpHint failed [%d,%d]\n", p->hint,p->sampleId);
			return -1;
		}

		/* Get number of samples for this sample */
		p->frameSamples = MP4GetSampleDuration(p->mp4, p->hint, p->sampleId);

		/* Get size of sample */
		p->frameSize = MP4GetSampleSize(p->mp4, p->hint, p->sampleId);

		/* Get sample timestamp */
		p->frameTime = MP4GetSampleTime(p->mp4, p->hint, p->sampleId);

		/* Set first flag */
		first = 1;

		/* Check if it is H264 and it is a Sync frame*/
		if (p->frameSubClass==AST_FORMAT_H264 && MP4GetSampleSync(p->mp4,p->track,p->sampleId))
			/* Send SEI info */
			mp4_send_h264_sei(p);
	}

	/* if it's the last */
	if (p->packetIndex + 1 == p->numHintSamples)
		last = 1;

	/* Unset */
	memset(f, 0, PKT_SIZE);

	/* Let mp4 lib allocate memory */
	AST_FRAME_SET_BUFFER(f,f,PKT_OFFSET,PKT_PAYLOAD);
	f->src = strdup(p->src);

	/* Set type */
	f->frametype = p->frameType;
	(f->subclass).integer = p->frameSubClass;

	f->delivery.tv_usec = 0;
	f->delivery.tv_sec = 0;
	/* Don't free the frame outside */
	f->mallocd = 0;

	/* If it's video set the mark of last rtp packet */
	if (f->frametype == AST_FRAME_VIDEO)
	{
		/* Set mark bit */
		(f->subclass).integer |= last;
		/* If it's the first packet of the frame */
		if (first)
			/* Set number of samples */
			f->samples = p->frameSamples * (90000 / p->timeScale);
	} else {
		/* Set number of samples */
		f->samples = p->frameSamples;
	}

	/* Get data pointer */
	data = AST_FRAME_GET_BUFFER(f);

	/* Read next rtp packet */
	if (!MP4ReadRtpPacket(
				p->mp4,				/* MP4FileHandle hFile */
				p->hint,			/* MP4TrackId hintTrackId */
				p->packetIndex++,		/* u_int16_t packetIndex */
				(u_int8_t **) &data,		/* u_int8_t** ppBytes */
				(u_int32_t *) &f->datalen,	/* u_int32_t* pNumBytes */
				0,				/* u_int32_t ssrc DEFAULT(0) */
				0,				/* bool includeHeader DEFAULT(true) */
				1				/* bool includePayload DEFAULT(true) */
			)) {
		ast_log(LOG_DEBUG, "Error reading packet [%d,%d]\n", p->hint, p->track);
		return -1;
	}

	/* Write frame */
	ast_write(p->chan, f);

	/* Are we the last packet in a hint? */
	if (last) {
		/* The first hint */
		p->packetIndex = 0;
		/* Go for next sample */
		p->sampleId++;
		p->numHintSamples = 0;
	}

	/* Set next send time */
	if ((!last) && (f->frametype == AST_FRAME_VIDEO))
		/* Send next now if it's not the last packet of the frame */
		/* This will send all the packets from the same frame without pausing between them */
		/* FIX: should wait depending on bandwith */
		next = 0;
	else if (p->timeScale)
		/* If it's from a different frame or it's audio */
		next = (p->frameSamples * 1000) / p->timeScale;
	else
		next = -1;

	/* exit next send time */
	return next;
}

static int mp4_play(struct ast_channel *chan, void *data)
{
	struct ast_module_user *u = NULL;
	struct mp4rtp audio = { chan, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ,0 };
	struct mp4rtp video = { chan, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 ,0 };
	MP4FileHandle mp4;
	MP4TrackId hintId;
	MP4TrackId trackId;
	int audioNext = -1;
	int videoNext = -1;
	int t = 0;
	int i = 0;
	struct ast_frame *f = NULL;
	char src[128];
	int res = 0;
	struct timeval tv ;

	char *parse;
	int numberofDigits = -1;
	char *varName = NULL;
	char *stopChars = NULL;
	char dtmfBuffer[MAX_DTMF_BUFFER_SIZE];
	struct ast_flags opts = { 0, };
	char *opt_args[OPT_ARG_ARRAY_SIZE];

	AST_DECLARE_APP_ARGS(args, AST_APP_ARG(filename); AST_APP_ARG(options););

	/* Check for data */
	if (!data || ast_strlen_zero(data)) {
                ast_log(LOG_WARNING, "mp4play requires an argument (filename)\n");
                return -1;
        }

	ast_log(LOG_DEBUG, "mp4play %s\n", (char *)data);

	/* Set random src */
	snprintf(src, 128, "mp4play%08lx", ast_random());
	audio.src = src;
	video.src = src;

	/* Reset dtmf buffer */
	memset(dtmfBuffer,0,MAX_DTMF_BUFFER_SIZE);

	/* Lock module */
	u = ast_module_user_add(chan);

	/* Duplicate input */
	parse = ast_strdup(data);

	/* Get input data */
	AST_STANDARD_APP_ARGS(args, parse);

	/* Parse input data */
	if (!ast_strlen_zero(args.options) &&
			ast_app_parse_options(mp4play_exec_options, &opts, opt_args, args.options)) {
		ast_log(LOG_WARNING, "mp4play cannot partse options\n");
		res = -1;
		goto clean;
	}

	/* Check filename */
	if (ast_strlen_zero(args.filename)) {
		ast_log(LOG_WARNING, "mp4play requires an argument (filename)\n");
		res = -1;
		goto clean;
	}

	/* If we have DTMF number of digits options chek it */
	if (ast_test_flag(&opts, OPT_NOOFDTMF) && !ast_strlen_zero(opt_args[OPT_ARG_NOOFDTMF])) {

		/* Get number of digits to wait for */
		numberofDigits = atoi(opt_args[OPT_ARG_NOOFDTMF]);

		/* Check valid number */
		if (numberofDigits<=0) {
			ast_log(LOG_WARNING, "mp4play does not accept n(%s), hanging up.\n", opt_args[OPT_ARG_NOOFDTMF]);
			res = -1;
			goto clean;
		}

		/* Check valid sizei */ 
		if (numberofDigits>MAX_DTMF_BUFFER_SIZE-1) {
			numberofDigits = MAX_DTMF_BUFFER_SIZE-1;
			ast_log(LOG_WARNING, "mp4play does not accept n(%s), buffer is too short cutting to %d .\n", opt_args[OPT_ARG_NOOFDTMF],MAX_DTMF_BUFFER_SIZE-1);
		}

		if (option_verbose > 2)
			ast_verbose(VERBOSE_PREFIX_3 "Setting number of digits to %d seconds.\n", numberofDigits);
	}

	/* If we have DTMF set variable otpion chekc it */
	if (ast_test_flag(&opts, OPT_DFTMINTOVAR) && !ast_strlen_zero(opt_args[OPT_ARG_DFTMINTOVAR])) {

		/* Get variable name */
		varName = opt_args[OPT_ARG_DFTMINTOVAR];

		if (option_verbose > 2)
			ast_verbose(VERBOSE_PREFIX_3 "Setting variable name to %s .\n", varName);
	}

	/* If we have DTMF stop digit optiont check it */
        if (ast_test_flag(&opts, OPT_STOPDTMF) && !ast_strlen_zero(opt_args[OPT_ARG_STOPDTMF])) {

		/* Get stop digits */
                stopChars = opt_args[OPT_ARG_STOPDTMF];

                if (option_verbose > 2)
			ast_verbose(VERBOSE_PREFIX_3 "Stop chars are %s.\n",stopChars);
        }

	/* Open mp4 file */
	mp4 = MP4Read((char *) args.filename, 9);

	/* If not valid */
	if (mp4 == MP4_INVALID_FILE_HANDLE)
	{
		ast_log(LOG_WARNING, "Invalid file handle for %s\n",(char *) args.filename);
		/* exit */
		res = -1;
		goto clean;
	}

	/* Get the first hint track */
	hintId = MP4FindTrackId(mp4, i++, MP4_HINT_TRACK_TYPE, 0);

	/* Iterate hint tracks */
	while (hintId != MP4_INVALID_TRACK_ID) {

		ast_log(LOG_DEBUG, "found hint track %d\n", hintId);

		/* Get asociated track */
		trackId = MP4GetHintTrackReferenceTrackId(mp4, hintId);

		/* Check it's good */
		if (trackId != MP4_INVALID_TRACK_ID) {

			/* Get type */
			const char *type = MP4GetTrackType(mp4, trackId);

			ast_log(LOG_DEBUG, "track %d %s\n", trackId, type);

			/* Check track type */
			if (strcmp(type, MP4_AUDIO_TRACK_TYPE) == 0) {
				/* it's audio */
				audio.mp4 = mp4;
				audio.hint = hintId;
				audio.track = trackId;
				audio.sampleId = 1;
				audio.packetIndex = 0;
				audio.frameType = AST_FRAME_VOICE;

				/* Get audio type */
				MP4GetHintTrackRtpPayload(mp4, hintId, &audio.name, &audio.type, NULL, NULL);

				/* Get time scale */
				audio.timeScale = MP4GetTrackTimeScale(mp4, hintId);

				/* Depending on the name */
				if (strcmp("PCMU", audio.name) == 0)
				{
					audio.frameSubClass = AST_FORMAT_ULAW;
					if (ast_set_write_format(chan, AST_FORMAT_ULAW))
						ast_log(LOG_WARNING, "mp4_play:	Unable to set write format to ULAW!\n");
				}
				else if (strcmp("PCMA", audio.name) == 0)
				{
					audio.frameSubClass = AST_FORMAT_ALAW;
					if (ast_set_write_format(chan, AST_FORMAT_ALAW))
						ast_log(LOG_WARNING, "mp4_play:	Unable to set write format to ALAW!\n");
				} 
				else if (strcmp("AMR", audio.name) == 0)
				{
					audio.frameSubClass = AST_FORMAT_AMRNB;
					if (ast_set_write_format(chan, AST_FORMAT_AMRNB))
						ast_log(LOG_WARNING, "mp4_play:	Unable to set write format to AMR-NB!\n");
				}

			} else if (strcmp(type, MP4_VIDEO_TRACK_TYPE) == 0) {
				/* it's video */
				video.mp4 = mp4;
				video.hint = hintId;
				video.track = trackId;
				video.sampleId = 1;
				video.packetIndex = 0;
				video.frameType = AST_FRAME_VIDEO;

				/* Get video type */
				MP4GetHintTrackRtpPayload(mp4, hintId, &video.name, &video.type, NULL, NULL);

				/* Get time scale */
				video.timeScale = MP4GetTrackTimeScale(mp4, hintId);

				ast_log(LOG_DEBUG, "video.name %s\n", video.name);

				/* Depending on the name */
				if (strcmp("H263", video.name) == 0)
					video.frameSubClass = AST_FORMAT_H263;
				else if (strcmp("H263-1998", video.name) == 0)
					video.frameSubClass = AST_FORMAT_H263_PLUS;
				else if (strcmp("H263-2000", video.name) == 0)
					video.frameSubClass = AST_FORMAT_H263_PLUS;
				else if (strcmp("H264", video.name) == 0)
					video.frameSubClass = AST_FORMAT_H264;
			}
		}

		/* Get the next hint track */
		hintId = MP4FindTrackId(mp4, i++, MP4_HINT_TRACK_TYPE, 0);
	}

	/* If we have audio */
	if (audio.name)
		/* Send audio */
		audioNext = mp4_rtp_read(&audio);

	/* If we have video */
	if (video.name)
		/* Send video */
		videoNext = mp4_rtp_read(&video);

	/* Calculate start time */
	tv = ast_tvnow();

	/* Wait control messages or finish of both streams */
	while (!((audioNext < 0) && (videoNext < 0))) {
		/* Get next time */
		if (audioNext < 0)
			t = videoNext;
		else if (videoNext < 0)
			t = audioNext;
		else if (audioNext < videoNext)
			t = audioNext;
		else
			t = videoNext;

		/* Wait time */
		int ms = t;

		/* Read from channel and wait timeout */
		while (ms > 0) {
			/* Wait */
			ms = ast_waitfor(chan, ms);

			/* if we have been hang up */
			if (ms < 0) 
				/* exit */
				goto end;

			/* if we have received something on the channel */
			if (ms > 0) {
				/* Read frame */
				f = ast_read(chan);

				/* If failed */
				if (!f) 
					/* exit */
					goto end;

				/* If it's a dtmf */
				if (f->frametype == AST_FRAME_DTMF) {

					/* Stop flag */
					bool stop = false;

					/* Get DTMF char */
					char dtmf[2];
					dtmf[0] = (f->subclass).integer;
					dtmf[1] = 0;

					/* Check if it's in the stop char digits */
					if (stopChars && strchr(stopChars,dtmf[0])) {
						/* Clean DMTF input */
						strcpy(dtmfBuffer,dtmf);
						/* Stop */
						stop = true;
						/* Continue after exit */
						res = 0;
						/* Log */
						ast_log( LOG_WARNING, "mp4play - found stop char %s\n",dtmf);
					/* Check if we have to append the DTMF and wait for more than one digit */
					} else if (numberofDigits>0) {
						/* Append to the DTMF buffer */
						strcat(dtmfBuffer,dtmf);
						/* Check length */
						if (strlen(dtmfBuffer)>=numberofDigits) {
							/* Continue after exit */
							res = 0;
							/* Stop */
							stop = true;	
						}
					/* Check for dtmf extension in context */
					} else if (ast_exists_extension(chan, chan->context, dtmf, 1, NULL)) {
						/* Set extension to jump */
						res = (f->subclass).integer;
						/* Clean DMTF input */
						strcpy(dtmfBuffer,dtmf);
						/* End */
						stop = true;
					}
					
					/* If we have to stop */
					if (stop) {
						/* Check DTMF variable`option*/
						if (varName)
							/* Build variable */
							pbx_builtin_setvar_helper(chan, varName, dtmfBuffer);
						/* Free frame */
						ast_frfree(f);
						/* exit */
						goto end;
					}
				} 

				/* Free frame */
				ast_frfree(f);
			}

			/* Calculate elapsed time */
			ms = t - ast_tvdiff_ms(ast_tvnow(),tv);
		}

		/* Get new time */
		struct timeval tvn = ast_tvnow();

		/* Calculate elapsed */
		t = ast_tvdiff_ms(tvn,tv);

		/* Set new time */
		tv = tvn;

		/* Remove time */
		if (audioNext > 0)
			audioNext -= t;
		if (videoNext > 0)
			videoNext -= t;

		/* if we have to send audio */
		if (audioNext<=0 && audio.name)
			audioNext += mp4_rtp_read(&audio);

		/* or video */
		if (videoNext<=0 && video.name)
			videoNext = mp4_rtp_read(&video);
	}

end:
	/* Log end */
	ast_log(LOG_DEBUG, "<app_mp4");

	/* Close file */
	MP4Close(mp4);

clean:
	/* Unlock module*/
	ast_module_user_remove(u);

	/* Free datra*/
	free(parse);

	/* Exit */
	return res;
}

static int mp4_save(struct ast_channel *chan, void *data)
{
	struct ast_module_user *u = NULL;
	struct ast_frame *f = NULL;
	struct mp4track audioTrack;
	struct mp4track videoTrack;
	MP4FileHandle mp4;
	MP4TrackId audio = -1;
	MP4TrackId video = -1;
	MP4TrackId hintAudio = -1;
	MP4TrackId hintVideo = -1;
	unsigned char type = 0;
	char *params = NULL;
	int audio_payload, video_payload;
	int loopVideo = 0;
	int waitVideo = 0;

	/* Check for file */
	if (!data)
		return -1;

	/* Check for params */
	params = strchr(data,'|');

	/* If there are params */
	if (params)
	{
		/* Remove from file name */
		*params = 0;

		/* Increase pointer */
		params++;

		/* Check video loopback */
		if (strchr(params,'v'))
		{
			/* Enable video loopback */
			loopVideo = 1;
		}

		/* Check video waiting */
		if (strchr(params,'V'))
		{
			/* Enable video loopback & waiting*/
			loopVideo = 1;
			waitVideo = 1;
		}
	}

	ast_log(LOG_DEBUG, ">mp4save [%s,%s]\n",(char*)data,params);

	/* Create mp4 file */
	mp4 = MP4CreateEx((char *) data, 9, 0, 1, 1, 0, 0, 0, 0);

	/* If failed */
	if (mp4 == MP4_INVALID_FILE_HANDLE)
		return -1;

	/* Lock module */
	u = ast_module_user_add(chan);

	if (ast_set_read_format(chan, AST_FORMAT_ULAW|AST_FORMAT_ALAW|AST_FORMAT_AMRNB))
		ast_log(LOG_WARNING, "mp4_save: Unable to set read format to ULAW|ALAW|AMRNB!\n");

	/* Send video update */
	ast_indicate(chan, AST_CONTROL_VIDUPDATE);

	/* Wait for data avaiable on channel */
	while (ast_waitfor(chan, -1) > -1)
	{

		/* Read frame from channel */
		f = ast_read(chan);

		/* if it's null */
		if (f == NULL)
			break;

		/* Check if we have to wait for video */
		if ((f->frametype == AST_FRAME_VOICE) && (!waitVideo)) 
		{
			/* Check if we have the audio track */
			if (audio == -1)
			{
				/* Check codec */
				if ((f->subclass).codec & AST_FORMAT_ULAW)
				{
					/* Create audio track */
					audio = MP4AddAudioTrack(mp4, 8000, 0, MP4_ULAW_AUDIO_TYPE);
					/* Set channel and sample properties */
					/* !!!! this should be under "mdia.minf.stbl.stsd.ulaw.*" 
					 * but should have to rewrite MP4AddAudioTrack also then !!!!*/
					MP4SetTrackIntegerProperty(mp4, audio, "mdia.minf.stbl.stsd.mp4a.channels", 1);
					MP4SetTrackIntegerProperty(mp4, audio, "mdia.minf.stbl.stsd.mp4a.sampleSize", 8);
					/* Create audio hint track */
					hintAudio = MP4AddHintTrack(mp4, audio);
					/* Set payload type for hint track */
					type = 0;
					audio_payload = 0;
					MP4SetHintTrackRtpPayload(mp4, hintAudio, "PCMU", &type, 0, NULL, 1, 0);
				} else if ((f->subclass).codec & AST_FORMAT_ALAW) {
					/* Create audio track */
					audio = MP4AddAudioTrack(mp4, 8000, 0, MP4_ALAW_AUDIO_TYPE);
					/* Set channel and sample properties */
					/* !!!! this should be under "mdia.minf.stbl.stsd.alaw.*" 
					 * but should have to rewrite MP4AddAudioTrack also then !!!!*/
					MP4SetTrackIntegerProperty(mp4, audio, "mdia.minf.stbl.stsd.mp4a.channels", 1);
					MP4SetTrackIntegerProperty(mp4, audio, "mdia.minf.stbl.stsd.mp4a.sampleSize", 8);
					/* Create audio hint track */
					hintAudio = MP4AddHintTrack(mp4, audio);
					/* Set payload type for hint track */
					type = 8;
					audio_payload = 0;
					MP4SetHintTrackRtpPayload(mp4, hintAudio, "PCMA", &type, 0, NULL, 1, 0);
				} else if ((f->subclass).codec & AST_FORMAT_AMRNB) {
					/* Create audio track */
					audio = MP4AddAmrAudioTrack(mp4, 8000, 0, 0, 1, 0); /* Should check framesPerSample*/
					/* Create audio hint track */
					hintAudio = MP4AddHintTrack(mp4, audio);
					/* Set payload type for hint track */
					type = 98;
					audio_payload = 1;
					MP4SetHintTrackRtpPayload(mp4, hintAudio, "AMR", &type, 0, NULL, 1, 0);
					/* Unknown things */
					MP4SetAudioProfileLevel(mp4, 0xFE);
				} else {
					/* Unknown code free it*/
					ast_frfree(f);
					/* skip this one */
					continue;
				}

				/* Set struct info */
				audioTrack.mp4 = mp4;
				audioTrack.track = audio;
				audioTrack.hint = hintAudio;
				audioTrack.length = 0;
				audioTrack.sampleId = 0;
				audioTrack.first = 1;
			}

			/* Check we have audio track */
			if (audio)
				/* Save audio rtp packet */
				mp4_rtp_write_audio(&audioTrack, f, audio_payload);

		} else if (f->frametype == AST_FRAME_VIDEO) {
			/* No skip and no add */
			int skip = 0;
			unsigned char *prependBuffer = NULL;
			unsigned char *frame = AST_FRAME_GET_BUFFER(f);
			int prependLength = 0;
			int intra = 0;
			int first = 0;

			/* Check codec */
			if ((f->subclass).codec & AST_FORMAT_H263)
			{
				/* Check if it's an intra frame */
				intra = (frame[1] & 0x10) != 0;
				/* Check PSC for first packet of frame */
				if ( (f->datalen>7) && (frame[4] == 0) && (frame[5] == 0) && ((frame[6] & 0xFC) == 0x80) )
					/* It's first */
					first = 1;
				/* payload length */
				video_payload = 4;
			} else if ((f->subclass).codec & AST_FORMAT_H263_PLUS) {
				/* Check if it's an intra frame */
				const unsigned char p = frame[0] & 0x04;
				const unsigned char v = frame[0] & 0x02;
				const unsigned char plen = ((frame[0] & 0x1 ) << 5 ) | (frame[1] >> 3);
				// const unsigned char pebit = frame[0] & 0x7; unused variable
				
				/* payload length */
				video_payload = 2;
				/* skip rest of headers */
				skip = plen + v;
				/* If its first packet of frame*/
				if (p)
				{
					/* it's first */
					first = 1;
					/* Check for intra in stream */
					intra = !(frame[4] & 0x02);
					/* Prepend open code */
					prependBuffer = (unsigned char*)"\0\0";
					prependLength = 2;
				}
			} else if ((f->subclass).codec & AST_FORMAT_H264) {
				/* Get packet type */
				const unsigned char nal = frame[0];
				const unsigned char type = nal & 0x1f;
				/* All intra & first*/
				intra = 1;
				first = 1;
				/* Check nal type */
				if (type<23) 
				{
					/* And add the data to the frame but not associated with the hint track */
					prependBuffer = (unsigned char*)"\0\0\1";
					prependLength = 3;
					/* Set payload and skip */
					video_payload = 0;
					skip = 0;
				}
			} else {
				/* Unknown code free it */
				ast_frfree(f);
				/* skip this one */
				continue;
			}

			/* Check if we have to wait for video */
			if (waitVideo)
			{
				/* If it's the first packet of an intra frame */
				if (first && intra)
				{
					/* no more waiting */
					waitVideo = 0;
				} else {
					/* free frame */
					ast_frfree(f);
					/* Keep on waiting */
					continue;
				}
			}

			/* Check if we have the video track */
			if (video == -1)
			{
				/* Check codec */
				if ((f->subclass).codec & AST_FORMAT_H263)
				{
					/* Create video track */
					video = MP4AddH263VideoTrack(mp4, 90000, 0, 176, 144, 0, 0, 0, 0);
					/* Create video hint track */
					hintVideo = MP4AddHintTrack(mp4, video);
					/* Set payload type for hint track */
					type = 34;
					MP4SetHintTrackRtpPayload(mp4, hintVideo, "H263", &type, 0, NULL, 1, 0);
				} else if ((f->subclass).codec & AST_FORMAT_H263_PLUS) {
					/* Create video track */
					video = MP4AddH263VideoTrack(mp4, 90000, 0, 176, 144, 0, 0, 0, 0);
					/* Create video hint track */
					hintVideo = MP4AddHintTrack(mp4, video);
					/* Set payload type for hint track */
					type = 96;
					MP4SetHintTrackRtpPayload(mp4, hintVideo, "H263-1998", &type, 0, NULL, 1, 0);
				} else if ((f->subclass).codec & AST_FORMAT_H264) {
					/* Should parse video packet to get this values */
					unsigned char AVCProfileIndication 	= 2;
					unsigned char AVCLevelIndication	= 1;
					unsigned char AVCProfileCompat		= 1;
					MP4Duration h264FrameDuration		= 1.0/30;
					/* Create video track */
					video = MP4AddH264VideoTrack(mp4, 90000, h264FrameDuration, 176, 144, AVCProfileIndication, AVCProfileCompat, AVCLevelIndication,  3);
					/* Create video hint track */
					hintVideo = MP4AddHintTrack(mp4, video);
					/* Set payload type for hint track */
					type = 99;
					MP4SetHintTrackRtpPayload(mp4, hintVideo, "H264", &type, 0, NULL, 1, 0);

				}

				/* Set struct info */
				videoTrack.mp4 = mp4;
				videoTrack.track = video;
				videoTrack.hint = hintVideo;
				videoTrack.length = 0;
				videoTrack.sampleId = 0;
				videoTrack.first = 1;
			}

			/* If we have created the track */
			if (video)
				/* Write rtp video packet */
				mp4_rtp_write_video(&videoTrack, f, video_payload, intra, skip, prependBuffer , prependLength);

			/* If video loopback is activated */
			if (loopVideo)
			{
				/* Send it back */
				ast_write(chan, f);
				/* Don't delete it */
				f = NULL;
			}

		} else if (f->frametype == AST_FRAME_DTMF) {
			/* If it's the dtmf param */
			if (params && strchr(params,(f->subclass).integer))
			{
				/* free frame */
				ast_frfree(f);
				/* exit */
				break;
			}

		}

		/* If we have frame */
		if (f)
			/* free frame */
			ast_frfree(f);
	}

	/* Save last video frame if needed */
	if (videoTrack.sampleId > 0)
		/* Save frame */
		mp4_rtp_write_video_frame(&videoTrack, 0);

	/* Close file */
	MP4Close(mp4);

	/* Unlock module*/
	ast_module_user_remove(u);

	printf("<mp4save\n");

	//Success
	return 0;
}

static int unload_module(void)
{
	int res = ast_unregister_application(app_play);
	res &= ast_unregister_application(app_save);

	ast_module_user_hangup_all();

	return res;
}

static int load_module(void)
{
	int res = ast_register_application(app_save, mp4_save, syn_save, des_save);
	res &= ast_register_application(app_play, mp4_play, syn_play, des_play);
	return 0;
}

//AST_MODULE_INFO_STANDARD(ASTERISK_GPL_KEY, "MP4 applications");

AST_MODULE_INFO(ASTERISK_GPL_KEY, AST_MODFLAG_DEFAULT, "MP4 application",
		.load = load_module,
		.unload = unload_module,
	       );
