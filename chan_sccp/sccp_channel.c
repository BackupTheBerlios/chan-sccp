#include "chan_sccp.h"
#include "sccp_actions.h"
#include "sccp_helper.h"
#include "sccp_device.h"
#include "sccp_pbx.h"
#include "sccp_channel.h"
#include <asterisk/callerid.h>
#include <asterisk/acl.h>

AST_MUTEX_DEFINE_STATIC(chanlock);

sccp_channel_t * sccp_channel_find_byid(int id) {
  sccp_channel_t * c = NULL;
  ast_mutex_lock(&chanlock);
  c = chans;
  while (c) {
    if (c->callid == id)
      break;
    c = c->lnext;
  }
  if (c->callid != id) {
    if (sccp_debug)
      ast_log(LOG_DEBUG,"Could not find channel id %d",id);
      return NULL;
  }
  ast_mutex_unlock(&chanlock);
  return c;
}

void sccp_channel_send_callinfo(sccp_channel_t * c) {
  sccp_moo_t * r;

  if (!c->owner) {
    ast_log(LOG_ERROR, "Call doesn't have an owner!\n");
    return;
  }

  if (c->sentCallInfo) {
     ast_log(LOG_DEBUG, "CallInfo already sent... skip it!\n");
     return;
  }

  REQ(r, CallInfoMessage);

  if (c->callingPartyName)
    strncpy(r->msg.CallInfoMessage.callingPartyName, c->callingPartyName, StationMaxNameSize - 1);
  if (c->callingPartyNumber)
    strncpy(r->msg.CallInfoMessage.callingParty, c->callingPartyNumber, StationMaxDirnumSize - 1);

  if (c->calledPartyName)
    strncpy(r->msg.CallInfoMessage.calledPartyName, c->calledPartyName, StationMaxNameSize - 1);
  if (c->calledPartyNumber)
    strncpy(r->msg.CallInfoMessage.calledParty, c->calledPartyNumber, StationMaxDirnumSize - 1);


  r->msg.CallInfoMessage.lel_lineId   = htolel(c->line->instance);
  r->msg.CallInfoMessage.lel_callRef  = htolel(c->callid);
  r->msg.CallInfoMessage.lel_callType = htolel((c->isOutgoing) ? 2 : 1); // XXX:T: Need to add 3, for Forarded call.
  sccp_dev_send(c->line->device, r);
  c->sentCallInfo = 1;
}

// XXX:T: whoops - need to move to device.
void sccp_channel_set_callstate(sccp_channel_t * c, StationDCallState state) {
  sccp_moo_t * r;

  // XXX:T: Assert!

  REQ(r, CallStateMessage)
  r->msg.CallStateMessage.lel_callState = htolel(state);
  r->msg.CallStateMessage.lel_lineInstance  = htolel((c) ? c->line->instance : 0);
  r->msg.CallStateMessage.lel_callReference = htolel((c) ? c->callid : 0);

  if (sccp_debug >= 2)
    ast_verbose(VERBOSE_PREFIX_2 "{CallStateMessage} callState=%s(%d), lineInstance=%d, callReference=%d\n", 
      TsCallStatusText[state],
      letohl(r->msg.CallStateMessage.lel_callState),
      letohl(r->msg.CallStateMessage.lel_lineInstance),
      letohl(r->msg.CallStateMessage.lel_callReference));

  sccp_dev_send(c->line->device, r);

  if (c->line->instance)
    c->line->dnState = state;

}

void sccp_channel_set_callingparty(sccp_channel_t * c, char *name, char *number) {
	if (!c)
		return;

	if (name && strncmp(name, c->callingPartyName, StationMaxNameSize - 1)) {
#ifdef ASTERISK_VERSION_HEAD
		ast_copy_string(c->callingPartyName, name, StationMaxNameSize);
#endif
#ifdef ASTERISK_VERSION_v1_0
		strncpy(c->callingPartyName, name, StationMaxNameSize - 1);
#endif
		/* we are changing something in the Party Name or number so callinfo func should send info */
		c->sentCallInfo = 0;
		if (sccp_debug >= 3)
			ast_verbose(VERBOSE_PREFIX_3 "Set callingParty Name: %s\n", c->callingPartyName);
	}

	if (number && strncmp(number, c->callingPartyNumber, StationMaxDirnumSize - 1)) {
#ifdef ASTERISK_VERSION_HEAD
		ast_copy_string(c->callingPartyNumber, number, StationMaxDirnumSize);
#endif
#ifdef ASTERISK_VERSION_v1_0
		strncpy(c->callingPartyNumber, number, StationMaxDirnumSize - 1);
#endif

		c->sentCallInfo = 0;

		if (sccp_debug >= 3)
			ast_verbose(VERBOSE_PREFIX_3 "Set callingParty Number %s\n", c->callingPartyNumber);
	}

	return;
}

void sccp_channel_set_calledparty(sccp_channel_t * c, char *name, char *number) {
	if (!c)
		return;

	if (name && strncmp(name, c->calledPartyName, StationMaxNameSize - 1)) {
#ifdef ASTERISK_VERSION_HEAD
		ast_copy_string(c->calledPartyName, name, StationMaxNameSize);
#endif
#ifdef ASTERISK_VERSION_v1_0
		strncpy(c->calledPartyName, name, StationMaxNameSize - 1);
#endif
		/* we are changing something in the Party Name or number so callinfo func should send info */
		c->sentCallInfo = 0;
		if (sccp_debug >= 3)
			ast_verbose(VERBOSE_PREFIX_3 "Set calledParty Name: %s\n", c->calledPartyName);
	}

	if (number && strncmp(number, c->calledPartyNumber, StationMaxDirnumSize - 1)) {
#ifdef ASTERISK_VERSION_HEAD
		ast_copy_string(c->calledPartyNumber, number, StationMaxDirnumSize);
#endif
#ifdef ASTERISK_VERSION_v1_0
		strncpy(c->calledPartyNumber, number, StationMaxDirnumSize - 1);
#endif
		c->sentCallInfo = 0;
		if (sccp_debug >= 3)
			ast_verbose(VERBOSE_PREFIX_3 "Set calledParty Number %s\n", c->calledPartyNumber);
	}

	return;
}

void sccp_channel_set_lamp(sccp_channel_t * c, int lampMode) {
	sccp_moo_t * r;
	REQ(r, SetLampMessage)
	r->msg.SetLampMessage.lel_stimulus = htolel(0x9);
	r->msg.SetLampMessage.lel_stimulusInstance = htolel(c->line->instance);
	r->msg.SetLampMessage.lel_lampMode = htolel(lampMode);
	sccp_dev_send(c->line->device, r);
}

void sccp_channel_StatisticsRequest(sccp_channel_t * c) {
  sccp_moo_t * r;

  if (!c)
    return;

  REQ(r, ConnectionStatisticsReq)

  /* XXX need to test what we have to copy in the DirectoryNumber */
  if (c->isOutgoing)
    strncpy(r->msg.ConnectionStatisticsReq.DirectoryNumber, c->calledPartyNumber, StationMaxDirnumSize - 1);
  else
    strncpy(r->msg.ConnectionStatisticsReq.DirectoryNumber, c->callingPartyNumber, StationMaxDirnumSize - 1);

  r->msg.ConnectionStatisticsReq.lel_callReference = htolel((c) ? c->callid : 0);
  r->msg.ConnectionStatisticsReq.lel_StatsProcessing = htolel(0);
  sccp_dev_send(c->line->device, r);


  if (sccp_debug)
    ast_verbose(VERBOSE_PREFIX_2 "Requesting CallStatisticsAndClear from Phone");
}

void sccp_channel_connect(sccp_channel_t * c) {
  sccp_moo_t * r;
  struct sockaddr_in sin;
  unsigned char m[3] = "";
  // Have to do variable declaration here for GCC 2.95 support
#ifdef ASTERISK_VERSION_HEAD
  struct ast_hostent ahp;
  struct hostent *hp;
#endif
  int payloadType = sccp_codec_ast2cisco(c->owner->readformat);

  ast_rtp_get_us(c->rtp, &sin);

#ifdef ASTERISK_VERSION_HEAD
// Very hackish way to set endpoint's IP
  hp = ast_gethostbyname(c->device->session->in_addr, &ahp);
  memcpy(&sin.sin_addr, hp->h_addr, sizeof(sin.sin_addr));
#endif

  REQ(r, OpenReceiveChannel)
  r->msg.OpenReceiveChannel.lel_conferenceId = htolel(c->callid);
  r->msg.OpenReceiveChannel.lel_passThruPartyId = htolel((uint32_t)c) ;
  r->msg.OpenReceiveChannel.lel_millisecondPacketSize = htolel(20);
  r->msg.OpenReceiveChannel.lel_payloadType = htolel((payloadType) ? payloadType : 4);
  r->msg.OpenReceiveChannel.qualifierIn.lel_vadValue = htolel(c->line->vad);
  sccp_dev_send(c->line->device, r);

  memset(m, 0, 4);
  ast_ouraddrfor(&sin.sin_addr, &__ourip);
  memcpy(m, &__ourip.s_addr, 4);
  if (sccp_debug)
    ast_verbose(VERBOSE_PREFIX_2 "Telling Endpoint to use %d.%d.%d.%d(%d):%d\n", m[0], m[1], m[2], m[3], ntohs(__ourip.s_addr), ntohs(sin.sin_port));

  payloadType = sccp_codec_ast2cisco(c->owner->writeformat);
  REQ(r, StartMediaTransmission)
  r->msg.StartMediaTransmission.lel_conferenceId = htolel(c->callid);
  r->msg.StartMediaTransmission.lel_passThruPartyId = htolel((uint32_t)c);
  r->msg.StartMediaTransmission.bel_remoteIpAddr = __ourip.s_addr;
  r->msg.StartMediaTransmission.lel_remotePortNumber = htolel(ntohs(sin.sin_port));
  r->msg.StartMediaTransmission.lel_millisecondPacketSize = htolel(20);
  r->msg.StartMediaTransmission.lel_payloadType = htolel((payloadType) ? payloadType : 4);
  r->msg.StartMediaTransmission.qualifierOut.lel_precedenceValue = htolel(0);
  r->msg.StartMediaTransmission.qualifierOut.lel_ssValue = htolel(0); // Silence supression
  r->msg.StartMediaTransmission.qualifierOut.lel_maxFramesPerPacket = htolel(95792);
  sccp_dev_send(c->line->device, r);
}

void sccp_channel_disconnect(sccp_channel_t * c) {
	sccp_moo_t * r;

	REQ(r, CloseReceiveChannel)
	r->msg.CloseReceiveChannel.lel_conferenceId = htolel(c ? c->callid : 0);
	r->msg.CloseReceiveChannel.lel_passThruPartyId = htolel((uint32_t)c);
	sccp_dev_send(c->line->device, r);

	REQ(r, StopMediaTransmission)
	r->msg.CloseReceiveChannel.lel_conferenceId = htolel(c ? c->callid : 0);
	r->msg.CloseReceiveChannel.lel_passThruPartyId = htolel((uint32_t)c);
	sccp_dev_send(c->line->device, r);

}

void sccp_channel_endcall(sccp_channel_t * c) {
	struct ast_frame f = { AST_FRAME_CONTROL, AST_CONTROL_HANGUP };

	ast_mutex_lock(&c->line->lock);
	ast_mutex_lock(&c->lock);

	if ((c->line->device->type == 0x2) || (c->line->device->type == 0x5)){
		ast_log(LOG_DEBUG, "sccp_channel_endcall for DeviceType 12SP+/30VIP\n");

		sccp_channel_StatisticsRequest(c);
		sccp_dev_set_speaker(c->line->device, StationSpeakerOff);
		sccp_dev_statusprompt_set(c->line->device, c, NULL, 0);
		sccp_channel_set_callstate(c, TsOnHook);
		sccp_dev_set_keyset(c->line->device, 0, KEYMODE_ONHOOK);
		sccp_dev_set_cplane(c->line->device,0,0);
		sccp_dev_set_mwi(c->line->device, c->line->instance, 0);
		sccp_channel_disconnect(c);
	} else {

		sccp_channel_disconnect(c);
		sccp_channel_StatisticsRequest(c);
		sccp_dev_set_speaker(c->line->device, StationSpeakerOff);
		sccp_dev_statusprompt_set(c->line->device, c, NULL, 0);
		sccp_channel_set_callstate(c, TsOnHook);
		sccp_dev_set_keyset(c->line->device, 0, KEYMODE_ONHOOK);
		sccp_dev_set_cplane(c->line->device,0,0);
		sccp_dev_set_mwi(c->line->device, c->line->instance, 0);

	}

	sccp_handle_time_date_req(c->device->session,NULL);
	sccp_dev_set_sptone(c->line->device, "NoTone");
	sccp_channel_disconnect(c);

	ast_mutex_unlock(&c->lock);
	if (!c->line) {
		ast_log(LOG_ERROR, "Channel %s doesn't have a line!\n", c->owner->name);
		return;
	}

	ast_mutex_unlock(&c->line->lock);

	// Queue a request for asterisk to hangup this channel.
	ast_queue_frame(c->owner, &f); // This locks the channel, so must be done outside of the locks

	return;
}
