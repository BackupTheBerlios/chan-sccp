/*
 *
 * SCCP support for Asterisk.
 *   -- By Zozo
 */

#include "chan_sccp.h"
#include "sccp_actions.h"
#include "sccp_helper.h"
#include "sccp_pbx.h"
#include "sccp_channel.h"
#include "sccp_device.h"
#include "sccp_line.h"
#include "sccp_sched.h"
#include <asterisk/utils.h>

AST_MUTEX_DEFINE_STATIC(devicelock);
AST_MUTEX_DEFINE_STATIC(linelock);



void sccp_handle_alarm(sccp_session_t * s, sccp_moo_t * r) {
	ast_log(LOG_NOTICE, "Alarm Message: Severity: %d, %s [%d/%d]\n", letohl(r->msg.AlarmMessage.lel_alarmSeverity), r->msg.AlarmMessage.text, letohl(r->msg.AlarmMessage.lel_parm1), letohl(r->msg.AlarmMessage.lel_parm2));
}

void sccp_handle_register(sccp_session_t * s, sccp_moo_t * r) {
  sccp_device_t * d;
  const value_string * v = device_types;
  char *mb, *cur, tmp[256];
  sccp_line_t * l;
  sccp_moo_t * r1;

  while(v->value) {
    if (v->key == letohl(r->msg.RegisterMessage.lel_deviceType))
      break;
    v++;
  }

  ast_log(LOG_DEBUG, "Trying to register device %s, Instance: %d, Type: %s, Version: %d\n", 
                      r->msg.RegisterMessage.sId.deviceName, 
                      letohl(r->msg.RegisterMessage.sId.lel_instance), 
                      (v) ? v->value : "Unknown",
                      (int)r->msg.RegisterMessage.protocolVer);

  ast_mutex_lock(&devicelock);

  d = devices;

  while (d) {
    ast_mutex_lock(&d->lock);
    if (!strcasecmp(r->msg.RegisterMessage.sId.deviceName, d->id)) {
      if (d->session) {
        // We still have an old session here.
	// This behavior of re-registering seems to be typical for the 7920. if the phone
	// cannot Re-Register, it reboots.

        ast_log(LOG_WARNING, "Device %s is doing a re-registration. FIXFIX!\n", d->id);
        ast_mutex_unlock(&d->lock);
	// break; 
      }
      ast_log(LOG_DEBUG, "Allocating Device %p to session %p\n", d, s);
      s->device = d;
      d->type = letohl(r->msg.RegisterMessage.lel_deviceType);
      d->session = s;
      ast_mutex_unlock(&d->lock);
      break;
    }
    ast_mutex_unlock(&d->lock);
    d = d->next;
  }

  ast_mutex_unlock(&devicelock);

  if (!d) {
    REQ(r1, RegisterRejectMessage);
    ast_log(LOG_ERROR, "Rejecting Device %s: Device not found\n", r1->msg.RegisterMessage.sId.deviceName);
    strncpy(r1->msg.RegisterRejectMessage.text, "Unknown Device", StationMaxDisplayTextSize);
    sccp_session_send(s, r1);
    return;
  }


  strncpy(tmp, d->autologin, sizeof(tmp));
  mb = tmp;
  while((cur = strsep(&mb, ","))) {
    if (strlen(cur)) {
      if (sccp_debug)
        ast_verbose(VERBOSE_PREFIX_1 "Auto logging into %s\n", cur);
      l = sccp_line_find_byname(cur);
      if (l)
        sccp_dev_attach_line(d, l);
      else
        ast_log(LOG_ERROR, "Failed to autolog %s into %s: Couldn't find line %s\n", d->id, cur, cur);
    }
  }

  d->currentLine = d->lines;

  REQ(r1, RegisterAckMessage);
  r1->msg.RegisterAckMessage.lel_protocolVer = htolel(0x3);
  r1->msg.RegisterAckMessage.lel_keepAliveInterval = htolel(keepalive);
  r1->msg.RegisterAckMessage.lel_secondaryKeepAliveInterval = htolel(keepalive);
  strncpy(r1->msg.RegisterAckMessage.dateTemplate, date_format, StationDateTemplateSize);
  sccp_dev_send(d, r1);
  sccp_dev_set_registered(d, RsProgress);
  sccp_dev_set_registered(d, RsOK);

  //this should be polled, anyway, but for quick MWI status after registration:
  sccp_dev_check_mwi(s->device);
  if (s->device->dnd && (time(0) + 5) > s->device->lastCallEndTime)
    sccp_dev_statusprompt_set(s->device, NULL, "DND is Enabled", 0);

//We pretty much know each device, but lets do it for the heck of it anyway :)
  sccp_dev_sendmsg(d, CapabilitiesReqMessage);

}


// Handle the configuration of the device here.

void
sccp_handle_button_template_req(sccp_session_t * s, sccp_moo_t * r)
{
  int b = 0, i;
  sccp_moo_t * r1;
  const btnlist * list;
  sccp_device_t * d = s->device;
  sccp_speed_t * k = d->speed_dials;
  sccp_line_t * l  = d->lines;
  int lineInstance = 1, speedInstance = 1;

  ast_mutex_lock(&devicelock);
  ast_mutex_lock(&linelock);

  REQ(r1, ButtonTemplateMessage)
  r1->msg.ButtonTemplateMessage.lel_buttonOffset = htolel(0);

  for (i = 0; i < StationMaxButtonTemplateSize ; i++) {
    r1->msg.ButtonTemplateMessage.definition[i].instanceNumber   = 0;
    r1->msg.ButtonTemplateMessage.definition[i].buttonDefinition = 0xFF;
  }

  if (!d->buttonSet) {
    ast_log(LOG_WARNING, "Don't have a button layout, sending blank template.\n");
    sccp_dev_send(s->device, r1);
    ast_mutex_unlock(&linelock);
    ast_mutex_unlock(&devicelock);
    return;
  }

  list = d->buttonSet->buttons;

  if (sccp_debug >= 2)
    ast_verbose(VERBOSE_PREFIX_2 "Configuring button template. buttonOffset=%d, buttonCount=%d, totalButtonCount=%d\n", 
      0, d->buttonSet->buttonCount, d->buttonSet->buttonCount);

  r1->msg.ButtonTemplateMessage.lel_buttonCount = htolel(d->buttonSet->buttonCount);
  r1->msg.ButtonTemplateMessage.lel_totalButtonCount = htolel(d->buttonSet->buttonCount);

  for (i = 0 ; i < d->buttonSet->buttonCount ; i++) {
    int inst = 1;

    r1->msg.ButtonTemplateMessage.definition[i].buttonDefinition = list->type;

    switch (list->type) {
      case BtLine:
        inst = lineInstance++;
        while (l) {
          if (l->device == s->device) {
            ast_log(LOG_DEBUG, "Line[%.2d] = LINE(%s)\n", b+1, l->name);
            l->instance = inst;
            l->dnState = TsOnHook;
            l = l->next;
            break;
          }
          l = l->next;
        }
        break;

      case BtSpeedDial:
        inst = speedInstance++;
        break;
      default:
        break;
    }

    r1->msg.ButtonTemplateMessage.definition[i].instanceNumber = inst;

    if (sccp_debug >= 3)
      ast_verbose(VERBOSE_PREFIX_3 "%d %X\n", inst, list->type);
    list++;
  }

  ast_mutex_unlock(&linelock);
  ast_mutex_unlock(&devicelock);
  sccp_dev_send(s->device, r1);
  sccp_dev_set_keyset(s->device, NULL, KEYMODE_ONHOOK);

  return;

  /*
  ast_log(LOG_DEBUG, "Configuring buttons for %s\n", s->device->id);
  */

  // If there are no lines, we don't add speed dials, either, as a device with
  // no lines but speed dials seems to break 3.3(4.1).  Makes sense, I guess anyway.

  if (b == 0) {
    ast_log(LOG_DEBUG, "No lines found, sending empty ButtonTemplate\n");
    return;
  }


  while (k) {
    ast_log(LOG_DEBUG, "Line[%.2d] = SPEEDDIAL(%s)\n", b+1, k->name);
    r1->msg.ButtonTemplateMessage.definition[b].instanceNumber   = (b + 1);
    r1->msg.ButtonTemplateMessage.definition[b].buttonDefinition = BtSpeedDial;
    k->index = (b + 1);
    b++;
    k = k->next;
  }


  ast_log(LOG_DEBUG, "buttonCount: %d\n", letohl(r1->msg.ButtonTemplateMessage.lel_buttonCount));

  sccp_dev_send(s->device, r1);
}

void
sccp_handle_line_number(sccp_session_t * s, sccp_moo_t * r)
{
  int lineNumber = letohl(r->msg.LineStatReqMessage.lel_lineNumber);
  sccp_line_t * llines;
  sccp_moo_t * r1;

  ast_log(LOG_DEBUG, "Configuring line number %d for device %s\n", lineNumber, s->device->id);

  ast_mutex_lock(&devicelock);
  llines = s->device->lines;
  while (llines) {
    if (llines->instance == lineNumber)
      break;
    llines = llines->next;
  }
  ast_mutex_unlock(&devicelock);

  REQ(r1, LineStatMessage)

  r1->msg.LineStatMessage.lel_lineNumber = htolel(lineNumber);

  if (llines == NULL) {
    ast_log(LOG_ERROR, "SCCP device %s requested a line configuration for unknown line %d\n", s->device->id, lineNumber);
    // XXX:T: Maybe we should return something here to tell the thing to fuck off and die?
    sccp_dev_send(s->device, r1);
    return;
  }

  memcpy(r1->msg.LineStatMessage.lineDirNumber, llines->name, StationMaxDirnumSize);
  memcpy(r1->msg.LineStatMessage.lineFullyQualifiedDisplayName, llines->label, StationMaxNameSize);
  
  sccp_dev_send(s->device, r1);
}

void sccp_handle_speed_dial_stat_req(sccp_session_t * s, sccp_moo_t * r) {
	sccp_speed_t * k = s->device->speed_dials;
	sccp_moo_t * r1;
	int wanted = letohl(r->msg.SpeedDialStatReqMessage.lel_speedDialNumber);

	if (sccp_debug >= 3)
		ast_verbose(VERBOSE_PREFIX_3 "Speed Dial Request for Button %d\n", wanted);

	REQ(r1, SpeedDialStatMessage)
	r1->msg.SpeedDialStatMessage.lel_speedDialNumber = htolel(wanted);

	if ((k = sccp_dev_speed_find_byindex(s->device, wanted)) != NULL) {
		strncpy(r1->msg.SpeedDialStatMessage.speedDialDirNumber, k->ext, StationMaxDirnumSize);
		strncpy(r1->msg.SpeedDialStatMessage.speedDialDisplayName, k->name, StationMaxNameSize);
	} else {
		ast_verbose(VERBOSE_PREFIX_3 "speeddial %d not assigned\n", wanted);
	}

	sccp_dev_send(s->device, r1);
}

void sccp_handle_stimulus(sccp_session_t * s, sccp_moo_t * r) {
	sccp_line_t * l;
	sccp_speed_t * k = s->device->speed_dials;
	sccp_channel_t * c;

	const value_string * v = deviceStimuli;
  
	int stimulus = letohl(r->msg.StimulusMessage.lel_stimulus);
	int stimulusInstance = letohl(r->msg.StimulusMessage.lel_stimulusInstance);

	while (v->value) {
		if (v->key == stimulus)
			break;
		v++;
	}

	if (sccp_debug)
		ast_verbose(VERBOSE_PREFIX_3 "Got {StimulusMessage} stimulus=%s(%d) stimulusInstance=%d\n", (v) ? v->value : "Unknown!", stimulus, stimulusInstance);

	switch (stimulus) {

		case BtLastNumberRedial: // We got a Redial Request
			if (!s->device->lastNumberLine)
				return;	// If there's no number, there is no reason to redial

			l = sccp_line_find_byid(s->device, s->device->lastNumberLine);

			if (l)
				sccp_dev_allocate_channel(s->device, l, 1, s->device->lastNumber);
			break;

		case BtLine: // We got a Line Request
			l = sccp_line_find_byid(s->device, stimulusInstance);
			sccp_device_select_line(s->device, l);
			if ((s->device->type == 0x2) || (s->device->type == 0x5)) {
				ast_log(LOG_DEBUG,"BtLine Hook for 12SP+\n");
				sccp_sk_newcall(s->device,l,NULL);
			}
			break;

		case BtSpeedDial:
			k = sccp_dev_speed_find_byindex(s->device, stimulusInstance);

			if (NULL != k) {
				ast_verbose(VERBOSE_PREFIX_3 "Speeddial Button (%d) pressed, configured number is (%s)\n", stimulusInstance, k->ext);
				c = sccp_get_active_channel(s->device);
				if (c) {
					sccp_pbx_senddigits(c, k->ext);
				} else {
					// Pull up a channel
					l = s->device->currentLine;
					if (l) {
						strncpy(s->device->lastNumber, k->ext, sizeof(k->ext)-1);
						sccp_dev_allocate_channel(s->device,l,1, k->ext);
					}
				}
			} else {
				ast_verbose(VERBOSE_PREFIX_3 "Speeddial Button (%d) pressed, no assigned number\n", stimulusInstance);
			}
			break;

		case BtHold: // Handle the Hold Function through the Softkey
			if (s->device->currentLine->instance) {
				if (s->device->currentLine->dnState == TsHold) {
					sccp_sk_resumecall(s->device, s->device->currentLine, s->device->currentLine->activeChannel);
				} else {
					sccp_sk_hold(s->device, s->device->currentLine, s->device->currentLine->activeChannel);
				}
			}
			break;

		case BtTransfer: // Handle transfer via the softkey
			sccp_sk_transfer(s->device, s->device->currentLine, s->device->currentLine->activeChannel);
			break;

		case BtVoiceMail: // Get a new Line and Dial the Voicemail.
			if (s->device->currentLine->channelCount > 0) {
				ast_log(LOG_NOTICE, "Cannot call voicemail while call in progress");
			} else {
				ast_log(LOG_NOTICE, "Dialing voicemail %s", s->device->currentLine->vmnum);
				sccp_dev_allocate_channel(s->device, s->device->currentLine, 1, s->device->currentLine->vmnum);
			}
			break;

		case BtConference:
			ast_log(LOG_NOTICE, "Conference Button is not yet handled. working on implementation\n");
			break;

		case BtForwardAll: // Call forward all
			ast_log(LOG_NOTICE, "Call forwarding is not yet handled. working on implementation\n");
			break;

		case BtCallPark: // Call parking
			ast_log(LOG_NOTICE, "Call parking is not yet handled. working on implementation\n");
			break;

		default: // Handling for unknown stimuli -- please report if you see something strange
			ast_log(LOG_NOTICE, "Don't know how to deal with stimulus %d with Phonetype %d \n", stimulus, s->device->type);
			break;
	}
}

void sccp_handle_offhook(sccp_session_t * s, sccp_moo_t * r, sccp_channel_t * chan) {
  sccp_line_t * l;

  if (!s->device->lines) {
    ast_log(LOG_NOTICE, "No lines registered on %s for take OffHook\n", s->device->id);
    sccp_dev_statusprompt_set(s->device, NULL, "No lines registered!", 0);
    sccp_dev_set_sptone(s->device, "BeepBonk");
    return;
  }

  if (sccp_debug)
    ast_verbose(VERBOSE_PREFIX_3 "Device d=%p s=%p s->d->s=%p Taken Offhook\n", s->device, s, s->device->session);

  // Check if there is a call comming in on our currently selected line.
  l = sccp_dev_get_activeline(s->device);

  chan = l->channels;
  while (chan) {
    if (chan->isRinging)
      break;
    chan = chan->next;
  }

  if (chan) {
    // Answer the ringing channel.
    ast_log(LOG_DEBUG, "Anwered Ringing Channel\n");
    s->device->active_channel = chan;
    sccp_dev_set_ringer(s->device, StationRingOff);
    sccp_channel_set_lamp(chan, LampOn);
    chan->isRinging = 0;
    sccp_dev_set_keyset(s->device, chan, KEYMODE_CONNECTED);
    sccp_dev_set_speaker(s->device, StationSpeakerOn);
    ast_queue_control(chan->owner, AST_CONTROL_ANSWER);
    sccp_channel_set_callstate(chan, TsOffHook);    
    sccp_channel_send_callinfo(chan);
    sccp_channel_set_callstate(chan, TsConnected);    
    start_rtp(chan);
    ast_setstate(chan->owner, AST_STATE_UP);
  } else {
    if (s->device->currentLine->channels)
      chan = s->device->currentLine->channels;
    else
      chan = sccp_dev_allocate_channel(s->device, s->device->currentLine, 1, NULL);

    if (!chan) {
      ast_log(LOG_ERROR, "Failed to allocate SCCP channel.\n");
      return;
    }

    sccp_dev_set_speaker(s->device, StationSpeakerOn);
    sccp_dev_statusprompt_set(s->device, chan, NULL, 0);
    sccp_dev_set_keyset(s->device, chan, KEYMODE_OFFHOOK);
    sccp_dev_set_sptone(s->device, "InsideDialTone");
  }

}

void sccp_handle_onhook(sccp_session_t * s, sccp_moo_t * r) {
  sccp_channel_t * c;

  if (sccp_debug)
    ast_verbose(VERBOSE_PREFIX_3 "Put Onhook\n");

  if (!s->device->lines) {
    ast_log(LOG_NOTICE, "No lines registered on %s to put OnHook\n", s->device->id);
    sccp_dev_set_sptone(s->device, "NoTone");
    return;
  }

  // get the active channel
  c = sccp_device_active_channel(s->device);

  if (!c) {
	if ((s->device->type == 0x2) || (s->device->type == 0x5)) {
                ast_log(LOG_DEBUG, "OnHook for 12SP+\n");
		sccp_dev_set_speaker(s->device, StationSpeakerOff);
		sccp_dev_set_sptone(s->device, "NoTone");
		return;
	} else {
		ast_log(LOG_ERROR, "Erp, tried to hangup when we didn't have an active channel?!\n");
		return;
	}
  }

  if (!c->line)
    ast_log(LOG_NOTICE, "Channel didn't have a parent on OnHook - Huuu?!\n");

  sccp_channel_endcall(c);

  return;
}

void sccp_handle_headset(sccp_session_t * s, sccp_moo_t * r) {
  // XXX:T: What should we do here?
}

void sccp_handle_capabilities_res(sccp_session_t * s, sccp_moo_t * r) {
  const codec_def * v = codec_list;
  int i;
  int n = letohl(r->msg.CapabilitiesResMessage.lel_count);
  s->device->capability = 0; // Clear capabilities
  if (sccp_debug)
    ast_verbose(VERBOSE_PREFIX_1 "Device has %d Capabilities\n", n);
  for (i = 0 ; i < n; i++) {
    v = codec_list;
    while (v->value) {
      if (v->key == letohl(r->msg.CapabilitiesResMessage.caps[i].lel_payloadCapability))
        break;
      v++;
    }
    if (v)
      s->device->capability |= v->astcodec;
    if (sccp_debug)
      ast_verbose(VERBOSE_PREFIX_3 "CISCO:%6d %-25s AST:%6d %s\n", letohl(r->msg.CapabilitiesResMessage.caps[i].lel_payloadCapability), (v) ? v->value : "Unknown", v->astcodec, ast_codec2str(v->astcodec));
  }
}


void sccp_handle_soft_key_template_req(sccp_session_t * s, sccp_moo_t * r) {
  int i = 0;
  const softkeytypes * v = button_labels;
  sccp_moo_t * r1;

  REQ(r1, SoftKeyTemplateResMessage)
  r1->msg.SoftKeyTemplateResMessage.lel_softKeyOffset = htolel(0);

  while (v->id) {
    ast_log(LOG_DEBUG, "Button(%d)[%2d] = %s\n", i, v->id, v->txt);
    strncpy(r1->msg.SoftKeyTemplateResMessage.definition[i].softKeyLabel, v->txt, 15);
    r1->msg.SoftKeyTemplateResMessage.definition[i].lel_softKeyEvent = htolel(v->id);
    i++;
    v++;
  }

  r1->msg.SoftKeyTemplateResMessage.lel_softKeyCount = htolel(i+1);
  r1->msg.SoftKeyTemplateResMessage.lel_totalSoftKeyCount = htolel(i+1);
  sccp_dev_send(s->device, r1);

  sccp_dev_set_keyset(s->device, NULL, KEYMODE_ONHOOK);
}

/*
 * XXX:T: This code currently only allows for a maximum of 32 SoftKeySets. We can
 *        extend this to up to 255, by using the offset field.
 */

void
sccp_handle_soft_key_set_req(sccp_session_t * s, sccp_moo_t * r)
{
  const softkey_modes * v = SoftKeyModes;
  int iKeySetCount = 0;
  sccp_moo_t * r1;

  REQ(r1, SoftKeySetResMessage)
  r1->msg.SoftKeySetResMessage.lel_softKeySetOffset = htolel(0);

  while (v && v->ptr) {
    const btndef * b = v->ptr;
    int c = 0;

    if (sccp_debug)
      ast_verbose(VERBOSE_PREFIX_3 "Set[%d] = ", v->setId);

    while (b && b->labelId != 0 ) {
      if (sccp_debug)
        ast_verbose(VERBOSE_PREFIX_1 "%d:%d ", c, b->labelId);
      if (b->labelId != -1)
        r1->msg.SoftKeySetResMessage.definition[v->setId].softKeyTemplateIndex[c] = b->labelId;
      c++;
      b++;
    }

    if (sccp_debug)
      ast_verbose(VERBOSE_PREFIX_3 "\n");
    v++;
    iKeySetCount++;
  };

  if (sccp_debug)
    ast_verbose( VERBOSE_PREFIX_3 "There are %d SoftKeySets.\n", iKeySetCount);
  
  r1->msg.SoftKeySetResMessage.lel_softKeySetCount = htolel(iKeySetCount);
  r1->msg.SoftKeySetResMessage.lel_totalSoftKeySetCount = htolel(iKeySetCount); // <<-- for now, but should be: iTotalKeySetCount;

  sccp_dev_send(s->device, r1);
}

void
sccp_handle_time_date_req(sccp_session_t * s, sccp_moo_t * req)
{
  time_t timer = 0;
  struct tm * cmtime = NULL;
  sccp_moo_t * r1;
  REQ(r1, DefineTimeDate)

  if (NULL == s) {
       ast_log(LOG_WARNING,"Session no longer valid\n");
       return;
  }

  /* modulate the timezone by full hours only */
  timer = time(0) + (s->device->tz_offset * 3600);
  cmtime = localtime(&timer);

  r1->msg.DefineTimeDate.lel_year = htolel(cmtime->tm_year+1900);
  r1->msg.DefineTimeDate.lel_month = htolel(cmtime->tm_mon+1);
  r1->msg.DefineTimeDate.lel_dayOfWeek = htolel(cmtime->tm_wday);
  r1->msg.DefineTimeDate.lel_day = htolel(cmtime->tm_mday);
  r1->msg.DefineTimeDate.lel_hour = htolel(cmtime->tm_hour);
  r1->msg.DefineTimeDate.lel_minute = htolel(cmtime->tm_min);
  r1->msg.DefineTimeDate.lel_seconds = htolel(cmtime->tm_sec);
  r1->msg.DefineTimeDate.lel_milliseconds = htolel(0);
  r1->msg.DefineTimeDate.lel_systemTime = htolel(timer);
  sccp_dev_send(s->device, r1);
}

void sccp_handle_keypad_button(sccp_session_t * s, sccp_moo_t * r) {
  int event = letohl(r->msg.KeypadButtonMessage.lel_kpButton);
  char resp;

  sccp_channel_t * c = sccp_get_active_channel(s->device);

  if (!c) {
    ast_log(LOG_NOTICE, "Device %s sent a Keypress, but there is no active channel!\n", s->device->id);
    return;
  }

  printf("Cisco Digit: %08x (%d)\n", event, event);

  // XXX:T: # working?
  if (event < 10)
    resp = '0' + event;
  else if (event == 14)
    resp = '*';
  else if (event == 15)
    resp = '#';
  else
    resp = '?';

  sccp_pbx_senddigit(c, resp);
}


void sccp_handle_soft_key_event(sccp_session_t * s, sccp_moo_t * r) {
  const softkeytypes * b = button_labels;
  sccp_channel_t * c = NULL;
  sccp_line_t * l = NULL;

  ast_log(LOG_DEBUG, "Got Softkey: keyEvent=%d lineInstance=%d callReference=%d\n",
    letohl(r->msg.SoftKeyEventMessage.lel_softKeyEvent),
    letohl(r->msg.SoftKeyEventMessage.lel_lineInstance),
    letohl(r->msg.SoftKeyEventMessage.lel_callReference));

  while (b->id) {
    if (b->id == letohl(r->msg.SoftKeyEventMessage.lel_softKeyEvent))
      break;
    b++;
  }

  if (!b->func) {
    ast_log(LOG_WARNING, "Don't know how to handle keypress %d\n", letohl(r->msg.SoftKeyEventMessage.lel_softKeyEvent));
    return;
  }

  if (sccp_debug >= 2)
    ast_verbose(VERBOSE_PREFIX_2 "Softkey %s (%d) has been pressed.\n", b->txt, b->id);

  if (letohl(r->msg.SoftKeyEventMessage.lel_lineInstance) != 0)
    l = sccp_line_find_byid(s->device, (int)(letohl(r->msg.SoftKeyEventMessage.lel_lineInstance)));

  if (letohl(r->msg.SoftKeyEventMessage.lel_callReference) != 0) {
    c = sccp_channel_find_byid((int)(letohl(r->msg.SoftKeyEventMessage.lel_callReference)));
    if (c) {
      if (c->line->device != s->device)
        c = NULL;
    }
  }

  if (sccp_debug >= 3)
    ast_verbose(VERBOSE_PREFIX_3 "Calling func()\n");

  b->func(s->device, l, c);

  if (sccp_debug >= 3)
    ast_verbose(VERBOSE_PREFIX_3 "Returned from func()\n");

}

void sccp_handle_open_recieve_channel_ack(sccp_session_t * s, sccp_moo_t * r) {
	sccp_channel_t * c = sccp_get_active_channel(s->device);
	struct sockaddr_in sin;
	struct in_addr in;
	char iabuf[INET_ADDRSTRLEN];

	in.s_addr = r->msg.OpenReceiveChannelAck.bel_ipAddr;

	ast_log(LOG_DEBUG, "Got OpenChannel ACK.  Status: %d, RemoteIP: %s, Port: %d, PassThruId: %d\n", 
		letohl(r->msg.OpenReceiveChannelAck.lel_orcStatus),
		ast_inet_ntoa(iabuf, sizeof(iabuf), in),
		letohl(r->msg.OpenReceiveChannelAck.lel_portNumber),
		letohl(r->msg.OpenReceiveChannelAck.lel_passThruPartyId));

	if (!c) {
		ast_log(LOG_ERROR, "Device %s sent OpenChannelAck, but there is no active channel!\n", s->device->id);
		return;
	}

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = r->msg.OpenReceiveChannelAck.bel_ipAddr;
	sin.sin_port = htons(letohl(r->msg.OpenReceiveChannelAck.lel_portNumber));

	if (c->rtp && sin.sin_port)
		ast_rtp_set_peer(c->rtp, &sin);

	printf("Peer RTP is at port %s:%d\n", ast_inet_ntoa(iabuf, sizeof(iabuf), sin.sin_addr), ntohs(sin.sin_port));

}

void sccp_handle_version(sccp_session_t * s, sccp_moo_t * r) {
	sccp_moo_t * r1;
	char * verStr = sccp_helper_getversionfor(s);

	REQ(r1, VersionMessage)
	strncpy(r1->msg.VersionMessage.requiredVersion, verStr, (StationMaxVersionSize-1));
	sccp_dev_send(s->device, r1);
}

void sccp_handle_ConnectionStatistics(sccp_session_t * s, sccp_moo_t * r) {
	ast_verbose(VERBOSE_PREFIX_3 "Statistics from %s callid: %d Packets sent: %d rcvd: %d lost: %d jitter: %d latency: %d\n", r->msg.ConnectionStatisticsRes.DirectoryNumber,
		letohl(r->msg.ConnectionStatisticsRes.lel_CallIdentifier),
		letohl(r->msg.ConnectionStatisticsRes.lel_SentPackets),
		letohl(r->msg.ConnectionStatisticsRes.lel_RecvdPackets),
		letohl(r->msg.ConnectionStatisticsRes.lel_LostPkts),
		letohl(r->msg.ConnectionStatisticsRes.lel_Jitter),
		letohl(r->msg.ConnectionStatisticsRes.lel_latency)
		);
}

void sccp_handle_ServerResMessage(sccp_session_t * s, sccp_moo_t * r) {
	sccp_moo_t * r1;
	static char ourhost[48] = "asterisk";
	static char ourhost_2[48];
	static int ourport;
	static struct in_addr __ourip;

	REQ(r1, ServerResMessage);
	strncpy (r1->msg.ServerResMessage.server[0].serverName, ourhost, sizeof(r1->msg.ServerResMessage.server[0].serverName));
	strncpy (r1->msg.ServerResMessage.server[1].serverName, ourhost_2, sizeof(r1->msg.ServerResMessage.server[1].serverName));
	strncpy (r1->msg.ServerResMessage.server[2].serverName, ourhost_2, sizeof(r1->msg.ServerResMessage.server[2].serverName));
	strncpy (r1->msg.ServerResMessage.server[3].serverName, ourhost_2, sizeof(r1->msg.ServerResMessage.server[3].serverName));
	strncpy (r1->msg.ServerResMessage.server[4].serverName, ourhost_2, sizeof(r1->msg.ServerResMessage.server[4].serverName));
	r1->msg.ServerResMessage.serverListenPort[0] = ourport;
	r1->msg.ServerResMessage.serverIpAddr[0] = __ourip.s_addr;
	sccp_dev_send(s->device, r1);
   
	ast_log(LOG_DEBUG, "Sending Reply for ServerRequestMessage\n");
}

void sccp_handle_ConfigStatMessage(sccp_session_t * s, sccp_moo_t * r) {
	sccp_moo_t * r1;
	/* static int	 	lel_stationInstance; *
	 * static char		lel_userName[4];     *
	 * static char		lel_serverName[4];   */
	static int		lel_numberLines      = 0x0001;
	static int		lel_numberSpeedDials = 0x0003;

	REQ(r1, ConfigStatMessage);
	r1->msg.ConfigStatMessage.lel_numberLines	     = lel_numberLines;
	r1->msg.ConfigStatMessage.lel_numberSpeedDials 	     = lel_numberSpeedDials;
	sccp_dev_send(s->device, r1);

	ast_log(LOG_DEBUG, "Sending ConfigStatMessage (after Req)\n");
}

void sccp_handle_EnblocCallMessage(sccp_session_t * s, sccp_moo_t * r) {
	ast_log(LOG_NOTICE, "EnblocCallMessage recognized - not implemented yet\n");
}
