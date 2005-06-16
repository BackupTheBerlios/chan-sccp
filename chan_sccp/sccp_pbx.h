struct ast_channel * sccp_new_channel(sccp_channel_t * sub, int state);
void * sccp_start_channel (void *data);
void start_rtp(sccp_channel_t * sub);

#ifdef ASTERISK_VERSION_HEAD
const struct ast_channel_tech sccp_tech;
#endif

void sccp_pbx_senddigit(sccp_channel_t * c, char digit);
void sccp_pbx_senddigits(sccp_channel_t * c, char digits[AST_MAX_EXTENSION]);
