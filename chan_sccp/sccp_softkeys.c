#include "chan_sccp.h"
#include "sccp_actions.h"
#include "sccp_helper.h"
#include "sccp_pbx.h"
#include "sccp_channel.h"
#include "sccp_device.h"
#include "sccp_line.h"
#include <asterisk/musiconhold.h>


void sccp_sk_redial(sccp_device_t * d , sccp_line_t * l, sccp_channel_t * c) {
	ast_log(LOG_DEBUG, "Redial Softkey.\n");

	if (!d->lastNumberLine)
		return;

	l = sccp_line_find_byid(d, d->lastNumberLine);

	if (l)
		sccp_line_dial(l, d->lastNumber);
}

void sccp_sk_newcall(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c) {
	ast_log(LOG_DEBUG, "Starting new call....\n");

	if (!l)
		l = d->currentLine;

	c = sccp_dev_allocate_channel(d, l, 1, NULL);

	if (!c) {
		ast_log(LOG_ERROR, "Failed to allocate channel\n");
		return;
	}

	sccp_dev_set_speaker(l->device, StationSpeakerOn);
	sccp_dev_statusprompt_set(l->device, c, NULL, 0);
	sccp_dev_set_keyset(l->device, c, KEYMODE_OFFHOOK);
	sccp_dev_set_sptone(l->device, "InsideDialTone");
}

void sccp_sk_endcall(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c) {
	if (!c) {
		ast_log(LOG_WARNING, "Tried to EndCall while no call in progress.\n");
		return;
	}

	sccp_channel_endcall(c);
}


void sccp_sk_addline(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c) {
	ast_log(LOG_DEBUG, "### AddLine Softkey pressed - NOT SUPPORTED\n");
}

void sccp_sk_dnd(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c) {
	ast_mutex_lock(&d->lock);

	d->dnd = (d->dnd) ? 0 : 1;

	if (d->dnd) {
		sccp_dev_statusprompt_set(d, NULL, "DND is On", 0);
	} else {
		sccp_dev_statusprompt_set(d, NULL, "DND is Off", 5);
	}

	ast_mutex_unlock(&d->lock);
}


void sccp_sk_back(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c) {
	ast_log(LOG_DEBUG, "### Back Softkey pressed - NOT SUPPORTED\n");
}

void sccp_sk_dial(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c) {
	ast_log(LOG_DEBUG, "### Dial Softkey pressed - NOT SUPPORTED\n");
}

void sccp_sk_clear(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c) {
	ast_log(LOG_DEBUG, "### Clear Softkey pressed - NOT SUPPORTED\n");
}

void sccp_sk_answer(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c) {
	ast_log(LOG_DEBUG, "### Answer Softkey pressed\n");

	if (d && d->session) {
		if ((c->line->dnState != TsOnHook)&&(c->line->dnState != TsRingIn)) {
			// Handle stuff when already one call in progress
			sccp_channel_set_lamp(c, LampBlink);             // Wink Message
			sccp_channel_set_callstate(c, TsHold);          // It's HOLD!
			sccp_dev_set_keyset(d,c, KEYMODE_ONHOLD);       // KeyPad Select
			sccp_dev_statusprompt_set(d,c,"Call On Hold",0);  // Hold it
			sccp_channel_StatisticsRequest(c);              // Rquest statistics
			sccp_dev_set_keyset(d,c, KEYMODE_ONHOLD);       // KeyPad Select
			sccp_channel_disconnect(c);                     // Disconnect Channel
#ifdef ASTERISK_VERSION_v1_0
			ast_moh_start(c->owner->bridge, NULL);                          // Start Moh,
#endif
#ifdef ASTERISK_VERSION_HEAD
			ast_moh_start(ast_bridged_channel(c->owner), NULL);
#endif
		}

		sccp_handle_offhook(d->session,NULL,c);
	}
}

void sccp_sk_reject(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c) {
	ast_log(LOG_DEBUG, "### Reject Softkey pressed - NOT SUPPORTED\n");
}

void sccp_sk_hold(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c) {
	if (!c) {
		ast_log(LOG_DEBUG, "### Hold Softkey while no call -> IGNORED\n");
		sccp_dev_statusprompt_set(d,c,"No call to put on hold.",5);
		return;
	}

/* prevent segmentation fault when no bridged call */
#ifdef ASTERISK_VERSION_v1_0
	if (!c->owner->bridge) {
#endif
#ifdef ASTERISK_VERSION_HEAD
	if (!ast_bridged_channel(c->owner)) {
#endif
		ast_log(LOG_DEBUG, "### Hold Softkey while no bridged call -> IGNORED\n");
		sccp_dev_statusprompt_set(d,c,"Cannot put this type of call on hold.",5);
		return;
}

	ast_log(LOG_DEBUG, "### Hold Softkey pressed\n");
	sccp_channel_set_lamp(c, LampBlink);            // Wink Message
	sccp_channel_set_callstate(c, TsHold);          // It's HOLD!
	sccp_dev_statusprompt_set(d,c,"Call On Hold",0);  // Hold it
	sccp_channel_StatisticsRequest(c);              // Rquest statistics
	sccp_dev_set_keyset(d,c, KEYMODE_ONHOLD);       // KeyPad Select
	sccp_channel_disconnect(c);                     // Disconnect Channel
	d->active_channel = NULL;                       // Don't want to hang up on a held call

#ifdef ASTERISK_VERSION_v1_0
	ast_moh_start(c->owner->bridge, NULL);          // Start Moh,
#endif
#ifdef ASTERISK_VERSION_HEAD
	ast_moh_start(ast_bridged_channel(c->owner), NULL);
#endif

}

void sccp_sk_conference(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c) {
	ast_log(LOG_DEBUG, "### Conference Softkey pressed - NOT SUPPORTED\n");
}

void sccp_sk_transfer(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c) {
	if (c) {
		ast_log(LOG_DEBUG, "### Transfer softkey pressed, faking by sending #\n");
		sccp_pbx_senddigit(c,'#');
	} else {
		ast_log(LOG_DEBUG, "### Transfer when there is no active call\n");
	}
}

void sccp_sk_blindxfr(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c) {
	ast_log(LOG_DEBUG, "### Blind Xfer Softkey pressed - NOT SUPPORTED\n");
}

void sccp_sk_cfwd_all(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c) {
	ast_log(LOG_DEBUG, "### CFwdAll Softkey pressed - NOT SUPPORTED\n");
}

void sccp_sk_cfwd_busy(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c) {
	ast_log(LOG_DEBUG, "### CFwdBusy Softkey pressed - NOT SUPPORTED\n");
}

void sccp_sk_cfwd_noanswer(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c) {
	ast_log(LOG_DEBUG, "### CFwdNoAnswer Softkey pressed - NOT SUPPORTED\n");
}

void sccp_sk_resumecall(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c) {
	// This is simply the reverse of the Hold Function.
	if (c) {
		ast_log(LOG_DEBUG, "### ResumeCall Softkey pressed\n");
#ifdef ASTERISK_VERSION_v1_0
		if (c->owner->bridge)
			ast_moh_stop(c->owner->bridge);           // Stop Moh,
#endif
#ifdef ASTERISK_VERSION_HEAD
		if (ast_bridged_channel(c->owner))
			ast_moh_stop(ast_bridged_channel(c->owner));
#endif
		d->active_channel = c;
		sccp_channel_connect(c);                          // Re-connect Channel
		sccp_dev_statusprompt_set(d,c,"Call Resumed",0);  // Indicate Resumed
		sccp_dev_set_keyset(d,c, KEYMODE_CONNECTED);      // KeyPad Select
		sccp_channel_set_callstate(c, TsConnected);       // It's connected
		sccp_channel_set_lamp(c, LampOn);                 // Wink Message Off
	} else {
		ast_log(LOG_DEBUG, "### ResumeCall Softkey but no Call present - Ignoring \n");
	}
}

void sccp_sk_info(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c) {
	ast_log(LOG_DEBUG, "### Info Softkey pressed - NOT SUPPORTED\n");
}

void sccp_sk_parkcall(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c) {
	ast_log(LOG_DEBUG, "### Park Softkey pressed - NOT SUPPORTED\n");
}

void sccp_sk_join(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c) {
	ast_log(LOG_DEBUG, "### Join Softkey pressed - NOT SUPPORTED\n");
}

void sccp_sk_meetme(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c) {
	ast_log(LOG_DEBUG, "### Meetme Softkey pressed - NOT SUPPORTED\n");
}

void sccp_sk_pickup(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c) {
	ast_log(LOG_DEBUG, "### CallPickup Softkey pressed - NOT SUPPORTED\n");
}

void sccp_sk_pickup_group(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c) {
	ast_log(LOG_DEBUG, "### CallPickupGroup Softkey pressed - NOT SUPPORTED\n");
}
