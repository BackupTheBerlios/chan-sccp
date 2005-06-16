#ifndef __CHAN_SCCP_H
#define __CHAN_SCCP_H

#include <sys/signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <asterisk/frame.h>
#include <asterisk/module.h>
#include <asterisk/channel.h>
#include <asterisk/lock.h>
#include <asterisk/logger.h>
#include <asterisk/sched.h>
#include <asterisk/io.h>
#include <asterisk/rtp.h>
#include <asterisk/config.h>
#include <asterisk/options.h>
#ifdef ASTERISK_VERSION_v1_0
#include <asterisk/channel_pvt.h>
#endif

#ifdef linux
#include <endian.h>
#endif /* linux */

#ifdef __FreeBSD__
#include <sys/endian.h>
#endif /* FreeBSD */

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define letohl(x) (x)
#define letohs(x) (x)
#define htolel(x) (x)
#define htoles(x) (x)
#else
#include <bits/byteswap.h>
#define letohl(x) __bswap_32(x)
#define letohs(x) __bswap_16(x)
#define htolel(x) __bswap_32(x)
#define htoles(x) __bswap_16(x)
#endif

#define DEFAULT_SCCP_PORT             2000 /* SCCP uses port 2000. */
#define DEFAULT_SCCP_BACKLOG             2 /* the listen baklog. */
#define SCCP_MAX_AUTOLOGIN             100 /* Maximum allowed of autologins per device */
#define SCCP_KEEPALIVE                   5 /* Default keepalive time if not specified in sccp.conf. */

#define StationMaxDeviceNameSize        16
#define StationMaxButtonTemplateSize    42
#define StationDateTemplateSize          6
#define StationMaxDisplayTextSize       33
#define StationMaxDirnumSize            24
#define StationMaxNameSize              40
#define StationMaxSoftKeyDefinition     32
#define StationMaxSoftKeySetDefinition  16
#define StationMaxSoftKeyIndex          16
#define StationMaxSoftKeyLabelSize      16
#define StationMaxVersionSize           16

//Sanity check for asterisk version
#ifndef ASTERISK_VERSION_HEAD
	#ifndef ASTERISK_VERSION_v1_0
		#error Unknown asterisk version, please edit the makefile
	#endif
#endif

typedef enum {
  
  /* Client -> Server */
  
  KeepAliveMessage                = 0x0000,
  RegisterMessage                 = 0x0001,
  IpPortMessage                   = 0x0002,
  KeypadButtonMessage             = 0x0003,
  EnblocCallMessage               = 0x0004,
  StimulusMessage                 = 0x0005,
  OffHookMessage                  = 0x0006,
  OnHookMessage                   = 0x0007,
  HookFlashMessage                = 0x0008,
  ForwardStatReqMessage           = 0x0009,
  SpeedDialStatReqMessage         = 0x000A,
  LineStatReqMessage              = 0x000B,
  ConfigStatReqMessage            = 0x000C,
  TimeDateReqMessage              = 0x000D,
  ButtonTemplateReqMessage        = 0x000E,
  VersionReqMessage               = 0x000F,
  CapabilitiesResMessage          = 0x0010,
  MediaPortListMessage            = 0x0011,
  ServerReqMessage                = 0x0012,
  AlarmMessage                    = 0x0020,
  MulticastMediaReceptionAck      = 0x0021,
  OpenReceiveChannelAck           = 0x0022,
  ConnectionStatisticsRes         = 0x0023,
  OffHookWithCgpnMessage          = 0x0024,
  SoftKeySetReqMessage            = 0x0025,
  SoftKeyEventMessage             = 0x0026,
  UnregisterMessage               = 0x0027,
  SoftKeyTemplateReqMessage       = 0x0028,
  RegisterTokenReq                = 0x0029,
  HeadsetStatusMessage            = 0x002B,
  unknownClientMessage2           = 0x002D,

  /* Server -> Client */

  RegisterAckMessage              = 0x0081,
  StartToneMessage                = 0x0082,
  StopToneMessage                 = 0x0083,
  // ??
  SetRingerMessage                = 0x0085,
  SetLampMessage                  = 0x0086,
  SetHkFDetectMessage             = 0x0087,
  SetSpeakerModeMessage           = 0x0088,
  SetMicroModeMessage             = 0x0089,
  StartMediaTransmission          = 0x008A,
  StopMediaTransmission           = 0x008B,
  StartMediaReception             = 0x008C,
  StopMediaReception              = 0x008D,
  // ?
  CallInfoMessage                 = 0x008F,

  ForwardStatMessage              = 0x0090,
  SpeedDialStatMessage            = 0x0091,
  LineStatMessage                 = 0x0092,
  ConfigStatMessage               = 0x0093,
  DefineTimeDate                  = 0x0094,
  StartSessionTransmission        = 0x0095,
  StopSessionTransmission         = 0x0096,
  ButtonTemplateMessage           = 0x0097,
  VersionMessage                  = 0x0098,
  DisplayTextMessage              = 0x0099,
  ClearDisplay                    = 0x009A,
  CapabilitiesReqMessage          = 0x009B,
  EnunciatorCommandMessage        = 0x009C,
  RegisterRejectMessage           = 0x009D,
  ServerResMessage                = 0x009E,
  Reset                           = 0x009F,

  KeepAliveAckMessage             = 0x0100,
  StartMulticastMediaReception    = 0x0101,
  StartMulticastMediaTransmission = 0x0102,
  StopMulticastMediaReception     = 0x0103,
  StopMulticastMediaTransmission  = 0x0104,
  OpenReceiveChannel              = 0x0105,
  CloseReceiveChannel             = 0x0106,
  ConnectionStatisticsReq         = 0x0107,
  SoftKeyTemplateResMessage       = 0x0108,
  SoftKeySetResMessage            = 0x0109,

  SelectSoftKeysMessage           = 0x0110, 
  CallStateMessage                = 0x0111,
  DisplayPromptStatusMessage      = 0x0112, 
  ClearPromptStatusMessage        = 0x0113, 
  DisplayNotifyMessage            = 0x0114,
  ClearNotifyMessage              = 0x0115,
  ActivateCallPlaneMessage        = 0x0116,
  DeactivateCallPlaneMessage      = 0x0117,
  UnregisterAckMessage            = 0x0118,
  BackSpaceReqMessage             = 0x0119,
  RegisterTokenAck                = 0x011A,
  RegisterTokenReject             = 0x011B,

  unknownForwardMessage1          = 0x011D,

} sccp_message_t;


typedef uint32_t StationUserId; 
typedef uint32_t StationInstance; 

typedef enum {
  StatsProcessing_Clear_Stats = 0,
  StatsProcessing_DoNotClear = 1 
} StatsProcessing;

typedef enum { 
  Media_SilenceSuppression_Off = 0 , 
  Media_SilenceSuppression_On = 1 
} Media_SilenceSuppression; 

typedef enum { 
  Media_EchoCancellation_Off = 0 , 
  Media_EchoCancellation_On = 1 
} Media_EchoCancellation; 

typedef enum { 
  Media_G723BRate_5_3 = 1 , 
  Media_G723BRate_6_4 = 2 
} Media_G723BitRate; 

typedef struct {
  uint32_t lel_precedenceValue; 
  Media_SilenceSuppression lel_ssValue; 
  uint32_t lel_maxFramesPerPacket; 
  Media_G723BitRate lel_g723BitRate; /* only used with G.723 payload */
} Media_QualifierOutgoing; 

typedef struct {
  Media_EchoCancellation lel_vadValue; 
  Media_G723BitRate lel_g723BitRate; /* only used with G.723 payload */
} Media_QualifierIncoming; 

typedef struct { 
  char            deviceName[StationMaxDeviceNameSize];
  StationUserId   lel_reserved;
  StationInstance lel_instance;
} StationIdentifier;

typedef enum {
  DEVICE_RESET = 1 ,
  DEVICE_RESTART = 2
} DeviceResetType;

typedef enum {
  StationRingOff = 0x1,
  StationInsideRing = 0x2,
  StationOutsideRing = 0x3,
  StationFeatureRing = 0x4
} StationRingMode;

typedef enum {
  StationSpeakerOn = 1 ,
  StationSpeakerOff = 2
} StationSpeakerMode;

typedef enum {
  StationMicOn = 1 ,
  StationMicOff = 2
} StationMicrophoneMode;

typedef enum {
  StationHeadsetOn = 1 ,
  StationHeadsetOff = 2
} StationHeadsetMode;

typedef enum { 
  TsOffHook = 1 , 
  TsOnHook = 2 , 
  TsRingOut = 3 , 
  TsRingIn = 4 , 
  TsConnected = 5 , 
  TsBusy = 6 , 
  TsCongestion = 7 , 
  TsHold = 8 , 
  TsCallWaiting = 9 , 
  TsCallTransfer = 10, 
  TsCallPark = 11, 
  TsProceed = 12, 
  TsCallRemoteMultiline = 13, // Up!
  TsInvalidNumber = 14 
} StationDCallState; 

typedef enum {
  BtUnused = 0x0,
  BtLastNumberRedial = 0x1,
  BtSpeedDial = 0x2,
  BtHold = 0x3,
  BtTransfer = 0x4,
  BtForwardAll = 0x5,
  BtForwardBusy = 0x6,
  BtForwardNoAnswer = 0x7,
  BtDisplay = 0x8,
  BtLine = 0x9,
  BtT120Chat = 0xA,
  BtT120Whiteboard = 0xB,
  BtT120ApplicationSharing = 0xC,
  BtT120FileTransfer = 0xD,
  BtVideo = 0xE,
  BtVoiceMail = 0xF,
  BtAutoAnswer = 0x11,
  BtGenericAppB1 = 0x21,
  BtGenericAppB2 = 0x22,
  BtGenericAppB3 = 0x23,
  BtGenericAppB4 = 0x24,
  BtGenericAppB5 = 0x25,
  BtMeetMeConference = 0x7B,
  BtConference = 0x7D,
  BtCallPark = 0x7E,
  BtCallPickup = 0x7F,
  BtGroupCallPickup = 0x80,
  BtNone = 0xFF,
  BtKeypad = 0xF0,
} StationButtonType;

typedef enum {
  LampOff = 1,			// Lamp Off, 0% Duty
  LampOn = 2,			// Lamp On, 100% Duty
  LampWink = 3,			// Lamp slow blink, ~90% Duty
  LampFlash = 4,		// Lamp very fast blink, ~70% Duty
  LampBlink = 5,		// Lamp slow blink, ~50% Duty
} LampModeType;

typedef struct {
	int key;
	char * value;
	int astcodec;
} codec_def;

typedef struct {
  int    key;
  char * value;
} value_string;

typedef struct {
  int key;
  int value;
} value_value;

typedef struct {
  int type;
} btnlist;

typedef struct {
  const char * type;
  int buttonCount;
  const btnlist * buttons;
} button_modes;

typedef struct { 
  uint8_t instanceNumber;   /* set to instance number or StationKeyPadButton value */
  uint8_t buttonDefinition; /* set to one of the preceding Bt values */
} StationButtonDefinition; 

typedef struct {
  uint32_t lel_payloadCapability;
  uint32_t lel_maxFramesPerPacket;
  union { 
    uint8_t futureUse[8]; 
    Media_G723BitRate lel_g723BitRate; 
  } PAYLOADS; 
} MediaCapabilityStructure; 

typedef struct { 
  char     softKeyLabel[StationMaxSoftKeyLabelSize];
  uint32_t lel_softKeyEvent;
} StationSoftKeyDefinition; 

typedef struct { 
  uint8_t  softKeyTemplateIndex[StationMaxSoftKeyIndex];
  uint16_t les_softKeyInfoIndex[StationMaxSoftKeyIndex];
} StationSoftKeySetDefinition;

typedef struct{
  char     serverName[48];
} ServerIdentifier;

typedef union {

  // No struct
  struct { } StationKeepAliveMessage;

  struct {
    StationIdentifier sId;
    uint32_t          lel_stationIpAddr;
    uint32_t          lel_deviceType;
    uint32_t          lel_maxStreams;
    uint32_t          lel__unknown1;
    uint8_t          protocolVer;
    uint32_t          lel__unknown2;
    uint32_t          lel__unknown3;
    uint32_t          lel_monitorObject;
    uint32_t          lel__unknown4;

    // 7910:
    // 02 00 00 00 // protocolVer (1st bit)
    // 08 00 00 00 == 8
    // 00 00 00 00
    // 02 00 00 00 == 2
    // ce f1 00 00 // == (61092 / 206 / 241) 1668 dn-size 420
  } RegisterMessage;

  struct {

    // All char arrays are in multiples of 32bit

    char 	      lel_deviceName[16];
    uint32_t 	      lel_stationUserId;
    uint32_t	      lel_stationInstance;
    char	      lel_userName[40];
    char	      lel_serverName[40];
    uint32_t	      lel_numberLines;
    uint32_t	      lel_numberSpeedDials;
  } ConfigStatMessage;

  struct {
    uint16_t les_rtpMediaPort;
  } IpPortMessage;

  struct {
    uint32_t lel_kpButton; 
  } KeypadButtonMessage;

  struct {} EnblocCallMessage;

  struct {
    uint32_t lel_stimulus; 
    uint32_t lel_stimulusInstance; /* normally set to 1 (except speed dial and line) */
  } StimulusMessage;


  struct {} OffHookMessage;
  struct {} OnHookMessage;
  struct {} HookFlashMessage;
  struct {} ForwardStatReqMessage;

  struct {
    uint32_t lel_speedDialNumber;
  } SpeedDialStatReqMessage;

  struct {
    uint32_t lel_lineNumber;
  } LineStatReqMessage;

  struct {} ConfigStatReqMessage;
  struct {} TimeDateReqMessage;
  struct {} ButtonTemplateReqMessage;
  struct {} VersionReqMessage;

  struct {
    uint32_t                 lel_count;
    MediaCapabilityStructure caps[18];
  } CapabilitiesResMessage;

  struct {} MediaPortListMessage;
  struct {} ServerReqMessage;
  
  struct {
    uint32_t lel_alarmSeverity;
    char     text[80];
    uint32_t lel_parm1;
    uint32_t lel_parm2;
  } AlarmMessage;
  
  struct {} MulticastMediaReceptionAck;
  
  struct {
    uint32_t lel_orcStatus; /* OpenReceiveChanStatus */
    uint32_t bel_ipAddr; /* This field is apparently in big-endian format,
                            even though most other fields are in
                            little-endian format. */
    uint32_t lel_portNumber;
    uint32_t lel_passThruPartyId;
  } OpenReceiveChannelAck;
  
  struct {
	char		DirectoryNumber[StationMaxDirnumSize];
	uint32_t	lel_CallIdentifier;
	StatsProcessing	lel_StatsProcessingType;
	uint32_t	lel_SentPackets;
	uint32_t	lel_SentOctets;
	uint32_t	lel_RecvdPackets;
	uint32_t	lel_RecvdOctets;
	uint32_t	lel_LostPkts;
	uint32_t	lel_Jitter;
	uint32_t	lel_latency;
  } ConnectionStatisticsRes;
  
  
  struct {} OffHookWithCgpnMessage;
  struct {} SoftKeySetReqMessage;
  
  struct {
    uint32_t lel_softKeyEvent;
    uint32_t lel_lineInstance;
    uint32_t lel_callReference;
  } SoftKeyEventMessage;
  
  struct {} UnregisterMessage;
  struct {} SoftKeyTemplateReqMessage;
  struct {} RegisterTokenReq;

  struct {
    StationHeadsetMode lel_hsMode;
  } HeadsetStatusMessage;

  struct {} unknownClientMessage2;

  struct {
    uint32_t lel_keepAliveInterval;
    char     dateTemplate[StationDateTemplateSize];
    uint16_t les__filler1;
    uint32_t lel_secondaryKeepAliveInterval;
    uint32_t lel_protocolVer;
  } RegisterAckMessage;

  struct {
    uint32_t lel_tone;
    char unkown[12];
  } StartToneMessage;

  struct {
    uint32_t lel_tone;
  } StopToneMessage;

  struct {
    uint32_t lel_ringMode;
  } SetRingerMessage;

  struct {
    uint32_t lel_stimulus; 
    uint32_t lel_stimulusInstance;
    uint32_t lel_lampMode; 
  } SetLampMessage;

  struct {} SetHkFDetectMessage;

  struct {
    StationSpeakerMode lel_speakerMode;
  } SetSpeakerModeMessage;

  struct {
    StationMicrophoneMode lel_micMode;
  } SetMicroModeMessage;

  struct {
    uint32_t lel_conferenceId;
    uint32_t lel_passThruPartyId;
    uint32_t bel_remoteIpAddr; /* This field is apparently in big-endian
                                  format, even though most other fields are
                                  little-endian. */
    uint32_t lel_remotePortNumber;
    uint32_t lel_millisecondPacketSize;
    uint32_t lel_payloadType; /* Media_PayloadType */
    Media_QualifierOutgoing qualifierOut;
  } StartMediaTransmission;

  struct {
    int32_t lel_conferenceId;
    int32_t lel_passThruPartyId;
  } StopMediaTransmission;

  struct {
  } StartMediaReception;

  struct {
    int32_t lel_conferenceId;
    int32_t lel_passThruPartyId;
  } StopMediaReception;

  struct {
    char callingPartyName[40];
    char callingParty[24];
    char calledPartyName[40];
    char calledParty[24];
    int32_t  lel_lineId;
    int32_t  lel_callRef;
    int32_t  lel_callType; /* INBOUND=1, OUTBOUND=2, FORWARD=3 */
    char originalCalledPartyName[40];
    char originalCalledParty[24];
  } CallInfoMessage;

  struct {} ForwardStatMessage;

  struct {
    uint32_t lel_speedDialNumber;
    char     speedDialDirNumber[StationMaxDirnumSize]; 
    char     speedDialDisplayName[StationMaxNameSize]; 
  } SpeedDialStatMessage;

  struct {
    uint32_t lel_lineNumber;
    char     lineDirNumber[StationMaxDirnumSize]; 
    char     lineFullyQualifiedDisplayName[StationMaxNameSize]; 
    char     _unknown5[44];
  } LineStatMessage;

  struct {
    uint32_t lel_year; 
    uint32_t lel_month; 
    uint32_t lel_dayOfWeek; 
    uint32_t lel_day; 
    uint32_t lel_hour; 
    uint32_t lel_minute; 
    uint32_t lel_seconds;
    uint32_t lel_milliseconds; 
    uint32_t lel_systemTime; 
  } DefineTimeDate;

  struct {} StartSessionTransmission;
  struct {} StopSessionTransmission;

  struct {
    uint32_t                lel_buttonOffset;
    uint32_t                lel_buttonCount;
    uint32_t                lel_totalButtonCount;
    StationButtonDefinition definition[StationMaxButtonTemplateSize]; 
  } ButtonTemplateMessage;

  struct {
    char requiredVersion[StationMaxVersionSize];
  } VersionMessage;

  struct {
    char     displayMessage[32];
    uint32_t lel_displayTimeout;
  } DisplayTextMessage;

  struct {} ClearDisplay;

  struct {} CapabilitiesReqMessage;
  struct {} EnunciatorCommandMessage;

  struct {
    char text[StationMaxDisplayTextSize];
  } RegisterRejectMessage;


  struct {
    ServerIdentifier    server[5];
    uint32_t 		serverListenPort[5];
    uint32_t    	serverIpAddr[5];
  } ServerResMessage;


  struct {
    int32_t lel_resetType;
  } Reset;

  struct {} KeepAliveAckMessage;
  struct {} StartMulticastMediaReception;
  struct {} StartMulticastMediaTransmission;
  struct {} StopMulticastMediaReception;
  struct {} StopMulticastMediaTransmission;

  struct {
    uint32_t lel_conferenceId;
    uint32_t lel_passThruPartyId;
    uint32_t lel_millisecondPacketSize; 
    uint32_t lel_payloadType; /* Media_PayloadType */
    Media_QualifierIncoming qualifierIn;
  } OpenReceiveChannel;

  struct {
    int32_t lel_conferenceId;
    int32_t lel_passThruPartyId;
  } CloseReceiveChannel;

  struct {			// Request Statistics from Phone
    char		DirectoryNumber[StationMaxDirnumSize];
    uint32_t		lel_callReference;
    uint32_t		lel_StatsProcessing;
  } ConnectionStatisticsReq;

  struct {
    uint32_t                   lel_softKeyOffset;
    uint32_t                   lel_softKeyCount;
    uint32_t                   lel_totalSoftKeyCount;
    StationSoftKeyDefinition   definition[StationMaxSoftKeyDefinition]; 
  } SoftKeyTemplateResMessage;

  struct {
    uint32_t                    lel_softKeySetOffset;
    uint32_t                    lel_softKeySetCount;
    uint32_t                    lel_totalSoftKeySetCount;
    StationSoftKeySetDefinition definition[StationMaxSoftKeySetDefinition]; 
  } SoftKeySetResMessage;

  struct {
    uint32_t lel_lineInstance;
    uint32_t lel_callReference;
    uint32_t lel_softKeySetIndex;
    uint16_t les_validKeyMask1;
    uint16_t les_validKeyMask2;
  } SelectSoftKeysMessage;

  struct {
    StationDCallState lel_callState;
    uint32_t          lel_lineInstance;
    uint32_t          lel_callReference;
    char              unknown[12];
  } CallStateMessage;

  struct {
    uint32_t lel_messageTimeout;
    char     promptMessage[32];
    uint32_t lel_lineInstance;
    uint32_t lel_callReference;
  } DisplayPromptStatusMessage;

  struct {
    uint32_t lel_lineInstance;
    uint32_t lel_callReference;
  } ClearPromptStatusMessage;

  struct {
    uint32_t lel_displayTimeout;
    char     displayMessage[100];
    uint32_t lel__unkn2;
    uint32_t lel__unkn3;
    uint32_t lel__unkn4;
    uint32_t lel__unkn5;
  } DisplayNotifyMessage;

  // No Struct.
  struct {} ClearNotifyMessage;

  struct {
    uint32_t lel_lineInstance;
  } ActivateCallPlaneMessage;

  // No Struct.
  struct {} DeactivateCallPlaneMessage;

  struct {} UnregisterAckMessage;
  struct {} BackSpaceReqMessage;
  struct {} RegisterTokenAck;
  struct {} RegisterTokenReject;
  struct {} unknownForwardMessage1;

} sccp_data_t;

/* I'm not quiet sure why this is called sccp_moo_t -
 * the only reason I can think of is I was very tired 
 * when i came up with it...  */

typedef struct {
  size_t      length;
  int         lel_reserved;
  int         lel_messageId;
  sccp_data_t msg;
} sccp_moo_t;

/* So in theory, a message should never be bigger than this.  
 * If it is, we abort the connection */
#define SCCP_MAX_PACKET sizeof(sccp_moo_t)

typedef struct sccp_channel  sccp_channel_t;
typedef struct sccp_session  sccp_session_t;
typedef struct sccp_line     sccp_line_t;
typedef struct sccp_speed    sccp_speed_t;
typedef struct sccp_device   sccp_device_t;
typedef struct sccp_intercom sccp_intercom_t;
typedef struct sccp_pvt      sccp_pvt_t;

/* An asterisk channel can now point to a channel (single device)
 * or an intercom.  So we need somethign to pass around :) */
struct sccp_pvt {
  sccp_channel_t  * chan;
  sccp_intercom_t * icom;
};

/* A line is a the equiv of a 'phone line' going to the phone. */
struct sccp_line {

  /* lockmeupandtiemedown */
  ast_mutex_t lock;

  /* This line's ID, used for logging into (for mobility) */
  char id[4];

  /* PIN number for mobility/roaming. */
  char pin[8];

  /* The lines position/instanceId on the current device*/
  uint8_t instance;

  /* the name of the line, so use in asterisk (i.e SCCP/<name>) */
  char name[80];

  /* A description for the line, displayed on in header (on7960/40) 
   * or on main  screen on 7910 */
  char description[80];

  /* A name for the line, displayed next to the button (7960/40). */
  char label[42];

  /* mainbox numbers (seperated by commas) to check for messages */
  char mailbox[AST_MAX_EXTENSION];

  /* Voicemail number to dial */
  char vmnum[AST_MAX_EXTENSION];

  /* The context we use for outgoing calls. */
  char context[AST_MAX_EXTENSION];

  /* CallerId to use on outgoing calls*/
#ifdef ASTERISK_VERSION_v1_0
  char callerid[AST_MAX_EXTENSION];
#elif ASTERISK_VERSION_HEAD
  char cid_name[AST_MAX_EXTENSION];
  char cid_num[AST_MAX_EXTENSION];
#endif

  /* The currently active channel. */
  sccp_channel_t * activeChannel;

  /* Linked list of current channels for this line */
  sccp_channel_t * channels;

  /* Number of currently active channels */
  unsigned int channelCount;

  /* Next line in the global list of devices */
  sccp_line_t * next;

  /* Next line on the current device. */
  sccp_line_t * lnext;

  /* The device this line is currently registered to. */
  sccp_device_t * device;

  /* current state of the hook on this line */
  // enum { DsDown, DsIdle, DsSeize, DsAlerting } dnState; 

  /* If we want VAD on this line */
  // XXX:T: Asterisk RTP implementation doesn't seem to handle this
  // XXX:T: correctly atm, althoguh not *really* looked/checked.

  unsigned int         vad:1;

  unsigned int         hasMessages:1;
  unsigned int         spareBit2:1;
  unsigned int         spareBit3:1;
  unsigned int         spareBit4:1;
  unsigned int         spareBit5:1;
  unsigned int         spareBit6:1;
  unsigned int         spareBit7:1;

  StationDCallState dnState;

};

/* This defines a speed dial button */
struct sccp_speed {

  /* The name of the speed dial button */
  char name[40];

  /* The number to dial when it's hit */
  char ext[AST_MAX_EXTENSION];

  /* The index (instance) n the current device */
  int index;

  /* The parent device we're currently on */
  sccp_device_t * device;

  /* Pointer to next speed dial */
  sccp_speed_t * next;

};


struct sccp_device {

  /* SEP<macAddress> of the device. */
  char id[StationMaxDeviceNameSize];

  char description[StationMaxDeviceNameSize];

  /* lines to auto login this device to */
  char autologin[SCCP_MAX_AUTOLOGIN];

  /* model of this phone used for setting up features/softkeys/buttons etc. */
  int type;

  /* timezone offset */
  int tz_offset;

  /* Current softkey set we're using */
  int currentKeySet;
  int currentKeySetLine;

  /* version to send to the phone */
  char imgversion[24];

  /* If the device has been rully registered yet */
  enum { RsNone, RsProgress, RsFailed, RsOK, RsTimeout } registrationState;

  /* time() the last call ended. */
  time_t lastCallEndTime;

  /* asterisk codec device preference */
  struct ast_codec_pref codecs;


  int keyset;
  int ringermode;
  int speaker;
  int mic;
  int linesWithMail;
  int currentTone;
  int registered;
  int capability;

  unsigned int         hasMessages:1;
  unsigned int         dnd:1;
  unsigned int         spareBit2:1;
  unsigned int         spareBit3:1;
  unsigned int         spareBit4:1;
  unsigned int         spareBit5:1;
  unsigned int         spareBit6:1;
  unsigned int         spareBit7:1;

  sccp_channel_t * active_channel;
  sccp_speed_t * speed_dials;
  sccp_line_t * lines;
  sccp_line_t * currentLine;
  sccp_session_t * session;
  sccp_device_t * next;

  const button_modes * buttonSet;
  char lastNumber[AST_MAX_EXTENSION];
  int  lastNumberLine;

  ast_mutex_t lock;

  struct ast_ha * ha;
  struct sockaddr_in addr;
  struct in_addr ourip;
};

struct sccp_intercom {
  ast_mutex_t       lock;
  char              id[24];
  char              description[24];
  sccp_device_t  ** devices;
  sccp_intercom_t * next;
};

struct sccp_session {
  ast_mutex_t          lock;
  pthread_t            t;
  char               * in_addr;
  void * buffer;
  size_t buffer_size;



  struct sockaddr_in   sin;
  time_t               lastKeepAlive;
  int                  fd;
  int                  rtpPort;
  char                 inbuf[SCCP_MAX_PACKET];
  sccp_device_t      * device;
  sccp_session_t     * next;
};

struct sccp_channel {
  ast_mutex_t          lock;
  char                 calledPartyName[40];
  char                 calledPartyNumber[24];
  char                 callingPartyName[40];
  char                 callingPartyNumber[24];
  uint32_t             callid;
  sccp_device_t	     * device;
  struct ast_channel * owner;
  sccp_line_t        * line;
  struct ast_rtp     * rtp;
  sccp_channel_t     * next,
                     * lnext;
  unsigned int         isOutgoing:1;
  unsigned int         isRinging:1;
  unsigned int         sentCallInfo:1;
  unsigned int         spareBit1:1;
  unsigned int         spareBit2:1;
  unsigned int         spareBit3:1;
  unsigned int         spareBit4:1;
  unsigned int         spareBit5:1;
};


static const value_string message_list [] = {

  /* Station -> Callmanager */
  {0x0000, "KeepAliveMessage"},
  {0x0001, "RegisterMessage"},
  {0x0002, "IpPortMessage"},
  {0x0003, "KeypadButtonMessage"},
  {0x0004, "EnblocCallMessage"},
  {0x0005, "StimulusMessage"},
  {0x0006, "OffHookMessage"},
  {0x0007, "OnHookMessage"},
  {0x0008, "HookFlashMessage"},
  {0x0009, "ForwardStatReqMessage"},
  {0x000A, "SpeedDialStatReqMessage"},
  {0x000B, "LineStatReqMessage"},
  {0x000C, "ConfigStatReqMessage"},
  {0x000D, "TimeDateReqMessage"},
  {0x000E, "ButtonTemplateReqMessage"},
  {0x000F, "VersionReqMessage"},
  {0x0010, "CapabilitiesResMessage"},
  {0x0011, "MediaPortListMessage"},
  {0x0012, "ServerReqMessage"},
  {0x0020, "AlarmMessage"},
  {0x0021, "MulticastMediaReceptionAck"},
  {0x0022, "OpenReceiveChannelAck"},
  {0x0023, "ConnectionStatisticsRes"},
  {0x0024, "OffHookWithCgpnMessage"},
  {0x0025, "SoftKeySetReqMessage"},
  {0x0026, "SoftKeyEventMessage"},
  {0x0027, "UnregisterMessage"},
  {0x0028, "SoftKeyTemplateReqMessage"},
  {0x0029, "RegisterTokenReq"},
  {0x002B, "HeadsetStatusMessage"},
  {0x002D, "unknownClientMessage2"},

  /* Callmanager -> Station */
  /* 0x0000, 0x0003? */
  {0x0081, "RegisterAckMessage"},
  {0x0082, "StartToneMessage"},
  {0x0083, "StopToneMessage"},
  {0x0085, "SetRingerMessage"},
  {0x0086, "SetLampMessage"},
  {0x0087, "SetHkFDetectMessage"},
  {0x0088, "SetSpeakerModeMessage"},
  {0x0089, "SetMicroModeMessage"},
  {0x008A, "StartMediaTransmission"},
  {0x008B, "StopMediaTransmission"},
  {0x008C, "StartMediaReception"},
  {0x008D, "StopMediaReception"},
  {0x008F, "CallInfoMessage"},
  {0x0090, "ForwardStatMessage"},
  {0x0091, "SpeedDialStatMessage"},
  {0x0092, "LineStatMessage"},
  {0x0093, "ConfigStatMessage"},
  {0x0094, "DefineTimeDate"},
  {0x0095, "StartSessionTransmission"},
  {0x0096, "StopSessionTransmission"},
  {0x0097, "ButtonTemplateMessage"},
  {0x0098, "VersionMessage"},
  {0x0099, "DisplayTextMessage"},
  {0x009A, "ClearDisplay"},
  {0x009B, "CapabilitiesReqMessage"},
  {0x009C, "EnunciatorCommandMessage"},
  {0x009D, "RegisterRejectMessage"},
  {0x009E, "ServerResMessage"},
  {0x009F, "Reset"},
  {0x0100, "KeepAliveAckMessage"},
  {0x0101, "StartMulticastMediaReception"},
  {0x0102, "StartMulticastMediaTransmission"},
  {0x0103, "StopMulticastMediaReception"},
  {0x0104, "StopMulticastMediaTransmission"},
  {0x0105, "OpenReceiveChannel"},
  {0x0106, "CloseReceiveChannel"},
  {0x0107, "ConnectionStatisticsReq"},
  {0x0108, "SoftKeyTemplateResMessage"},
  {0x0109, "SoftKeySetResMessage"},
  {0x0110, "SelectSoftKeysMessage"},
  {0x0111, "CallStateMessage"},
  {0x0112, "DisplayPromptStatusMessage"},
  {0x0113, "ClearPromptStatusMessage"},
  {0x0114, "DisplayNotifyMessage"},
  {0x0115, "ClearNotifyMessage"},
  {0x0116, "ActivateCallPlaneMessage"},
  {0x0117, "DeactivateCallPlaneMessage"},
  {0x0118, "UnregisterAckMessage"},
  {0x0119, "BackSpaceReqMessage"},
  {0x011A, "RegisterTokenAck"},
  {0x011B, "RegisterTokenReject"},
  {0x011D, "unknownForwardMessage1"},
  {0      , NULL}	/* terminator */
};



static const codec_def codec_list [] = {
	{1   , "Non-standard codec",	0},
	{2   , "G.711 A-law 64k",	AST_FORMAT_ALAW},
	{3   , "G.711 A-law 56k",	0}, // 56k variants not currently supported
	{4   , "G.711 u-law 64k",	AST_FORMAT_ULAW},
	{5   , "G.711 u-law 56k",	0},
	{6   , "G.722 64k",		0}, // G722 not currently supported by Asterisk
	{7   , "G.722 56k",		0},
	{8   , "G.722 48k",		0},
	{9   , "G.723.1",		AST_FORMAT_G723_1},
	{10  , "G.728",			0},
	{11  , "G.729",			0},
	{12  , "G.729 Annex A",		AST_FORMAT_G729A},
	{13  , "IS11172 AudioCap",	0},/* IS11172 is an ISO MPEG standard */
	{14  , "IS13818 AudioCap",	0},/* IS13818 is an ISO MPEG standard */
	{15  , "G.729 Annex B",		0},
	{16  , "G.729 Annex A + B",	0},
	{18  , "GSM Full Rate",		0},
	{19  , "GSM Half Rate",		0},
	{20  , "GSM Enhanced Full Rate",0},
	{25  , "Wideband 256k",		0},
	{32  , "Data 64k",		0},
	{33  , "Data 56k",		0},
	{80  , "GSM",			0},
	{81  , "ActiveVoice",		0},
	{82  , "G.726 32K",		0},
	{83  , "G.726 24K",		0},
	{84  , "G.726 16K",		0},
	{85  , "G.729 Annex B",		0},
	{86  , "G.729B Low Complexity",	0},
	{0   , NULL,			0}
};

static const value_string tone_list [] = {
  {0    , "Silence"},
  {1    , "Dtmf1"},
  {2    , "Dtmf2"},
  {3    , "Dtmf3"},
  {4    , "Dtmf4"},
  {5    , "Dtmf5"},
  {6    , "Dtmf6"},
  {7    , "Dtmf7"},
  {8    , "Dtmf8"},
  {9    , "Dtmf9"},
  {0xa  , "Dtmf0"},
  {0xe  , "DtmfStar"},
  {0xf  , "DtmfPound"},
  {0x10 , "DtmfA"},
  {0x11 , "DtmfB"},
  {0x12 , "DtmfC"},
  {0x13 , "DtmfD"},
  {0x21 , "InsideDialTone"}, // OK
  {0x22 , "OutsideDialTone"}, // OK
  {0x23 , "LineBusyTone"}, // OK
  {0x24 , "AlertingTone"}, // OK
  {0x25 , "ReorderTone"}, // OK
  {0x26 , "RecorderWarningTone"},
  {0x27 , "RecorderDetectedTone"},
  {0x28 , "RevertingTone"},
  {0x29 , "ReceiverOffHookTone"},
  {0x2a , "PartialDialTone"},
  {0x2b , "NoSuchNumberTone"},
  {0x2c , "BusyVerificationTone"},
  {0x2d , "CallWaitingTone"}, // OK
  {0x2e , "ConfirmationTone"},
  {0x2f , "CampOnIndicationTone"},
  {0x30 , "RecallDialTone"},
  {0x31 , "ZipZip"}, // OK
  {0x32 , "Zip"}, // OK
  {0x33 , "BeepBonk"}, // OK (Beeeeeeeeeeeeeeeeeeeeeeeep)
  {0x34 , "MusicTone"},
  {0x35 , "HoldTone"}, // OK
  {0x36 , "TestTone"},
  {0x40 , "AddCallWaiting"},
  {0x41 , "PriorityCallWait"},
  {0x42 , "RecallDial"},
  {0x43 , "BargIn"},
  {0x44 , "DistinctAlert"},
  {0x45 , "PriorityAlert"},
  {0x46 , "ReminderRing"},
  {0x50 , "MF1"},
  {0x51 , "MF2"},
  {0x52 , "MF3"},
  {0x53 , "MF4"},
  {0x54 , "MF5"},
  {0x55 , "MF6"},
  {0x56 , "MF7"},
  {0x57 , "MF8"},
  {0x58 , "MF9"},
  {0x59 , "MF0"},
  {0x5a , "MFKP1"},
  {0x5b , "MFST"},
  {0x5c , "MFKP2"},
  {0x5d , "MFSTP"},
  {0x5e , "MFST3P"},
  {0x5f , "MILLIWATT"}, // OK
  {0x60 , "MILLIWATTTEST"},
  {0x61 , "HIGHTONE"},
  {0x62 , "FLASHOVERRIDE"},
  {0x63 , "FLASH"},
  {0x64 , "PRIORITY"},
  {0x65 , "IMMEDIATE"},
  {0x66 , "PREAMPWARN"},
  {0x67 , "2105HZ"},
  {0x68 , "2600HZ"},
  {0x69 , "440HZ"},
  {0x6a , "300HZ"},
  {0x7f , "NoTone"},
  {0   , NULL}
};

static const value_string deviceStimuli[] = {
  {1    , "LastNumberRedial"},
  {2    , "SpeedDial"},
  {3    , "Hold"},
  {4    , "Transfer"},
  {5    , "ForwardAll"},
  {6    , "ForwardBusy"},
  {7    , "ForwardNoAnswer"},
  {8    , "Display"},
  {9    , "Line"},
  {0xA  , "T120Chat"},
  {0xB  , "T120Whiteboard"},
  {0xC  , "T120ApplicationSharing"},
  {0xD  , "T120FileTransfer"},
  {0xE  , "Video"},
  {0xF  , "VoiceMail"},
  {0x11 , "AutoAnswer"},
  {0x21 , "GenericAppB1"},
  {0x22 , "GenericAppB2"},
  {0x23 , "GenericAppB3"},
  {0x24 , "GenericAppB4"},
  {0x25 , "GenericAppB5"},
  {0x7b , "MeetMeConference"},
  {0x7d , "Conference=0x7d"},
  {0x7e , "CallPark=0x7e"},
  {0x7f , "CallPickup"},
  {0x80 , "GroupCallPickup=80"},
  {0,NULL}
};

static const value_string buttonDefinitions[] = {
  {1    , "LastNumberRedial"},
  {2    , "SpeedDial"},
  {3    , "Hold"},
  {4    , "Transfer"},
  {5    , "ForwardAll"},
  {6    , "ForwardBusy"},
  {7    , "ForwardNoAnswer"},
  {8    , "Display"},
  {9    , "Line"},
  {0xa  , "T120Chat"},
  {0xb  , "T120Whiteboard"},
  {0xc  , "T120ApplicationSharing"},
  {0xd  , "T120FileTransfer"},
  {0xe  , "Video"},
  {0x10 , "AnswerRelease"},
  {0xf0 , "Keypad"},
  {0xfd , "AEC"},
  {0xff , "Undefined"},
  {0   , NULL}
};

static const value_string alarmSeverities[] = {
  {0   , "Critical"},
  {1   , "Warning"},
  {2   , "Informational"},
  {4   , "Unknown"},
  {7   , "Major"},
  {8   , "Minor"},
  {10  , "Marginal"},
  {20  , "TraceInfo"},
  {0   , NULL}
};

static const value_string device_types[] = {
  {1  , "30SPplus"},
  {2  , "12SPplus"},
  {3  , "12SP"},
  {4  , "12"},
  {5  , "30VIP"},
  {6  , "Telecaster"},
  {7  , "TelecasterMgr"},
  {8  , "TelecasterBus"},
  {9  , "ConferencePhone7935"},
  {20 , "Virtual30SPplus"},
  {21 , "PhoneApplication"},
  {30 , "AnalogAccess"},
  {40 , "DigitalAccessPRI"},
  {41 , "DigitalAccessT1"},
  {42 , "DigitalAccessTitan2"},
  {47 , "AnalogAccessElvis"},
  {49 , "DigitalAccessLennon"},
  {50 , "ConferenceBridge"},
  {51 , "ConferenceBridgeYoko"},
  {60 , "H225"},
  {61 , "H323Phone"},
  {62 , "H323Trunk"},
  {70 , "MusicOnHold"},
  {71 , "Pilot"},
  {72 , "TapiPort"},
  {73 , "TapiRoutePoint"},
  {80 , "VoiceInBox"},
  {81 , "VoiceInboxAdmin"},
  {82 , "LineAnnunciator"},
  {90 , "RouteList"},
  {100, "LoadSimulator"},
  {110, "MediaTerminationPoint"},
  {111, "MediaTerminationPointYoko"},
  {120, "MGCPStation"},
  {121, "MGCPTrunk"},
  {122, "RASProxy"},
  {255, "NotDefined"},
  {30006, "Cisco7970"},
  {30008, "Cisco7902"},
  { 0    , NULL}
};


#define MAX_TsCallStatusText 14
extern char * TsCallStatusText[MAX_TsCallStatusText];
int handle_message(sccp_moo_t * r, sccp_session_t * s);

typedef void sk_func (sccp_device_t * d, sccp_line_t * l, sccp_channel_t * c);

typedef struct {
  int       id;
  char    * txt;
  sk_func * func;
} softkeytypes;

#include "sccp_softkeys.h"

static const softkeytypes button_labels [] = {
  {  1, "Redial",   sccp_sk_redial },
  {  2, "NewCall",  sccp_sk_newcall },
  {  3, "Hold",     sccp_sk_hold },
  {  4, "Trnsfer",  sccp_sk_transfer },
  {  5, "CFwdAll",  sccp_sk_cfwd_all },
  {  6, "CFwdBusy", sccp_sk_cfwd_busy },
  {  7, "CFwdNoAnswer", sccp_sk_cfwd_noanswer },
  {  8, "<<",	    sccp_sk_back},
  {  9, "EndCall",  sccp_sk_endcall },
  { 10, "Resume",   sccp_sk_resumecall },
  { 11, "Answer",   sccp_sk_answer },
  { 12, "Info",     sccp_sk_info },
  { 13, "Confrn",   sccp_sk_conference },
  { 14, "Park",	    sccp_sk_parkcall },
  { 15, "Join",     sccp_sk_join },
  { 16, "MeetMe",   sccp_sk_meetme },
  { 17, "PickUp",   sccp_sk_pickup },
  { 18, "GPickUp",  sccp_sk_pickup_group },
  { 19, "RmLstC",   NULL }, // Remove LastCall ???
  { 20, "Barge",    NULL },
  { 21, "Barge",    NULL },
  {  0, NULL, NULL}
};

typedef struct {
  int labelId;
  void * callBack;
} btndef;

typedef struct {
  int   setId;
  const btndef * ptr;
} softkey_modes;


#define KEYMODE_ONHOOK 		0 
#define KEYMODE_CONNECTED  	1
#define KEYMODE_ONHOLD      	2
#define KEYMODE_RINGIN          3
#define KEYMODE_OFFHOOK         4
#define	KEYMODE_CONNTRANS       5
#define	KEYMODE_DIGITSFOLL      6
#define	KEYMODE_CONNCONF        7
#define	KEYMODE_RINGOUT         8
#define	KEYMODE_OFFHOOKFEAT     9
#define KEYMODE_MYST		10

static const btndef skSet_Onhook [] = {
	 { 1, sccp_sk_redial  		},
	 { 2, sccp_sk_newcall 		},
	 { 5, NULL 			}, //CallFwd
	 { 3, sccp_sk_hold 		},
	 { 9, sccp_sk_endcall 		},
	 { 10, sccp_sk_resumecall	}, //Resume was NULL
//	 { 11, sccp_sk_answer 		},
	 { 16, sccp_sk_conference	},	
	 { 17, NULL 			}, //CallPickup
	 { 18, NULL			}, //GrpCallPickup
	 { 4, sccp_sk_transfer		}, 
	 { 14, NULL			}, //Park
	 { 13, sccp_sk_conference       },
         { 0, NULL                      },
	 { 0, NULL 			},
	 { 0, NULL 			},
	 { 0, NULL  			}
};

static const btndef skSet_Connected [] = {
         { 3, sccp_sk_hold              },
         { 9, sccp_sk_endcall           },
         { 4, sccp_sk_transfer          },
	 { 14, NULL                     }, //Park
	 { 13, sccp_sk_conference       },
	 { 19, NULL			}, //Unknown
	 { 10, sccp_sk_resumecall       }, //XXX Call Resume
	 { 0, NULL                      },
	 { 0, NULL                      },
	 { 0, NULL                      },
	 { 0, NULL                      },
	 { 0, NULL                      },
	 { 0, NULL                      },
	 { 0, NULL                      },
	 { 0, NULL                      },
	 { 0, NULL                      },
};

static const btndef skSet_Onhold [] = {
	{ 10, sccp_sk_hold             },
	{ 2, sccp_sk_newcall           },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
};

static const btndef skSet_Ringin [] = {
	{ 11, sccp_sk_answer           },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
};

static const btndef skSet_Offhook [] = {
	{ 1, sccp_sk_redial            },
	{ 9, sccp_sk_endcall           },
	{ 5, NULL                      },
	{ 16, sccp_sk_conference       },
	{ 17, NULL		       },
	{ 18, NULL	               },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
};

static const btndef skSet_Conntrans []  = {
	{ 0, 0x0                       },
	{ 9, sccp_sk_endcall           },
	{ 4, sccp_sk_transfer	       },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
};

static const btndef skSet_DigitsFoll []  = {
	{ 8, sccp_sk_back	       },
	{ 9, sccp_sk_endcall           },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
};

static const btndef skSet_Connconf []  = {
	{ 0, NULL                      },
	{ 9, sccp_sk_endcall           },
	{ 13, sccp_sk_endcall	       },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
};

static const btndef skSet_RingOut [] = {
	{ 0, NULL                      },
	{ 9, sccp_sk_endcall           },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
};

static const btndef skSet_Offhookfeat [] = {
	{ 1, sccp_sk_redial            },
	{ 9, sccp_sk_endcall           },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
};

static const btndef skSet_Myst [] = {
	{ 21, NULL		       },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
	{ 0, NULL                      },
};


static const softkey_modes SoftKeyModes [] = {
/* According to CCM dump:
 OnHook(0), Connected(1), OnHold(2), RingIn(3)
 OffHook(4), ConnectedWithTransfer(5)
 Digitsafterdialingfirstdigit(6), Connected with Conference (7)
 RingOut (8), OffHookWithFeatures (9), Unknown (10)
 */
  { KEYMODE_ONHOOK,        skSet_Onhook },
  { KEYMODE_CONNECTED,	   skSet_Connected },
  { KEYMODE_ONHOLD,	   skSet_Onhold },
  { KEYMODE_RINGIN,	   skSet_Ringin },
  { KEYMODE_OFFHOOK,	   skSet_Offhook },
  { KEYMODE_CONNTRANS,	   skSet_Conntrans },
  { KEYMODE_DIGITSFOLL,	   skSet_DigitsFoll },
  { KEYMODE_CONNCONF,	   skSet_Connconf },
  { KEYMODE_RINGOUT,	   skSet_RingOut },
  { KEYMODE_OFFHOOKFEAT,   skSet_Offhookfeat },
  { KEYMODE_MYST,	   skSet_Myst }, 
  /* Strange stuff, but anyway */
  { 0, NULL }
};

static const btnlist layout_12SPPLUS [] = {
  { BtLine },
  { BtLine },
  { BtLastNumberRedial },
  { BtSpeedDial },
  { BtSpeedDial },
  { BtSpeedDial },
  { BtHold},
  { BtTransfer },
  { BtForwardAll },
  { BtCallPark },
  { BtVoiceMail },
  { BtConference },
};

static const btnlist layout_30VIP [] = {
  { BtLine },                           // 1 - 4 Line
  { BtLine },                           // 2
  { BtLine },                           // 3
  { BtLine },                           // 4
  { BtSpeedDial },                      // 5 -> 13 Speed Dials
  { BtSpeedDial },                      // 6
  { BtSpeedDial },                      // 7
  { BtSpeedDial },                      // 8
  { BtSpeedDial },                      // 9
  { BtSpeedDial },                      // 10
  { BtSpeedDial },                      // 11
  { BtSpeedDial },                      // 12
  { BtSpeedDial },                      // 13

  // Column 2
  { BtVoiceMail },                      // 14 Message waiting indicator
  { BtForwardAll },                     // 15 Call forward
  { BtConference },                     // 16 Conf.
  { BtCallPark },                       // 17 Call Parking
  { BtSpeedDial },                      // 18 -> 26 Speed dials
  { BtSpeedDial },                      // 19
  { BtSpeedDial },                      // 20
  { BtSpeedDial },                      // 21
  { BtSpeedDial },                      // 22
  { BtSpeedDial },                      // 23
  { BtSpeedDial },                      // 24
  { BtSpeedDial },                      // 25
  { BtSpeedDial },                      // 26

  // Note that the last 4 button (hold, transfer, redial, and display are hard programmed
  // Anybody know what the display button does?
};

static const btnlist layout_7902 [] = {
	{BtLine},
	{BtHold},
	{BtTransfer},
	{BtDisplay},
	{BtVoiceMail},
	{BtConference},
	{BtForwardAll},
	{BtSpeedDial},
	{BtSpeedDial},
	{BtSpeedDial},
	{BtSpeedDial},
	{BtLastNumberRedial},
};

static const btnlist layout_7905 [] = {
	{BtLine},
	{BtHold},
};

static const btnlist layout_7910 [] = {
  {BtLine},
  {BtHold},
  {BtTransfer},
  {BtDisplay},
  {BtVoiceMail},
  {BtConference},
  {BtForwardAll},
  {BtSpeedDial},
  {BtSpeedDial},
  {BtLastNumberRedial},
};

static const btnlist layout_7920 [] = {
  {BtLine},
  {BtLine},
  {BtSpeedDial},
  {BtSpeedDial},
  {BtSpeedDial},
  {BtSpeedDial},
};

static const btnlist layout_7935 [] = {
  { BtLine },
  { BtLine },
};

static const btnlist layout_7940 [] = {
  { BtLine },
  { BtLine },
};

static const btnlist layout_7960 [] = {
  { BtLine },
  { BtLine },
  { BtLine },
  { BtSpeedDial },
  { BtSpeedDial },
  { BtSpeedDial },
};

static const btnlist layout_7970 [] = {
  // Cisco 7970 stuff
  { BtLine },
  { BtLine },
  { BtLine },
  { BtSpeedDial },
  { BtSpeedDial },
  { BtSpeedDial },
  { BtSpeedDial },
  { BtSpeedDial },
};

static const btnlist layout_7960_7914 [] = {
  // 7960
  { BtLine },
  { BtUnused },
  { BtUnused },
  { BtUnused },
  { BtUnused },
  { BtUnused },
  // 7914
  { BtLine },
  { BtSpeedDial },
  { BtSpeedDial },
  { BtSpeedDial },
  { BtSpeedDial },
  { BtSpeedDial },
  { BtSpeedDial },
  { BtSpeedDial },
  { BtSpeedDial },
  { BtSpeedDial },
  { BtSpeedDial },
  { BtSpeedDial },
  { BtSpeedDial },
  { BtSpeedDial },
};

static const button_modes default_layouts [] = {
  { "12",   12, layout_12SPPLUS }, // Two colums of six buttons
  { "30",   26, layout_30VIP }, // Two colums of 13 buttons (26), plus 4 fixed feature
  { "7902", 10, layout_7902 },
  { "7905", 10, layout_7905 },
  { "7910", 10, layout_7910 },
  { "7914", 20, layout_7960_7914 },
  { "7920", 6,  layout_7920 },
  { "7935", 2,  layout_7935 },
  { "7940", 2,  layout_7940 },
  { "7960", 6,  layout_7960 },
  { "7970", 8,  layout_7970 },
  {   NULL, 0 },
};

/* XXX: remove it if not needed
extern ast_mutex_t      intercomlock;
extern ast_mutex_t      devicelock;
extern ast_mutex_t      chanlock;
extern ast_mutex_t      sessionlock;
extern ast_mutex_t      linelock;
extern ast_mutex_t      usecnt_lock;
*/

extern sccp_session_t * sessions;
extern sccp_device_t  * devices;
extern sccp_intercom_t* intercoms;
extern sccp_line_t    * lines;
extern sccp_channel_t * chans;
extern int              global_capability;
extern struct ast_codec_pref global_codecs;
extern int              keepalive;
extern int              usecnt;
extern int              sccp_debug;
extern struct in_addr   __ourip;
extern char             date_format[6];
extern struct sched_context * sccp_sched;
extern char		vmnum[AST_MAX_EXTENSION];


#ifdef ASTERISK_VERSION_v1_0
struct ast_channel *sccp_request(char *type, int format, void *data);
#endif
#ifdef ASTERISK_VERSION_HEAD
struct ast_channel *sccp_request(const char *type, int format, void *data, int *cause);
#endif

int sccp_devicestate(void *data);

#endif /* __CHAN_SCCP_H */

