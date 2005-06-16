
void sccp_sk_redial(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c);
void sccp_sk_newcall(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c);
void sccp_sk_addline(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c);
void sccp_sk_dnd(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c);
void sccp_sk_back(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c);
void sccp_sk_dial(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c);
void sccp_sk_clear(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c);
void sccp_sk_endcall(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c);

void sccp_sk_answer(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c);
void sccp_sk_reject(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c);
void sccp_sk_hold(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c);
void sccp_sk_conference(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c);
void sccp_sk_transfer(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c);
void sccp_sk_blindxfr(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c);

// following are emtry functions, because we are working on them later but
// added for completeness
void sccp_sk_cfwd_all(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c);
void sccp_sk_cfwd_busy(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c);
void sccp_sk_cfwd_noanswer(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c);
void sccp_sk_resumecall(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c);
void sccp_sk_info(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c);
void sccp_sk_parkcall(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c);
void sccp_sk_join(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c);
void sccp_sk_meetme(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c);
void sccp_sk_pickup(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c);
void sccp_sk_pickup_group(sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c);


