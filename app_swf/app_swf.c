/*
 * Asterisk -- An open source telephony toolkit.
 *
 * Copyright (C) 2005, Sergio Garcia Murillo
 *
 * http://sip.fontventa.com
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
 * \brief Mixer test application -- 
 *
 * \author Sergio Garcia Murillo <sergio.garcia@fontventa.com>
 * 
 * \ingroup applications
 */

#include "asterisk.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "asterisk/lock.h"
#include "asterisk/file.h"
#include "asterisk/logger.h"
#include "asterisk/channel.h"
#include "asterisk/pbx.h"
#include "asterisk/module.h"
#include "asterisk/rtp.h"

#include <mcu/xmlrpcflashclient.h>

static char *app = "swf_play";
static char *synopsis = "Play sfw";
static char *descrip = "  swf_play(url): Play swf\n";

static struct sched_context *sched;     /*!< The scheduling context */
static struct io_context *io;           /*!< The IO context */

void *flash_player;

static int swf_play_exec(struct ast_channel *chan, void *data)
{
	int format;
	int id;
	int audioPort;
	int videoPort;
	struct ast_module_user *u;
	struct ast_rtp *audio;
	struct ast_rtp *video;
	struct sockaddr_in addr;
	int infds[2];
	int outfd;
	int ms;

	u = ast_module_user_add(chan);

	/* log */
	ast_log(LOG_WARNING,">flash play [%x]\n",chan);

	format = ast_best_codec(chan->nativeformats);
	ast_set_write_format(chan, format);
	ast_set_read_format(chan, format);

	/* Create audio and video rtp sockets */
	audio = ast_rtp_new(sched,io,0,0);
	video = ast_rtp_new(sched,io,0,0);

	/* Get local port address */
	ast_rtp_get_us(audio,&addr);
	audioPort = ntohs(addr.sin_port);
	ast_rtp_get_us(video,&addr);
	videoPort = ntohs(addr.sin_port);

	/* log */
	ast_log(LOG_WARNING,"-FlashStartPlaying [%s]\n",(char*)data);

	/* Start playing*/
	id = FlashStartPlaying(flash_player,"127.0.0.1",audioPort,videoPort,data);

	/* Fill fd array */
	infds[0] = ast_rtp_fd(audio);
	infds[1] = ast_rtp_fd(video);

	/* log */
	ast_log(LOG_WARNING,"Loop [%x]\n",chan);

	/* Loop */
	while (1)
	{
		/* No output */
		outfd = -1;
		/* 10 seconds timeout */
		ms = 10000;
		/* Read from channels and fd*/
		if(ast_waitfor_nandfds(&chan,1,infds,2,NULL,&outfd,&ms))
		{
			/* Got data in channel */
			struct ast_frame *f = ast_read(chan);
			/* Check for hangup */
			if (!f)
				/* Exit */
				goto end;
			/* Depending on type*/
			switch (f->frametype)
			{
				default:
					/* Free frame */
					ast_frfree(f);
			}
		} else if (outfd==infds[0]) {
			/* Read audio */
			struct ast_frame *f = ast_rtp_read(audio);
			/* Send it now */
			f->delivery.tv_sec = 0;
			f->delivery.tv_usec = 0;
			/* send to channel */
			ast_write(chan,f);
		} else if (outfd==infds[1]) {
			/* Read video */
			struct ast_frame *f = ast_rtp_read(video);
			/* Send it now */
			f->delivery.tv_sec = 0;
			f->delivery.tv_usec = 0;
			/* send to channel */
			ast_write(chan,f);
		}
	}
end:
	ast_log(LOG_WARNING,"-FlashStopPlaying\n");
	/* Stop Playback */
	FlashStopPlaying(flash_player,id);
	/* log */
	ast_log(LOG_WARNING,"<swf_play\n");

	/* Exit */
	ast_module_user_remove(u);
	return -1;
}

static int unload_module(void)
{
	int res;

	res = ast_unregister_application(app);

	ast_module_user_hangup_all();

	/* Destroy Flash proxy */
	DestroyFlashClient(flash_player);

	sched_context_destroy(sched);
	io_context_destroy(io);

	return res;
}

static int load_module(void)
{

	if (!(sched = sched_context_create())) {
		ast_log(LOG_ERROR, "Unable to create scheduler context\n");
		return 	AST_MODULE_LOAD_FAILURE;
	}

	if (!(io = io_context_create())) {
		ast_log(LOG_ERROR, "Unable to create I/O context\n");
		sched_context_destroy(sched);
		return AST_MODULE_LOAD_FAILURE;
	}

	/* Create Flash player proxy */
	flash_player = CreateFlashClient("http://127.0.0.1:8080/flash");

	return ast_register_application(app, swf_play_exec, synopsis, descrip);
}

AST_MODULE_INFO_STANDARD(ASTERISK_GPL_KEY, "Swf playback Application");
