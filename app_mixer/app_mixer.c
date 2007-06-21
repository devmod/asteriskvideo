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

#include <xmlrpcproxyclient.h>

static char *app = "mixer_join";
static char *synopsis = "Add participant to an existing mixing conference";
static char *descrip = "  mixer_join(confId): Add participant to conference\n";

void *mcu;
int room;


static int join_exec(struct ast_channel *chan, void *data)
{
	int res = -1;
	int format;
	int partId;
	int audioPort;
	int videoPort;
	struct ast_module_user *u;
	struct ast_rtp *audio;
	struct ast_rtp *video;
	struct sockaddr_in *addr;

	u = ast_module_user_add(chan);

	format = ast_best_codec(chan->nativeformats);
	ast_set_write_format(chan, format);
	ast_set_read_format(chan, format);

	/* Create participant */
	partId = CreateParticipant(mcu,room);

	/* Set codecs */


	/* Create audio and video rtp sockets */
	audio = ast_rtp_new(NULL,NULL,0,0);
	video = ast_rtp_new(NULL,NULL,0,0);

	/* Get local port address */
	ast_rtp_get_us(audio,addr);
	audioPort = ntohs(addr->sin_port);
	ast_rtp_get_us(video,addr);
	videoPort = ntohs(addr->sin_port);

	/* Start sending */

	/* Start receiving */

	while (ast_waitfor(chan, -1) > -1) {
		struct ast_frame *f = ast_read(chan);
		if (!f)
			break;
		f->delivery.tv_sec = 0;
		f->delivery.tv_usec = 0;
		switch (f->frametype) {
		case AST_FRAME_DTMF:
			if (f->subclass == '#') {
				res = 0;
				ast_frfree(f);
				goto end;
			}
			/* fall through */
		default:
			if (ast_write(chan, f)) {
				ast_frfree(f);
				goto end;
			}
		}
		ast_frfree(f);
	}
end:
	ast_module_user_remove(u);
	return res;
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

	return res;
}

static int load_module(void)
{
	/* Create MCU proxy */
	mcu = CreateMCUClient("http://127.0.0.1:8080/");

	/* Create default room */
	room = CreateConference(mcu,"Default Room");

	return ast_register_application(app, join_exec, synopsis, descrip);
}

AST_MODULE_INFO_STANDARD(ASTERISK_GPL_KEY, "Mixer Test Application");
