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

#include <mcu/xmlrpcmcuclient.h>

static char *app = "mixer_join";
static char *synopsis = "Add participant to an existing mixing conference";
static char *descrip = "  mixer_join(confId): Add participant to conference\n";

static struct sched_context *sched;     /*!< The scheduling context */
static struct io_context *io;           /*!< The IO context */

void *mcu;
int room;

static int join_exec(struct ast_channel *chan, void *data)
{
	int format;
	int partId;
	int audioPort;
	int videoPort;
	int remoteAudioPort;
	int remoteVideoPort;
	struct ast_module_user *u;
	struct ast_rtp *audio;
	struct ast_rtp *video;
	struct sockaddr_in addr;
	int infds[2];
	int outfd;
	int ms;
	int comp=0;

	u = ast_module_user_add(chan);

	/* log */
	ast_log(LOG_WARNING,">mixer_joini [%x]\n",chan);

	format = ast_best_codec(chan->nativeformats);
	ast_set_write_format(chan, format);
	ast_set_read_format(chan, format);

	/* log */
	ast_log(LOG_WARNING,"Adding participant [%x]\n",chan);

	/* Create participant */
	partId = CreateParticipant(mcu,room);

	/* Create audio and video rtp sockets */
	audio = ast_rtp_new(sched,io,0,0);
	video = ast_rtp_new(sched,io,0,0);

	/* Get local port address */
	ast_rtp_get_us(audio,&addr);
	audioPort = ntohs(addr.sin_port);
	ast_rtp_get_us(video,&addr);
	videoPort = ntohs(addr.sin_port);

	/* Set codecs */
	SetVideoCodec(mcu,room,partId,H263,CIF,300,5);
	SetAudioCodec(mcu,room,partId,PCMU);

	/* Start sending in mcu side*/
	StartSendingAudio(mcu,room,partId,"127.0.0.1",audioPort);
	StartSendingVideo(mcu,room,partId,"127.0.0.1",videoPort);

	/* Start receiving in mcu side*/
	StartReceivingAudio(mcu,room,partId,&remoteAudioPort);
	StartReceivingVideo(mcu,room,partId,&remoteVideoPort);

	/* Set send data */
	addr.sin_family      = AF_INET;
	addr.sin_addr.s_addr = INADDR_ANY;
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");

	/* Set peer addres of local rtp */
	addr.sin_port = htons(remoteAudioPort);
	ast_rtp_set_peer(audio,&addr);
	addr.sin_port = htons(remoteVideoPort);
	ast_rtp_set_peer(video,&addr);

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
			/* Send it now */
			f->delivery.tv_sec = 0;
			f->delivery.tv_usec = 0;
			/* Depending on type*/
			switch (f->frametype)
			{
				case AST_FRAME_VOICE:
					/* Send to mixer */
					ast_rtp_write(audio,f);
					break;
				case AST_FRAME_VIDEO:
					/* Send to mixer */
					ast_rtp_write(video,f);
					break;
				case AST_FRAME_DTMF:
					/* Get dtmf */
					if (f->subclass == '#') {
						/* Loop composition tipe */
						if (comp==2)
							comp = 0;
						else
							comp++;
						/* Set it*/
						SetCompositionType(mcu,room,comp,CIF);	
					}
					/* Free frame */
					ast_frfree(f);
					break;
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
	/* log */
	ast_log(LOG_WARNING,"Removing participant\n");
	/* Remove Participant */
	DeleteParticipant(mcu,room,partId);
	/* log */
	ast_log(LOG_WARNING,"<mixer_join\n");

	/* Exit */
	ast_module_user_remove(u);
	return -1;
}

static int unload_module(void)
{
	int res;

	res = ast_unregister_application(app);

	ast_module_user_hangup_all();

	/* Delete default room */
	DeleteConference(mcu,room);

	/* Destroy MCU proxy */
	DestroyMCUClient(mcu);

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

	/* Create MCU proxy */
	mcu = CreateMCUClient("http://127.0.0.1:8080/mcu");

	/* Create default room */
	room = CreateConference(mcu,"Default Room");

	return ast_register_application(app, join_exec, synopsis, descrip);
}

AST_MODULE_INFO_STANDARD(ASTERISK_GPL_KEY, "Mixer Test Application");
