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

static char *app_loop = "h324m_loopback";
static char *syn_loop = "H324m loopback mode";
static char *des_loop = "  h324m_loopback():  Estabish connection and loopback media.\n";


static int app_h324m_loopback(struct ast_channel *chan, void *data)
{
	struct ast_frame *f;
	struct ast_module_user *u;

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
	int  H324MSessionEnd(void * id);

	/* Destroy session */
	H324MSessionDestroy(id);

	ast_log(LOG_DEBUG, "exit");

	/* Unlock module*/
	ast_module_user_remove(u);

	//Exit
	return 0;
}

static int unload_module(void)
{
	int res;

	res = ast_unregister_application(app_loop);

	ast_module_user_hangup_all();

	return res;
}

static int load_module(void)
{
	int res;

	res = ast_register_application(app_loop, app_h324m_loopback, syn_loop, des_loop);
	return 0;
}

AST_MODULE_INFO_STANDARD(ASTERISK_GPL_KEY, "H324M stack");

