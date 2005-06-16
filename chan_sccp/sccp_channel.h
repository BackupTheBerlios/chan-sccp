sccp_channel_t * sccp_channel_find_byid(int id);
void sccp_channel_send_callinfo(sccp_channel_t * c);
void sccp_channel_set_callstate(sccp_channel_t * c, StationDCallState state);
void sccp_channel_set_callingparty(sccp_channel_t * c, char *name, char *number);
void sccp_channel_set_calledparty(sccp_channel_t * c, char *name, char *number);
void sccp_channel_set_lamp(sccp_channel_t * c, int lampMode);
void sccp_channel_connect(sccp_channel_t * c);
void sccp_channel_disconnect(sccp_channel_t * c);
void sccp_channel_endcall(sccp_channel_t * c);
void sccp_channel_StatisticsRequest(sccp_channel_t * c);


