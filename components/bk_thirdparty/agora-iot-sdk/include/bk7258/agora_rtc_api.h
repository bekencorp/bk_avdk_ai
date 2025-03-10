/*************************************************************
 *
 * This is a part of the Agora RTC Service SDK.
 * Copyright (C) 2020 Agora IO
 * All rights reserved.
 *
 *************************************************************/

#ifndef __AGORA_RTC_API_H__
#define __AGORA_RTC_API_H__

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_MSC_VER)
#if defined(__BUILDING_AGORA_SDK__)
#define __agora_api__ __declspec(dllexport)
#else
#define __agora_api__ __declspec(dllimport)
#endif
#else
#define __agora_api__
#endif

#define AGORA_RTC_CHANNEL_NAME_MAX_LEN (64)
#define AGORA_RTC_USER_ID_MAX_LEN (255)
#define AGORA_RTC_PRODUCT_ID_MAX_LEN (63)
#define AGORA_RTM_UID_MAX_LEN 64
#define AGORA_RTM_DATA_MAX_LEN (31 * 1024)
#define AGORA_CREDENTIAL_MAX_LEN 256
#define AGORA_CERTIFICATE_MAX_LEN 1024
#define AGORA_LICENSE_VALUE_LEN 32

/** Error code. */
typedef enum {
/******************************************************************************|
|                               Common(0~99)                                   |
*******************************************************************************/
#if 1 // Common Start
  /** No error. */
  ERR_OKAY = 0,
  /** General error */
  ERR_FAILED = 1,
  // Invalid argument
  ERR_INVALID_PARAM = 2,
  // Not invalid state to call api, such as can not send audio/video when mute local
  ERR_INVALID_STATE = 3,
  // This interface is not support
  ERR_NOT_SUPPORTED = 4,
  // Connection not in join channel successfully status
  ERR_NOT_IN_CHANNEL = 6,
   // Agora Service is not initialized, please call agora_rtc_init firstly
  ERR_NOT_INITIALIZED = 7,
  // Connection is not exist
  ERR_NOT_EXIST_CONNECTION = 8,
  /**
   * An API timeout occurs. Some API methods require the SDK to return the
   * execution result, and this error occurs if the request takes too long
   * (more than 10 seconds) for the SDK to process.
   */
  ERR_TIMEDOUT = 10,
  // No memory available
  ERR_NO_MEMORY = 11,
   // API async or sync call failed
  ERR_CALL_FAILED = 12,
  // Network is unavailable
  ERR_NET_DOWN = 14,
  /**
   * Request to join channel is rejected.
   * It occurs when local user is already in channel and try to join the same channel again.
   */
  ERR_JOIN_CHANNEL_REJECTED = 17,
  /**
   * Request to leave channel is rejected.
   * It occurs when local user is already not in channel and try to leave the channel.
   */
  ERR_LEAVE_CHANNEL_REJECTED = 18,
  // SDK initialization failed due to license unverified(for old license version)
  ERR_LICENSE_VERIFICATION_FAILURE = 23,
  // Encrption error
  ERR_ENCRYPTION = 30,
  // Decryption error
  ERR_DECRYPTION = 31,

#endif // Common End

/******************************************************************************|
|                               RTN(100~199)                                   |
*******************************************************************************/
#if 1 // RTN Start
  /** App ID is invalid. */
  ERR_INVALID_APP_ID = 101,
  /** Channel is invalid. */
  ERR_INVALID_CHANNEL_NAME = 102,
  /** Fails to get server resources in the specified region. */
  ERR_NO_SERVER_RESOURCES = 103,
  /** Server rejected request to look up channel. */
  ERR_LOOKUP_CHANNEL_REJECTED = 105,
  /** Server rejected request to open channel. */
  ERR_OPEN_CHANNEL_REJECTED = 107,
  /*
   * Token expired due to reasons belows:
   * - Authorized Timestamp expired:      The timestamp is represented by the number of
   *                                      seconds elapsed since 1/1/1970. The user can use
   *                                      the Token to access the Agora service within five
   *                                      minutes after the Token is generated. If the user
   *                                      does not access the Agora service after five minutes,
   *                                      this Token will no longer be valid.
   * - Call Expiration Timestamp expired: The timestamp indicates the exact time when a
   *                                      user can no longer use the Agora service (for example,
   *                                      when a user is forced to leave an ongoing call).
   *                                      When the value is set for the Call Expiration Timestamp,
   *                                      it does not mean that the Token will be expired,
   *                                      but that the user will be kicked out of the channel.
   */
  ERR_TOKEN_EXPIRED = 109,
  /**
  * Token is invalid due to reasons belows:
  * - If application certificate is enabled on the Dashboard,
  *   valid token SHOULD be set when invoke.
  *
  * - If uid field is mandatory, users must specify the same uid as the one used to generate the token,
  *   when calling `agora_rtc_join_channel`.
  */
  ERR_INVALID_TOKEN = 110,
  /** Dynamic token has been enabled, but is not provided when joining the channel.
   *  Please specify the valid token when calling `agora_rtc_join_channel`.
   */
  ERR_DYNAMIC_TOKEN_BUT_USE_STATIC_KEY = 115,
  /** Switching roles failed.
   *  Please try to rejoin the channel.
   */
  ERR_SET_CLIENT_ROLE_NOT_AUTHORIZED = 119,
  /** Decryption fails. The user may have used a different encryption password to join the channel.
   *  Check your settings or try rejoining the channel.
   */
  ERR_DECRYPTION_FAILED = 120,
  /* Ticket to open channel is invalid */
  ERR_OPEN_CHANNEL_INVALID_TICKET = 121,
  /* Try another server. */
  ERR_OPEN_CHANNEL_TRY_NEXT_VOS = 122,
  /** Client is banned by the server */
  ERR_CLIENT_IS_BANNED_BY_SERVER = 123,

#endif // RTN End

/******************************************************************************|
|                               Audio(200~299)                                 |
*******************************************************************************/
#if 1 // Audio Start
  // Try use audio codec but SDK doesn't support
  ERR_AUDIO_CODEC_NOT_SUPPORT = 200,
  // Invalid opus frame size
  ERR_AUDIO_INVALID_PCM_LEN = 201,
  // Invalid audio codec param
  ERR_AUDIO_INVALID_CODEC_PARAM = 202,
  /** Audio decoder does not match incoming audio data type.
   *  The audio codec of send and recv must be the same
   */
  ERR_AUDIO_DECODER_NOT_MATCH = 221,
  /** Audio decoder does not enable
   *  Currently SDK built-in audio codec only supports G722 and OPUS.
   */
  ERR_AUDIO_DECODER_NOT_ENABLE = 222,
#endif // Audio End

/******************************************************************************|
|                               Video(300~399)                                 |
*******************************************************************************/
#if 1 // Video Start
  /** Sending video data too fast and over the bandwidth limit.
   *  Very likely that packet loss occurs with this sending speed.
   */
  ERR_VIDEO_SEND_OVER_BANDWIDTH_LIMIT = 300,
  // Video packet packetizer failed
  ERR_VIDEO_SEND_PACKETIZE_FAILED = 301,
#endif // Video End

/******************************************************************************|
|                               RDT(600~620)                                   |
*******************************************************************************/
#if 1 // RDT Start
  ERR_RDT_USER_NOT_EXIST = 600,
  ERR_RDT_USER_NOT_READY = 601,
  ERR_RDT_DATA_BLOCKED = 602,
  ERR_RDT_CMD_EXCEED_LIMIT = 603,
  ERR_RDT_DATA_EXCEED_LIMIT = 604,
#endif // RDT End

/******************************************************************************|
|                               RTM(1000~1099)                                 |
*******************************************************************************/
#if 1 // RTM Start
  ERR_RTM_NOT_LOGINED = 1001,
  ERR_RTM_HAD_LOGINED = 1002,
  ERR_RTM_SEND_TO_SELF = 1003,
  ERR_RTM_EXCEED_MSG_SIZE = 1004,
  ERR_RTM_EXCEED_MSG_CNT = 1005,
  ERR_RTM_EXCEED_TCP_SKB = 1006,
#endif // RTM End

} agora_err_code_e;

typedef enum {
  /**
   * 1: Invalid license
  */
  ERR_LICENSE_INVALID = 1,
  /**
   * 2: License expired
  */
  ERR_LICENSE_EXPIRE = 2,
  /**
   * 3: Exceed license minutes limit
  */
  ERR_LICENSE_MINUTES_EXCEED = 3,
  /**
   * 4: License use in limited period
  */
  ERR_LICENSE_LIMITED_PERIOD = 4,
  /**
   * 5: Same license used in different devices at the same time
  */
  ERR_LICENSE_DIFF_DEVICES = 5,
  /**
   * 99: SDK internal error
  */
  ERR_LICENSE_INTERNAL = 99,
} license_err_reason_e;

/**
 * The definition of the user_offline_reason_e enum.
 */
typedef enum {
  /**
   * 0: Remote user leaves channel actively
   */
  USER_OFFLINE_QUIT = 0,
  /**
   * 1: Remote user is dropped due to timeout
   */
  USER_OFFLINE_DROPPED = 1,
  /**
   * 2: The user switches the client role from the host to the audience.
   */
  USER_OFFLINE_BECOME_AUDIENCE = 2,
} user_offline_reason_e;

/**
 * The definition of the video_data_type_e enum.
 */
typedef enum {
  /* 0: YUV420 */
  VIDEO_DATA_TYPE_YUV420 = 0,
  /* 2: H264 */
  VIDEO_DATA_TYPE_H264 = 2,
  /* 3: H265 */
  VIDEO_DATA_TYPE_H265 = 3,
  /* 6: generic */
  VIDEO_DATA_TYPE_GENERIC = 6,
  /* 20: generic JPEG */
  VIDEO_DATA_TYPE_GENERIC_JPEG = 20,
} video_data_type_e;

/**
 * The definition of the video_frame_type_e enum.
 */
typedef enum {
  /**
   * 0: unknow frame type
   * If you set it ,the SDK will judge the frame type
   */
  VIDEO_FRAME_AUTO_DETECT = 0,
  /**
   * 3: key frame
   */
  VIDEO_FRAME_KEY = 3,
  /*
   * 4: delta frame, e.g: P-Frame
   */
  VIDEO_FRAME_DELTA = 4,
} video_frame_type_e;

typedef uint16_t video_frame_rate_e;

/**
 * Video stream types.
 */
typedef enum {
  /**
   * 0: The high-quality video stream, which has a higher resolution and bitrate.
   */
  VIDEO_STREAM_HIGH = 0,
  /**
   * 1: The low-quality video stream, which has a lower resolution and bitrate.
   */
  VIDEO_STREAM_LOW = 1,
} video_stream_type_e;

/*
 * Video rotation information.
 */
typedef enum {
  /**
   * 0: Rotate the video by 0 degree clockwise.
   */
  VIDEO_ORIENTATION_0 = 0,
  /**
   * 90: Rotate the video by 90 degrees clockwise.
   */
  VIDEO_ORIENTATION_90 = 1,
  /**
   * 180: Rotate the video by 180 degrees clockwise.
   */
  VIDEO_ORIENTATION_180 = 2,
  /**
   * 270: Rotate the video by 270 degrees clockwise.
   */
  VIDEO_ORIENTATION_270 = 3
} video_orientation_e;

/**
 * The definition of the video_frame_info_t struct.
 */
typedef struct {
  /**
   * The video data type: #video_data_type_e.
   */
  video_data_type_e data_type;
  /**
   * The video stream type: #video_stream_type_e
   */
  video_stream_type_e stream_type;
  /**
   * The frame type of the encoded video frame: #video_frame_type_e.
   */
  video_frame_type_e frame_type;
  /**
   * The number of video frames per second, can be set 0-60.
   * -This value will be used for calculating timestamps of the encoded image.
   * - If frame_per_sec equals zero, then real timestamp will be used.
   * - Otherwise, timestamp will be adjusted to the value of frame_per_sec set.
   */
  video_frame_rate_e frame_rate;
  /**
   * The rotation information of the encoded video frame: #VIDEO_ORIENTATION.
   */
  video_orientation_e rotation;
} video_frame_info_t;

/**
 * Audio codec type list.
 */
typedef enum {
  /**
   * 0: Disable
   */
  AUDIO_CODEC_DISABLED = 0,
  /**
   * 1: OPUS
   */
  AUDIO_CODEC_TYPE_OPUS = 1,
  /**
   * 2: G722
   */
  AUDIO_CODEC_TYPE_G722 = 2,
  /**
   * 3: G711A
   */
  AUDIO_CODEC_TYPE_G711A = 3,
  /**
   * 4: G711U
   */
  AUDIO_CODEC_TYPE_G711U = 4,
} audio_codec_type_e;

/**
 * Audio data type list.
 */
typedef enum {
  /**
   * 1: OPUS, sample=16k
   */
  AUDIO_DATA_TYPE_OPUS = 1,
  /**
   * 2: OPUSFB, sample=48k
   */
  AUDIO_DATA_TYPE_OPUSFB = 2,
  /**
   * 3: PCMA, sample=8k
   */
  AUDIO_DATA_TYPE_PCMA = 3,
  /**
   * 4: PCMU, sample=8k
   */
  AUDIO_DATA_TYPE_PCMU = 4,
  /**
   * 5: G722, sample=16k
   */
  AUDIO_DATA_TYPE_G722 = 5,
  /*
   * 6: AACLC sample=8k
   */
  AUDIO_DATA_TYPE_AACLC_8K = 6,
  /*
   * 7: AACLC sample=16k
   */
  AUDIO_DATA_TYPE_AACLC_16K = 7,
  /**
   * 8: AACLC, sample=48k
   */
  AUDIO_DATA_TYPE_AACLC = 8,
  /**
   * 9: HEAAC, sample=32k
   */
  AUDIO_DATA_TYPE_HEAAC = 9,
  /**
   * 100: PCM (audio codec should be enabled)
   */
  AUDIO_DATA_TYPE_PCM = 100,
  /**
   * 253: GENERIC
   */
  AUDIO_DATA_TYPE_GENERIC = 253,
} audio_data_type_e;

/**
 * The definition of the audio_frame_info_t struct.
 */
typedef struct {
  /**
   * Audio data type, reference #audio_data_type_e.
   */
  audio_data_type_e data_type;
} audio_frame_info_t;

/**
 * The definition of log level enum
 */
typedef enum {
  RTC_LOG_DEFAULT = 0, // the same as RTC_LOG_NOTICE
  RTC_LOG_EMERG, // system is unusable
  RTC_LOG_ALERT, // action must be taken immediately
  RTC_LOG_CRIT, // critical conditions
  RTC_LOG_ERROR, // error conditions
  RTC_LOG_WARNING, // warning conditions
  RTC_LOG_NOTICE, // normal but significant condition, default level
  RTC_LOG_INFO, // informational
  RTC_LOG_DEBUG, // debug-level messages
} rtc_log_level_e;

/**
 * IP areas.
 */
typedef enum {
  /* the same as AREA_CODE_GLOB */
  AREA_CODE_DEFAULT = 0x00000000,
  /* Mainland China. */
  AREA_CODE_CN = 0x00000001,
  /* North America. */
  AREA_CODE_NA = 0x00000002,
  /* Europe. */
  AREA_CODE_EU = 0x00000004,
  /* Asia, excluding Mainland China. */
  AREA_CODE_AS = 0x00000008,
  /* Japan. */
  AREA_CODE_JP = 0x00000010,
  /* India. */
  AREA_CODE_IN = 0x00000020,
  /* Oceania */
  AREA_CODE_OC = 0x00000040,
  /* South-American */
  AREA_CODE_SA = 0x00000080,
  /* Africa */
  AREA_CODE_AF = 0x00000100,
  /* South Korea */
  AREA_CODE_KR = 0x00000200,
  /* Hong Kong and Macou */
  AREA_CODE_HKMC = 0x00000400,
  /* United States */
  AREA_CODE_US = 0x00000800,
  /* Russia */
  AREA_CODE_RU = 0x00001000,
  /* The global area (except China) */
  AREA_CODE_OVS = 0xFFFFFFFE,
  /* (Default) Global. */
  AREA_CODE_GLOB = (0xFFFFFFFF),
} area_code_e;

/**
 * log config
 */
typedef struct {
  bool log_disable;
  bool log_disable_desensitize;
  rtc_log_level_e log_level;
  const char *log_path;
  const char *log_tag;
  int (*log_printf)(const char *fmt, ...);
} log_config_t;

/**
 * The definition of the service option
 */
typedef struct {
  /**
   * area code for regional restriction, default is golbal, Supported areas refer to enum area_code_e
   * and bit operations for multiple regions are supported
   */
  uint32_t area_code;
  /**
   * the  product_id (device name), the max product_id length is 63
   */
  char product_id[AGORA_RTC_PRODUCT_ID_MAX_LEN + 1];
  /**
   * sdk log config
   */
  log_config_t log_cfg;
  /**
   * license value return when license actived, the max license_value length is 32
   * Supported character scopes are:
   * - The 26 lowercase English letters: a to z
   * - The 26 uppercase English letters: A to Z
   * - The 10 numbers: 0 to 9
   */
  char license_value[AGORA_LICENSE_VALUE_LEN + 1];
  /**
   * Determines whether to enable domain limit.
   * - `true`: only connect to servers that already parsed by DNS
   * - `false`: (Default) connect to servers with no limit
   */
  bool domain_limit;
} rtc_service_option_t;

typedef struct {
  /**
   * Configure sdk built-in audio codec
   * If AUDIO_CODEC_TYPE_OPUS is selected, your PCM data is encoded as OPUS and then streamed to Agora channel
   * If AUDIO_CODEC_TYPE_G722 is selected, your PCM data is encoded as G722 and then streamed to Agora channel
   * If you provide encoded audio data, such as AAC, instead of raw PCM, please disable audio codec by selecting AUDIO_CODEC_DISABLED
   */
  audio_codec_type_e audio_codec_type;
  /**
   * Pcm sample rate. Ignored if audio coded is diabled
   */
  int pcm_sample_rate;
  /**
   * Pcm channel number. Ignored if audio coded is diabled
   */
  int pcm_channel_num;
  /**
   * Pcm duration. default 20ms
   */
  int pcm_duration;
} audio_codec_option_t;

/**
 * The definition of the rtc_audio_process_options_t struct.
 */
typedef struct{
  // whether to open audio process
  bool enable_audio_process;
  // whether to open uplink aec
  bool enable_aec;
  // whether to open downlink aec;
  bool enable_downlink_aec;
  // whether to open ns
  bool enable_ns;

  bool ref_data_from_sdk;
  // whether to dump audio data from audio process
  bool enable_dump_data;
} rtc_audio_process_options_t;


/**
 * The definition of the rtc_channel_options_t struct.
 */
typedef struct {
  // whether to auto subscribe audio
  bool auto_subscribe_audio;
  // whether to auto subscribe video
  bool auto_subscribe_video;
  // whether to use the audio jitter buffer
  bool enable_audio_jitter_buffer;
  // whether to use the audio mixing function, depends on enable_audio_jitter_buffer
  bool enable_audio_mixer;
  // audio encode and decode configuration when send pcm audio data by #agora_rtc_send_audio_data
  audio_codec_option_t audio_codec_opt;
  // audio process options
  rtc_audio_process_options_t audio_process_opt;


} rtc_channel_options_t;


/**
 * Network event type enum
 */
typedef enum {
  NETWORK_EVENT_DOWN = 0,
  NETWORK_EVENT_UP,
  NETWORK_EVENT_CHANGE,
} network_event_e;


/**
 * Connection identification
 */
typedef uint32_t connection_id_t;

/**
 * Special connection id
 */
// All connections Created
#define CONNECTION_ID_ALL ((connection_id_t)0)
// Invalid connection id
#define CONNECTION_ID_INVALID ((connection_id_t)-1)

/**
 * connection info
 */
typedef struct {
  // Connection identification
  connection_id_t conn_id;
  // user id
  uint32_t uid;
  // channel name
  char channel_name[AGORA_RTC_CHANNEL_NAME_MAX_LEN + 1];
} connection_info_t;

/**
 * Agora RTC SDK event handler
 */
typedef struct {
  /**
   * Occurs when local user joins channel successfully.
   *
   * @param[in] conn_id    Connection identification
   * param[in] uid         local uid
   * @param[in] elapsed_ms Time elapsed (ms) since channel is established
   */
  void (*on_join_channel_success)(connection_id_t conn_id, uint32_t uid, int elapsed_ms);

  /**
   * Occurs when the connection of channel is interrupt or keepalive timeout(4s).
   *
   * @param[in] conn_id    Connection identification
   */
  void (*on_reconnecting)(connection_id_t conn_id);

  /**
   * Occurs when connection of channel is disconnected from the server more than 10s
   *
   * @param[in] conn_id    Connection identification
   */
  void (*on_connection_lost)(connection_id_t conn_id);

  /**
   * Occurs when local user rejoins channel successfully after disconnect
   *
   * When channel loses connection with server due to network problems,
   * SDK will retry to connect automatically. If success, it will be triggered.
   *
   * @param[in] conn_id    Connection identification
   * @param[in] uid        local uid
   * @param[in] elapsed_ms Time elapsed (ms) since rejoin due to network
   */
  void (*on_rejoin_channel_success)(connection_id_t conn_id, uint32_t uid, int elapsed_ms);

  /**
   * Occurs when connection license verification fails
   *
   * You can know the reason accordding to error code
   * @param[in] conn_id Connection identification
   * @param[in] error   Error code, see #license_err_code_e
   */
  void (*on_license_validation_failure)(connection_id_t conn_id, int error);

  /**
   * Report error message during runtime.
   *
   * In most cases, it means SDK can't fix the issue and application should take action.
   *
   * @param[in] conn_id Connection identification
   * @param[in] code    Error code, see #agora_err_code_e
   * @param[in] msg     Error message
   */
  void (*on_error)(connection_id_t conn_id, int code, const char *msg);

  /**
   * Occurs when a remote user joins channel successfully.
   *
   * @param[in] conn_id    Connection identification
   * @param[in] uid        Remote user ID
   * @param[in] elapsed_ms Time elapsed (ms) since channel is established
   */
  void (*on_user_joined)(connection_id_t conn_id, uint32_t uid, int elapsed_ms);

  /**
   * Occurs when remote user leaves the channel.
   *
   * @param[in] conn_id Connection identification
   * @param[in] uid     Remote user ID
   * @param[in] reason  Reason, see #user_offline_reason_e
   */
  void (*on_user_offline)(connection_id_t conn_id, uint32_t uid, int reason);

  /**
   * Occurs when a remote user sends notification before enable/disable sending audio.
   *
   * @param[in] conn_id Connection identification
   * @param[in] uid     Remote user ID
   * @param[in] muted   Mute status:
   *                    - false(0): unmuted
   *                    - true (1): muted
   */
  void (*on_user_mute_audio)(connection_id_t conn_id, uint32_t uid, bool muted);

  /**
   * Occurs when a remote user sends notification before enable/disable sending video.
   *
   * @param[in] conn_id Connection identification
   * @param[in] uid     Remote user ID
   * @param[in] muted   Mute status:
   *                    - false(0): unmuted
   *                    - true (1): muted
   */
  void (*on_user_mute_video)(connection_id_t conn_id, uint32_t uid, bool muted);

  /**
   * Occurs when receiving the audio frame of a remote user in the channel.
   *
   * @param[in] conn_id    Connection identification
   * @param[in] uid        Remote user ID to which data is sent
   * @param[in] sent_ts    Timestamp (ms) for sending data
   * @param[in] data_ptr   Audio frame buffer
   * @param[in] data_len   Audio frame buffer length (bytes)
   * @param[in] frame_info Audio frame info
   */
  void (*on_audio_data)(connection_id_t conn_id, uint32_t uid, uint16_t sent_ts, const void *data_ptr, size_t data_len,
                        const audio_frame_info_t *info_ptr);

  /**
   * Occurs every 20ms.
   *
   * @param[in] conn_id     Connection identification
   * @param[in] data_ptr    Audio frame buffer
   * @param[in] data_len    Audio frame buffer length (bytes)
   * @param[in] frame_info  Audio frame info
   */
  void (*on_mixed_audio_data)(connection_id_t conn_id, const void *data_ptr, size_t data_len,
                              const audio_frame_info_t *info_ptr);

  /**
   * Occurs when receiving the video frame of a remote user in the channel.
   *
   * @param[in] conn_id      Connection identification
   * @param[in] uid          Remote user ID to which data is sent
   * @param[in] sent_ts      Timestamp (ms) for sending data
   * @param[in] data_ptr     Video frame buffer
   * @param[in] data_len     Video frame buffer lenth (bytes)
   * @param[in] frame_info   Video frame info
   */
  void (*on_video_data)(connection_id_t conn_id, uint32_t uid, uint16_t sent_ts, const void *data_ptr, size_t data_len,
                        const video_frame_info_t *info_ptr);

  /**
   * Occurs when network bandwidth change is detected. User is expected to adjust encoder bitrate to |target_bps|
   *
   * @param[in] conn_id    Connection identification
   * @param[in] target_bps Target value (bps) by which the bitrate should update
   */
  void (*on_target_bitrate_changed)(connection_id_t conn_id, uint32_t target_bps);

  /**
   * Occurs when a remote user requests a keyframe.
   *
   * This callback notifies the sender to generate a new keyframe.
   *
   * @param[in] conn_id      Connection identification
   * @param[in] uid          Remote user ID
   * @param[in] stream_type  Stream type for which a keyframe is requested
   */
  void (*on_key_frame_gen_req)(connection_id_t conn_id, uint32_t uid, video_stream_type_e stream_type);


  /**
   * Occurs when token will expired.
   *
   * @param[in] conn_id    Connection identification
   * @param[in] token      The token will expire
   */
  void (*on_token_privilege_will_expire)(connection_id_t conn_id, const char *token);



} agora_rtc_event_handler_t;

/**
 * @brief Get SDK version.
 * @return A const static string describes the SDK version
 */
extern const char *agora_rtc_get_version(void);

/**
 * @brief Convert error code to const static string.
 * @note You do not have to release the string after use.
 * @param[in] err Error code
 * @return Const static error string
 */
extern __agora_api__ const char *agora_rtc_err_2_str(int err);


/**
 * @brief Initialize the Agora RTSA service.
 * @note Each process can only be initialized once.
 * @param[in] app_id          Application ID
 *                            If 'uid' is set as 0, SDK will assign a valid ID to the user
 * @param[in] event_handler   A set of callback that handles Agora SDK events
 * @param[in] option          RTC service option when init, If need't set option, set NULL
 * @return
 * - = 0: Success
 * - < 0: Failure
 */
extern __agora_api__ int agora_rtc_init(const char *app_id, const agora_rtc_event_handler_t *event_handler,
                                        rtc_service_option_t *option);
/**
 * @brief Release all resource allocated by Agora RTSA SDK
 * @return
 * - = 0: Success
 * - < 0: Failure
 */
extern __agora_api__ int agora_rtc_fini(void);

/**
 * @brief Set the log level.
 * @param[in] level Log level. Range reference to rtc_log_level_e enum
 * @return
 * - = 0: Success
 * - < 0: Failure
 */
extern __agora_api__ int agora_rtc_set_log_level(rtc_log_level_e level);

/**
 * @brief Sets the log file configuration.
 * @param[in] per_logfile_size The size (bytes) of each log file.
 *                             The value range is [0, 10*1024*1024(10MB)], default 1*1024*1024(1MB).
 *                             0 means set log off
 * @param[in] logfile_roll_count The maximum number of log file numbers.
 *                               The value range is [0, 100], default 10.
 *                               0 means set log off
 * @return
 * - 0: Success.
 * - <0: Failure.
 */
extern __agora_api__ int agora_rtc_config_log(int size_per_file, int max_file_count);

/**
 * @brief Create a connection, connection can join channel
 * @param[out] conn_id: Store created connection id
 * @return
 *  - =0: Success
 *  - <0: Failure
 */
extern __agora_api__ int agora_rtc_create_connection(connection_id_t *conn_id);

/**
 * @brief Destroy a connection
 * @param[in] conn_id    : Connection identification
 * @return
 *  - =0: Success
 *  - <0: Failure
 */
extern __agora_api__ int agora_rtc_destroy_connection(connection_id_t conn_id);

/**
 * @brief Get a connection info
 * @param[in] conn_id    : Connection identification
 * @param[out] conn_info : Connection info
 * @return
 *  - =0: Success
 *  - <0: Failure
 */
extern __agora_api__ int agora_rtc_get_connection_info(connection_id_t conn_id, connection_info_t *conn_info);

/**
 * @brief Local user joins channel.
 * @note Users in the same channel with the same App ID can send data to each other.
 *       You can join more than one channel at the same time. All channels that
 *       you join will receive the audio/video data stream that you send unless
 *       you stop sending the audio/video data stream in a specific channel.
 * @param[in] conn_id      : Connection identification
 * @param[in] channel_name : Channel name
 *            Length=strlen(channel_name) should be less than 64 bytes
 *            Supported character scopes are:
 *            - The 26 lowercase English letters: a to z
 *            - The 26 uppercase English letters: A to Z
 *            - The 10 numbers: 0 to 9
 *            - The space
 *            - "!", "#", "$", "%", "&", "(", ")", "+", "-", ":", ";", "<",
 *              "=", ".", ">", "?", "@", "[", "]", "^", "_", " {", "}", "|", "~", ","
 * @param[in] uid   : User ID.
 *                   A 32-bit unsigned integer with a value ranging from 1 to 2^32-1. The uid must be unique.
 *                   If a uid is set to 0, the SDK assigns and returns a uid in the on_join_channel_success callback.
 *                     Your application must record and maintain the returned uid, because the SDK does not do so.
 * @param[in] token : Token string generated by the server, length=strlen(token) Range is [32, 512]
 *                      - if token authorization is enabled on developer website, it should be set correctly
 *                      - else token can be set as `NULL`
 * @param[in] options   channel options when create channel.
 *                      If do not set channel options, set NULL
 * @return
 * - = 0: Success
 * - < 0: Failure
 */
extern __agora_api__ int agora_rtc_join_channel(connection_id_t conn_id, const char *channel_name, uint32_t uid,
                                                const char *token, rtc_channel_options_t *options);

/**
 * @brief Allow Local user leaves channel.
 * @note Local user should leave channel when data transmission is stopped
 * @param[in] conn_id   : Connection identification
 * @return
 * - = 0: Success
 * - < 0: Failure
 */
extern __agora_api__ int agora_rtc_leave_channel(connection_id_t conn_id);

/**
 * @brief Renew token for specific channel OR all channels.
 * @note Token should be renewed when valid duration reached expiration.
 * @param[in] conn_id   Connection identification
 * @param[in] token     Token string, strlen(token) Range is [32, 512]
 * @return
 * - = 0: Success
 * - < 0: Failure
 */
extern __agora_api__ int agora_rtc_renew_token(connection_id_t conn_id, const char *token);

/**
 * Set network state
 * @note It should only be used on systems where the SDK is not aware of network events, such as Android11 and later.
 * @param event : network event, ref@network_event_e
 * @return =0: success; <0: failure
 */
extern __agora_api__ int agora_rtc_notify_network_event(network_event_e event);

/**
 * @brief Decide whether to enable/disable sending local audio data to specific channel OR all channels.
 * @param[in] conn_id   Connection identification, if set CONNECTION_ID_ALL(0) is for all connections
 * @param[in] mute      Toggle sending local audio
 *                      - false(0): unmuted
 *                      - true (1): muted
 * @return
 * - = 0: Success
 * - < 0: Failure
 */
extern __agora_api__ int agora_rtc_mute_local_audio(connection_id_t conn_id, bool mute);

/**
 * @brief Decide whether to enable/disable sending local video data to specific channel OR all channels.
 * @param[in] conn_id   Connection identification, if set CONNECTION_ID_ALL(0) is for all connections
 * @param[in] mute      Toggle sending local video
 *                      - false(0): unmuted
 *                      - true (1): muted
 * @return
 * - = 0: Success
 * - < 0: Failure
 */
extern __agora_api__ int agora_rtc_mute_local_video(connection_id_t conn_id, bool mute);
/**
 * @brief Decide whether to enable/disable receiving remote audio data from specific channel OR all channels.
 * @param[in] conn_id      Connection identification, if set CONNECTION_ID_ALL(0) is for all connections
 * @param[in] remote_uid    Remote user ID
 *                          - if `remote_uid` is set 0, it's for all users
 *                          - else it's for specific user
 * @param[in] mute          Toggle receiving remote audio
 *                          - false(0): unmuted
 *                          - true (1): muted
 * @return
 * - = 0: Success
 * - < 0: Failure
 */
extern __agora_api__ int agora_rtc_mute_remote_audio(connection_id_t conn_id, uint32_t remote_uid, bool mute);

/**
 * @brief Decide whether to enable/disable receiving remote video data from specific channel OR all channels.
 * @param[in] conn_id       Connection identification, if set CONNECTION_ID_ALL(0) is for all connections
 * @param[in] remote_uid    Remote user ID
 *                          - if `remote_uid` is set 0, it's for all users
 *                          - else it's for specific user
 * @param[in] mute          Toggle receiving remote video
 *                          - false(0): unmuted
 *                          - true (1): muted
 * @return
 * - = 0: Success
 * - < 0: Failure
 */
extern __agora_api__ int agora_rtc_mute_remote_video(connection_id_t conn_id, uint32_t remote_uid, bool mute);

/**
 * @brief Request remote user to generate a keyframe for all video streams OR specific video stream.
 * @param[in] conn_id      Connection identification
 * @param[in] remote_uid   Remote user ID
 *                         - if `remote_uid` is set 0, it's for all users
 *                         - else it's for specific user
 * @param[in] stream_type    Stream type
 * @return
 * - = 0: Success
 * - < 0: Failure
 */
extern __agora_api__ int agora_rtc_request_video_key_frame(connection_id_t conn_id, uint32_t remote_uid,
                                                           video_stream_type_e stream_type);
/**
 * @brief Send an audio frame to all channels OR specific channel.
 *        All remote users in this channel will receive the audio frame.
 * @note All channels that you joined will receive the audio frame that you send
 *       unless you stop sending the local audio to a specific channel.
 * @param[in] conn_id   Connection identification
 * @param[in] data_ptr  Audio frame buffer
 * @param[in] data_len  Audio frame buffer length (bytes)
 * @param[in] info_ptr  Audio frame info, see #audio_frame_info_t
 * @return
 * - = 0: Success
 * - < 0: Failure
 */
extern __agora_api__ int agora_rtc_send_audio_data(connection_id_t conn_id, const void *data_ptr, size_t data_len,
                                                   audio_frame_info_t *info_ptr);

/**
 * @brief Send a video frame to all channels OR specific channel.
 *        All remote users in the channel will receive the video frame.
 * @note All channels that you join will receive the video frame that you send
 *       unless you stop sending the local video to a specific channel.
 * @param[in] conn_id   Connection identification
 * @param[in] data_ptr  Video frame buffer
 * @param[in] data_len  Video frame buffer length (bytes)
 * @param[in] info_ptr  Video frame info
 * @return
 * - = 0: Success
 * - < 0: Failure
 */
extern __agora_api__ int agora_rtc_send_video_data(connection_id_t conn_id, const void *data_ptr, size_t data_len,
                                                   video_frame_info_t *info_ptr);
/**
 * @brief Set network bandwdith estimation (bwe) param
 * @param[in] conn_id   : Connection identification, if set CONNECTION_ID_ALL(0) is for all connections
 * @param[in] min_bps   : bwe min bps
 * @param[in] max_bps   : bwe max bps
 * @param[in] start_bps : bwe start bps
 *
 * @return:
 * - = 0: Success
 * - < 0: Failure
 */
extern __agora_api__ int agora_rtc_set_bwe_param(connection_id_t conn_id, uint32_t min_bps, uint32_t max_bps,
                                                 uint32_t start_bps);




/**
 * Set config params
 *
 * @param [in] params : config params described by json
 * @note  supported sets are shown below, they can be together in params json string
 *  - {"rtc.encryption": {"enable": true/false, "master_key": "xxx..."}}
 *  - {"rtc_network": { "type": INT_xx, "id": INT_xx, "update": true|false }}
 * @return:
 *  - = 0: Success
 *  - < 0: Failure
 */
extern __agora_api__ int agora_rtc_set_params(const char *params);




#ifdef __cplusplus
}
#endif

#endif /* __AGORA_RTC_API_H__ */
