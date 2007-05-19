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

/*
 * Changelog:
 *
 *  15-01-2006
 *  	Code cleanup and ast_module_user_add added.
 *  	Thanxs Denis Smirnov.
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

#ifndef AST_FORMAT_AMR
#define AST_FORMAT_AMR (1 << 13)
#endif 

static char *app_play = "mp4play";
static char *syn_play = "MP4 file playblack";
static char *des_play = "  mp4play():  Play mp4 file to user. \n";

static char *app_save = "mp4save";
static char *syn_save = "MP4 file record";
static char *des_save = "  mp4save():  Record mp4 file. \n";

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

static int mp4_rtp_write_audio(struct mp4track *t, struct ast_frame *f, int payload)
{
	/* Next sample */
	t->sampleId++;

	ast_log(LOG_DEBUG, "Saving #%d:%d:%d %d samples %d size of audio\n", t->sampleId, t->track, t->hint, f->samples, f->datalen);

	/* Add hint */
	MP4AddRtpHint(t->mp4, t->hint);

	/* Add rtp packet to hint track */
	MP4AddRtpPacket(t->mp4, t->hint, 0, 0);

	/* Save rtp specific payload header to hint */
	if (payload > 0)
		MP4AddRtpImmediateData(t->mp4, t->hint, f->data, payload);

	/* Set which part of sample audio goes to this rtp packet */
	MP4AddRtpSampleData(t->mp4, t->hint, t->sampleId, 0, f->datalen - payload);

	/* Write rtp hint */
	MP4WriteRtpHint(t->mp4, t->hint, f->samples, 1);

	/* Write audio */
	MP4WriteSample(t->mp4, t->track, f->data + payload, f->datalen - payload, f->samples, 0, 0);

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

static int mp4_rtp_write_video(struct mp4track *t, struct ast_frame *f, int payload, bool intra, int skip, unsigned char * prependBuffer, int prependLength)
{
	/* rtp mark */
	bool mBit = f->subclass & 0x1;

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

		ast_log(LOG_DEBUG, "New video hint\n");

		/* Add hint */
		MP4AddRtpHint(t->mp4, t->hint);
	}

	/* Add rtp packet to hint track */
	MP4AddRtpPacket(t->mp4, t->hint, mBit, 0);

	/* Save rtp specific payload header to hint */
	if (payload > 0)
		MP4AddRtpImmediateData(t->mp4, t->hint, f->data, payload);

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
	memcpy(t->frame + t->length, (char*)f->data + payload + skip, f->datalen - payload - skip);

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
	unsigned char type;

};

static int mp4_rtp_read(struct mp4rtp *p)
{
	struct ast_frame *f;
	int next = 0;
	int last = 0;

	/* If it's first packet of a frame */
	if (!p->numHintSamples) {
		/* Get number of rtp packets for this sample */
		if (!MP4ReadRtpHint(p->mp4, p->hint, p->sampleId, &p->numHintSamples)) {
			ast_log(LOG_DEBUG, "MP4ReadRtpHint failed\n");
			return -1;
		}

		/* Get number of samples for this sample */
		p->frameSamples = MP4GetSampleDuration(p->mp4, p->hint, p->sampleId);

		/* Get size of sample */
		p->frameSize = MP4GetSampleSize(p->mp4, p->hint, p->sampleId);

		/* Get sample timestamp */
		p->frameTime = MP4GetSampleTime(p->mp4, p->hint, p->sampleId);
	} else {
		/* Reset frame samples */
		p->frameSamples = 0;
	}


	/* if it's the last */
	if (p->packetIndex + 1 == p->numHintSamples)
		last = 1;

	/* malloc frame & data */
	f = (struct ast_frame *) malloc(sizeof(struct ast_frame) + 1500);

	/* Unset */
	memset(f, 0, sizeof(struct ast_frame) + 1500);

	/* Let mp4 lib allocate memory */
	f->data = (void*)f + AST_FRIENDLY_OFFSET;
	f->datalen = 1500;
	f->src = 0;

	/* Set type */
	f->frametype = p->frameType;
	f->subclass = p->frameSubClass;

	f->delivery.tv_usec = 0;
	f->delivery.tv_sec = 0;
	f->mallocd = 0;

	/* If it's video set the mark of last rtp packet */
	if (f->frametype == AST_FRAME_VIDEO)
	{
		/* Set mark bit */
		f->subclass |= last;
		/* Set number of samples */
		f->samples = p->frameSamples * (90000 / p->timeScale);
	} else {
		/* Set number of samples */
		f->samples = p->frameSamples;
	}

	/* Read next rtp packet */
	if (!MP4ReadRtpPacket(
				p->mp4,				/* MP4FileHandle hFile */
				p->hint,			/* MP4TrackId hintTrackId */
				p->packetIndex++,		/* u_int16_t packetIndex */
				(u_int8_t **) &f->data,		/* u_int8_t** ppBytes */
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
	if (!last && f->frametype == AST_FRAME_VIDEO)
		next = 0;
	else if (p->timeScale)
		next = (p->frameSamples * 1000) / p->timeScale;
	else
		next = -1;


	/* exit next send time */
	return next;
}

static int mp4_play(struct ast_channel *chan, void *data)
{
	struct ast_module_user *u;
	struct mp4rtp audio = { chan, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	struct mp4rtp video = { chan, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	MP4FileHandle mp4;
	MP4TrackId hintId;
	MP4TrackId trackId;
	const char *type = NULL;
	int audioNext = -1;
	int videoNext = -1;
	int t = 0;
	int i = 0;
	struct ast_frame *f;

	/* Check for data */
	if (!data)
		return -1;

	ast_log(LOG_DEBUG, "mp4play %s\n", (char *)data);
	printf( "mp4play %s\n", (char *)data);

	/* Lock module */
	u = ast_module_user_add(chan);

	/* Open mp4 file */
	mp4 = MP4Read((char *) data, 9);

	/* If not valid */
	if (mp4 == MP4_INVALID_FILE_HANDLE)
	{
		/* Unlock module*/
		ast_module_user_remove(u);
		/* exit */
		return -1;
	}

	/* Get the first hint track */
	hintId = MP4FindTrackId(mp4, i++, MP4_HINT_TRACK_TYPE, 0);

	/* Iterate hint tracks */
	while (hintId != MP4_INVALID_TRACK_ID) {

		ast_log(LOG_DEBUG, "found hint track %d\n", hintId);
		printf( "found hint track %d\n", hintId);

		/* Get asociated track */
		trackId = MP4GetHintTrackReferenceTrackId(mp4, hintId);

		/* Check it's good */
		if (trackId != MP4_INVALID_TRACK_ID) {

			/* Get type */
			type = MP4GetTrackType(mp4, trackId);

			ast_log(LOG_DEBUG, "track %d %s\n", trackId, type);
			printf( "track %d %s\n", trackId, type);

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
					audio.frameSubClass = AST_FORMAT_ULAW;
				else if (strcmp("PCMA", audio.name) == 0)
					audio.frameSubClass = AST_FORMAT_ALAW;
				else if (strcmp("AMR", audio.name) == 0)
					audio.frameSubClass = AST_FORMAT_AMR;

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
	struct timeval tv = ast_tvnow();

	/* Wait control messages or finish of both streams */
	while (!(audioNext < 0 && videoNext < 0)) {
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
			if (ms < 0) {
				/* Unlock module*/
				ast_module_user_remove(u);
				/* exit */
				return -1;
			}

			/* if we have received something on the channel */
			if (ms > 0) {
				/* Read frame */
				f = ast_read(chan);

				/* If failed */
				if (!f) {
					/* Unlock module*/
					ast_module_user_remove(u);
					/* exit */
					return -1;
				}

				/* If it's a dtmf */
				if (f->frametype == AST_FRAME_DTMF) {
					char dtmf[2];
					int res;

					/* Get dtmf number */
					res = f->subclass;
					dtmf[0] = res;
					dtmf[1] = 0;


					/* Check for dtmf extension in context */
					if (ast_exists_extension(chan, chan->context, dtmf, 1, NULL)) {
						/* Free frame */
						ast_frfree(f);
						/* exit */
						return res;
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
		if (audioNext<=0)
			audioNext += mp4_rtp_read(&audio);

		/* or video */
		if (videoNext<=0)
			videoNext = mp4_rtp_read(&video);
	}

	ast_log(LOG_DEBUG, "exit");

	/* Unlock module*/
	ast_module_user_remove(u);

	//Exit
	return 0;
}
static int mp4_save(struct ast_channel *chan, void *data)
{
	struct ast_module_user *u;
	struct ast_frame *f;
	struct mp4track audioTrack;
	struct mp4track videoTrack;
	MP4FileHandle mp4;
	MP4TrackId audio = -1;
	MP4TrackId video = -1;
	MP4TrackId hintAudio = -1;
	MP4TrackId hintVideo = -1;
	unsigned char type = 0;
	bool intra = 0;
	int payload;

	/* Check for file */
	if (!data)
		return -1;

	/* Create mp4 file */
	mp4 = MP4CreateEx((char *) data, 9, 0, 1, 1, 0, 0, 0, 0);

	/* If failed */
	if (mp4 == MP4_INVALID_FILE_HANDLE)
		return -1;

	/* Lock module */
	u = ast_module_user_add(chan);

	printf(">mp4save\n");

	/* Wait for data avaiable on channel */
	while (ast_waitfor(chan, -1) > -1) {

		/* Read frame from channel */
		f = ast_read(chan);

		/* if it's null */
		if (f == NULL)
			break;

		/* Check frame type */
		if (f->frametype == AST_FRAME_VOICE) {
			/* Check if we have the audio track */
			if (audio == -1) {
				/* Check codec */
				if (f->subclass & AST_FORMAT_ULAW) {
					/* Create audio track */
					audio = MP4AddAudioTrack(mp4, 8000, 0, MP4_ULAW_AUDIO_TYPE);
					/* Create audio hint track */
					hintAudio = MP4AddHintTrack(mp4, audio);
					/* Set payload type for hint track */
					type = 0;
					payload = 0;
					MP4SetHintTrackRtpPayload(mp4, hintAudio, "PCMU", &type, 0, NULL, 1, 0);
				} else if (f->subclass & AST_FORMAT_ALAW) {
					/* Create audio track */
					audio = MP4AddAudioTrack(mp4, 8000, 0, MP4_ALAW_AUDIO_TYPE);
					/* Create audio hint track */
					hintAudio = MP4AddHintTrack(mp4, audio);
					/* Set payload type for hint track */
					type = 8;
					payload = 0;
					MP4SetHintTrackRtpPayload(mp4, hintAudio, "PCMA", &type, 0, NULL, 1, 0);
				} else if (f->subclass & AST_FORMAT_AMR) {
					/* Create audio track */
					audio = MP4AddAmrAudioTrack(mp4, 8000, 0, 0, 1, 0); /* Should check framesPerSample*/
					/* Create audio hint track */
					hintAudio = MP4AddHintTrack(mp4, audio);
					/* Set payload type for hint track */
					type = 98;
					payload = 1;
					MP4SetHintTrackRtpPayload(mp4, hintAudio, "AMR", &type, 0, NULL, 1, 0);
					/* Unknown things */
					MP4SetAudioProfileLevel(mp4, 0xFE);
				}
				/* Set struct info */
				audioTrack.mp4 = mp4;
				audioTrack.track = audio;
				audioTrack.hint = hintAudio;
				audioTrack.length = 0;
				audioTrack.sampleId = 0;
				audioTrack.first = 1;
			}

			/* Save audio rtp packet */
			mp4_rtp_write_audio(&audioTrack, f, payload);

		} else if (f->frametype == AST_FRAME_VIDEO) {
			/* Check if we have the video track */
			if (video == -1) {
				/* Check codec */
				if (f->subclass & AST_FORMAT_H263) {
					/* Create video track */
					video = MP4AddH263VideoTrack(mp4, 90000, 0, 176, 144, 0, 0, 0, 0);
					/* Create video hint track */
					hintVideo = MP4AddHintTrack(mp4, video);
					/* Set payload type for hint track */
					type = 34;
					MP4SetHintTrackRtpPayload(mp4, hintVideo, "H263", &type, 0, NULL, 1, 0);
				} else if (f->subclass & AST_FORMAT_H263_PLUS) {
					/* Create video track */
					video = MP4AddH263VideoTrack(mp4, 90000, 0, 176, 144, 0, 0, 0, 0);
					/* Create video hint track */
					hintVideo = MP4AddHintTrack(mp4, video);
					/* Set payload type for hint track */
					type = 96;
					MP4SetHintTrackRtpPayload(mp4, hintVideo, "H263-1998", &type, 0, NULL, 1, 0);
				} else if (f->subclass & AST_FORMAT_H264) {
					/* Should parse video packet to get this values */
					unsigned char AVCProfileIndication 	= 2;
					unsigned char AVCLevelIndication	= 1;
					unsigned char AVCProfileCompat		= 1;
					MP4Duration h264FrameDuration		= 1.0/30;
					/* Create video track */
					video = MP4AddH264VideoTrack(mp4, 9000, h264FrameDuration, 176, 144, AVCProfileIndication, AVCProfileCompat, AVCLevelIndication,  3);
					/* Create video hint track */
					hintVideo = MP4AddHintTrack(mp4, video);
					/* Set payload type for hint track */
					type = 99;
					MP4SetHintTrackRtpPayload(mp4, hintVideo, "H264", &type, 0, NULL, 1, 0);

				} else {
					/* Unknown codec nothing to do */
					break;
				}

				/* Set struct info */
				videoTrack.mp4 = mp4;
				videoTrack.track = video;
				videoTrack.hint = hintVideo;
				videoTrack.length = 0;
				videoTrack.sampleId = 0;
				videoTrack.first = 1;
			}
			/* No skip and no add */
			int skip = 0;
			unsigned char *prependBuffer = NULL;
			int prependLength = 0;
			intra = 0;

			/* Check codec */
			if (f->subclass & AST_FORMAT_H263) {
				/* Check if it's an intra frame */
				intra = (((unsigned char *) (f->data))[1] & 0x10) != 0;
				/* payload length */
				payload = 4;
			} else if (f->subclass & AST_FORMAT_H263_PLUS) {
				/* Check if it's an intra frame */
				unsigned char p = ((unsigned char *) (f->data))[0] & 0x04;
				unsigned char v = ((unsigned char *) (f->data))[0] & 0x02;
				unsigned char plen = ((((unsigned char *) (f->data))[0] & 0x1 ) << 5 ) | (((unsigned char *) (f->data))[1] >> 3);
				unsigned char pebit = ((unsigned char *) (f->data))[0] & 0x7;
				/* payload length */
				payload = 2;
				/* skip rest of headers */
				skip = plen + v;
				/* If its first packet of frame*/
				if (p)
				{
					/* Check for intra in stream */
					intra = !(((unsigned char *) (f->data))[4] & 0x02);
					/* Prepend open code */
					prependBuffer = "\0\0";
					prependLength = 2;
				} 
			} else if (f->subclass & AST_FORMAT_H264) {
				/* All intra */
				intra = 1;
				/* Save all to the rtp payload */
				payload = f->datalen;
				/* Don't add frame data to payload */
				skip = f->datalen;
				/* And add the data to the frame but not associated with the hint track */
				prependBuffer = f->data+1;
				prependLength = f->datalen-1;
			} else {
				/* Unknown codec nothing to do */
				break;
			}

			/* Write rtp video packet */
			mp4_rtp_write_video(&videoTrack, f, payload, intra, skip, prependBuffer , prependLength);

		} else if (f->frametype == AST_FRAME_DTMF) {
		}

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
	int res;

	res = ast_unregister_application(app_play);
	res &= ast_unregister_application(app_save);

	ast_module_user_hangup_all();

	return res;
}

static int load_module(void)
{
	int res;

	res = ast_register_application(app_save, mp4_save, syn_save, des_save);
	res &= ast_register_application(app_play, mp4_play, syn_play, des_play);
	return 0;
}

AST_MODULE_INFO_STANDARD(ASTERISK_GPL_KEY, "MP4 applications");

