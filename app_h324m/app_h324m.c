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

static char *name_h324m_loopback = "h324m_loopback";
static char *syn_h324m_loopback = "H324m loopback mode";
static char *des_h324m_loopback = "  h324m_loopback():  Estabish connection and loopback media.\n";

static char *name_h324m_gw = "h324m_gw";
static char *syn_h324m_gw = "H324m gateway";
static char *des_h324m_gw = "  h324m_gw():  Creates a pseudo channel for an incoming h324m call.\n";

static char *name_video_loopback = "video_loopback";
static char *syn_video_loopback = "video_loopback";
static char *des_video_loopback = "  video_loopback():  Video loopback.\n";


static int app_h324m_loopback(struct ast_channel *chan, void *data)
{
	struct ast_frame *f;
	struct ast_module_user *u;
	void*  frame;

	ast_log(LOG_DEBUG, "h324m_loopback\n");

	/* Lock module */
	u = ast_module_user_add(chan);

	/* Create session */
	void* id = H324MSessionCreate();

	/* Init session */
	H324MSessionInit(id);

	/* Wait for data avaiable on channel */
	while (ast_waitfor(chan, -1) > -1) {

		/* Read frame from channel */
		f = ast_read(chan);

		/* if it's null */
		if (f == NULL)
			break;

		/* Check frame type */
		if (f->frametype == AST_FRAME_VOICE) {
			/* read data */
			H324MSessionRead(id, (unsigned char *)f->data, f->datalen);
			/* Get frames */
			while ((frame=H324MSessionGetFrame(id))!=NULL)
			{
				/* If it's video */
				if (FrameGetType(frame)==MEDIA_VIDEO)
					/* Send it back */
					H324MSessionSendFrame(id,frame);
				/* Delete frame */
				FrameDestroy(frame);
			}
			/* write data */
			H324MSessionWrite(id, (unsigned char *)f->data, f->datalen);
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
	void*  frame;
	int    reason = 0;
	int    ms;
	int    res;
	struct ast_channel *channels[2];
	struct ast_channel *pseudo;
	struct ast_channel *where;
	unsigned char* framedata;
	unsigned int framelength;

	ast_log(LOG_DEBUG, "h324m_loopback\n");

	/* Lock module */
	u = ast_module_user_add(chan);

	/* Request new channel */
	pseudo = ast_request("Local", 0xffffffff, data, &reason);
 
	printf("[%x]\n",pseudo);fflush(stdout);

	/* If somthing has gone wrong */
	if (!pseudo)
		/* goto end */
		goto end; 

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
					/* record but keep going */
					reason = f->subclass;
					break;
				case AST_CONTROL_BUSY:
				case AST_CONTROL_CONGESTION:
					/* Save cause */
					reason = f->subclass;
					/* exit */
					goto hangup_pseudo;
					break;
				case AST_CONTROL_ANSWER:
					/* Save cause */
					reason = f->subclass;	
					break;
			}
		}
		/* Delete frame */
		ast_frfree(f);
	}

	printf("[%x,%x]\n",pseudo->_state,AST_STATE_UP);fflush(stdout);

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

	printf("Answering\n");

	/* Set up array */
	channels[0] = chan;
	channels[1] = pseudo;

	/* No timeout */
	ms = -1;

	/* Wait for data avaiable on any channel */
	while ((where = ast_waitfor_n(channels, 2, &ms)) != NULL) {
		/* Read frame from channel */
		f = ast_read(where);

		/* if it's null */
		if (f == NULL)
			break;

		/* If it's on h324m channel */
		if (where==chan) {
			/* Check frame type */
			if (f->frametype == AST_FRAME_VOICE) {
				/* read data */
				H324MSessionRead(id, (unsigned char *)f->data, f->datalen);
				/* Get frames */
				while ((frame=H324MSessionGetFrame(id))!=NULL)
				{
					/* TODO!!! */
					/* Packetize outgoing frame */
					/* Delete frame */
					FrameDestroy(frame);
				}
				/* write data */
				H324MSessionWrite(id, (unsigned char *)f->data, f->datalen);
				/* deliver now */
				f->delivery.tv_usec = 0;
				f->delivery.tv_sec = 0;
				/* write frame */
				ast_write(chan, f);
			} else 
				/* Delete frame */
				ast_frfree(f);
		} else {
			/* Depending on the type */
			switch (f->frametype)
			{
				case AST_FRAME_VOICE:
					/* Create frame */
					frame = FrameCreate(MEDIA_AUDIO, CODEC_AMR, (unsigned char *)f->data, f->datalen);
					/* Send frame */
					H324MSessionSendFrame(id,frame);
					/* Delete frame */
					FrameDestroy(frame);
					break;
				case AST_FRAME_VIDEO:
					/* Depending on the codec */
					if (f->subclass & AST_FORMAT_H263) 
					{
						/* Get data & length without rfc 2190 (only A packets ) */
						framedata = (unsigned char *)f->data+4;
						framelength = f->datalen-4;
					} else if (f->subclass & AST_FORMAT_H263_PLUS) {
						/* Get initial data */
						framedata = (unsigned char *)f->data;
						framelength = f->datalen;
						/* Get header */
						unsigned char p = framedata[0] & 0x04;
						unsigned char v = framedata[0] & 0x02;
						unsigned char plen = ((framedata[0] & 0x1 ) << 5 ) || (framedata[1] >> 3);
						unsigned char pebit = framedata[0] & 0x7;
						/* skip header*/
						framedata += 2;
						framelength += 2;
						/* Check */
						if (v)
						{
							/* Increase ini */
							framedata++;
							framelength--;
						} else {
						}
						/* Check p bit */
						if (p || plen)
						{
							/* Decrease ini */
							framedata -= 2;
							framelength -= 2;
							/* Append 0s */	
							framedata[0] = 0;
							framedata[1] = 0;
						}
					} else
						break;
					/* Create frame */
					frame = FrameCreate(MEDIA_VIDEO, CODEC_H263, framedata, framelength);
					/* Send frame */
					H324MSessionSendFrame(id,frame);
					/* Delete frame */
					FrameDestroy(frame);
					break;
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
	ast_channel_free(pseudo);

end:
	/* Hangup channel if needed */
	ast_softhangup(chan, reason);

	/* Unlock module*/
	ast_module_user_remove(u);

	//Exit
	return 0;
}

static int app_video_loopback(struct ast_channel *chan, void *data)
{
	struct ast_frame *f;
	struct ast_module_user *u;

	ast_log(LOG_DEBUG, "video_loopback\n");

	/* Lock module */
	u = ast_module_user_add(chan);

	/* Wait for data avaiable on channel */
	while (ast_waitfor(chan, -1) > -1) {

		/* Read frame from channel */
		f = ast_read(chan);

		/* if it's null */
		if (f == NULL)
			break;

		/* Check frame type */
		if (f->frametype == AST_FRAME_VIDEO) {
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

	//Exit
	return 0;
}

static int unload_module(void)
{
	int res;

	res = ast_unregister_application(name_h324m_loopback);
	res &= ast_unregister_application(name_h324m_gw);
	res &= ast_unregister_application(name_video_loopback);

	ast_module_user_hangup_all();

	return res;
}

static int load_module(void)
{
	int res;

	res = ast_register_application(name_h324m_loopback, app_h324m_loopback, syn_h324m_loopback, des_h324m_loopback);
	res &= ast_register_application(name_h324m_gw, app_h324m_gw, syn_h324m_gw, des_h324m_gw);
	res &= ast_register_application(name_video_loopback, app_video_loopback, syn_video_loopback, des_video_loopback);
	return 0;
}

AST_MODULE_INFO_STANDARD(ASTERISK_GPL_KEY, "H324M stack");

