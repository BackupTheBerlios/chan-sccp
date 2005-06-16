/*
 * Asterisk - SCCP Support (By Zozo)
 * ---------------------------------------------------------------------------
 * sccp_helper.c - SCCP Helper Functions
 * 
 * $Id: sccp_helper.c,v 1.1 2005/06/16 10:12:51 cherso Exp $
 *
 */

#include "chan_sccp.h"
#include "sccp_helper.h"
#include "sccp_device.h"
#include <unistd.h>
#include <asterisk/app.h>

AST_MUTEX_DEFINE_STATIC(intercomlock);

const char * sccpmsg2str(int e) {
	const value_string * v = message_list;
	while (v) {
		if (v->key == e)
			return v->value;
		v++;
	}
	return "Unknown";
}

int sccp_line_hasmessages(sccp_line_t * l) {
	int newmsgs, oldmsgs;
	int totalmsgs = 0;
	char tmp[AST_MAX_EXTENSION] = "", * mb, * cur;
	strncpy(tmp, l->mailbox, sizeof(tmp));
	mb = tmp;

	while((cur = strsep(&mb, ","))) {
		if (strlen(cur)) {
			if (sccp_debug > 2)
				ast_verbose(VERBOSE_PREFIX_3 "Checking mailbox: %s\n", cur);
			ast_app_messagecount(cur, &newmsgs, &oldmsgs);
			totalmsgs += newmsgs;
		}
	}
	return totalmsgs;
}

sccp_intercom_t * sccp_intercom_find_byname(char * name) {
	sccp_intercom_t * i = NULL;

	ast_mutex_lock(&intercomlock);
	i = intercoms;
	while(i) {
		if (!strcasecmp(name, i->id))
			break;
		i = i->next;
	}
	ast_mutex_unlock(&intercomlock);
	return i;
}

char * sccp_helper_getversionfor(sccp_session_t * s) {
	return (strlen(s->device->imgversion) > 0) ? s->device->imgversion : "000";
}

int sccp_codec_ast2cisco(int fmt) {
	const codec_def * v = codec_list;
	while (v->value) {
		if (v->astcodec == fmt)
			return v->key;
		v++;
	}
	return 0;
}
