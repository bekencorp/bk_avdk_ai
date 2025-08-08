/*
 * Copyright 2025 Alibaba Group Holding Ltd.

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at

 *     http: *www.apache.org/licenses/LICENSE-2.0

 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __LIB_C_SDK_H__
#define __LIB_C_SDK_H__

#ifdef __cplusplus
extern "C"{
#endif

#include "c_utils.h"
#include "lib_c_storage.h"

enum {
    WS_DATA_TYPE_TEXT = 0x01,
    WS_DATA_TYPE_BINARY = 0x02,
    WS_DATA_TYPE_DISCONNECT = 0x08,
    WS_DATA_TYPE_PING = 0x09,
    WS_DATA_TYPE_PONG = 0x0A,
};

/**
 * 初始化阿里云SDK。
 * 
 * 返回: 初始化结果，0表示成功，非0表示失败。
 */
int32_t c_mmi_sdk_init(void);

#define C_SDK_REQ_LEN_REGISTER 300

/**
 * 生成设备注册请求所需的字符串。
 * 
 * 参数 req_str: 用于存储生成的设备注册请求字符串的缓冲区。
 * 参数 time_ms_str: 时间戳字符串，单位为毫秒
 * 
 * 返回: 生成结果，0表示成功，非0表示失败。
 */
int32_t c_device_gen_register_req(char req_str[C_SDK_REQ_LEN_REGISTER], char *time_ms_str);

/**
 * 解析云端的设备注册响应信息。
 * 
 * 参数 rsp_str: 指向设备注册响应字符串的指针。
 * 
 * 返回: 分析结果，0表示成功，非0表示失败。
 */
int32_t c_device_analyze_register_rsp(char *rsp_str);

#define C_SDK_REQ_LEN_GET_TOKEN     500
/**
 * @brief 检查mmi登录状态。
 *
 * 返回值表示当前mmi是否已完成登录流程，1为已登录，0为未登录。
 * 
 * @return uint8_t 
 *         返回1表示已登录，返回0表示未登录。
 */
uint8_t c_mmi_is_token_expire(void);

/**
 * @brief 生成mmi登录请求数据。
 *
 * 此函数根据提供的当前时间戳生成mmi登录请求字符串，
 * 
 *
 * @param req_str 存储生成的登录请求字符串的缓冲区
 * @param time_ms_str 时间戳字符串，单位为毫秒
 * @return int32_t 
 *         成功返回0，失败返回非零的错误码
 */
int32_t c_mmi_gen_get_token_req(char req_str[C_SDK_REQ_LEN_GET_TOKEN], char *time_ms_str);

/**
 * @brief 解析mmi登录响应数据。
 *
 * 解析登录响应字符串，并触发事件：
 * - C_MMI_EVENT_USER_CONFIG: 用户配置初始化
 * - C_MMI_EVENT_DATA_INIT: 数据初始化完成
 *
 * @param rsp_str 包含mmi登录响应的字符串
 * @return int32_t 
 *         成功返回0，失败返回非零的错误码
 */
int32_t c_mmi_analyze_get_token_rsp(char *rsp_str);

/**
 * @brief 获取WSS服务器主机名字符串。
 *
 * 此函数返回配置的WSS服务器主机名字符串。
 * 
 * @return char* 
 *         返回指向主机名字符串的指针，若未配置则返回NULL。
 */
char *c_mmi_get_wss_host(void);

/**
 * @brief 获取WSS服务器端口字符串。
 *
 * 此函数返回配置的WSS服务器端口字符串。
 * 
 * @return char* 
 *         返回指向端口字符串的指针，若未配置则返回NULL。
 */
char *c_mmi_get_wss_port(void);

/**
 * @brief 获取WSS服务API路径字符串。
 *
 * 此函数返回配置的WSS服务API路径字符串。
 * 
 * @return char* 
 *         返回指向API路径字符串的指针，若未配置则返回NULL。
 */
char *c_mmi_get_wss_api(void);

/**
 * @brief 获取WSS请求头信息字符串。
 *
 * 此函数返回配置的WSS请求头信息字符串。
 * 
 * @return char* 
 *         返回指向请求头字符串的指针，若未配置则返回NULL。
 */
char *c_mmi_get_wss_header(void);

enum {
    C_MMI_EVENT_USER_CONFIG,    // 用户对于sdk的配置应该在该事件回调中实现，如音频缓冲区大小、工作模式、音色等
    C_MMI_EVENT_DATA_INIT,	    // 当SDK完成初始化后触发该事件，可在该事件回调中开始建立业务连接
    C_MMI_EVENT_SPEECH_START,   // 当SDK开始进行音频上行时触发此事件
    C_MMI_EVENT_DATA_DEINIT,	// 当SDK注销后触发此事件

    C_MMI_EVENT_CAMERA_OPEN,        // 此事件在SDK需要使用摄像头时触发，建议在该事件回调中打开摄像头
    C_MMI_EVENT_CAMERA_CLOSE,       // 此事件在SDK不再需要使用摄像头时触发，建议在该事件回调中关闭摄像头并释放其所有资源
    C_MMI_EVENT_CAMERA_CAPTURE,     // 此事件在SDK判断需要采集图像时触发，建议在接收到该事件后拍一张照片并传入SDK
    C_MMI_EVENT_CAMERA_VIDEO_START, // 此事件在SDK判断需要开始录像时触发，建议在接收到该事件后开始录像并将数据传入SDK，录像FPS需要大于等于初始化设置的FPS
    C_MMI_EVENT_CAMERA_VIDEO_STOP,  // 此事件在SDK判断需要结束录像时触发，建议在接收到该事件后结束摄像头录像


    C_MMI_EVENT_ASR_START,	    // 当ASR开始返回数据时触发此事件
    C_MMI_EVENT_ASR_INCOMPLETE,	// 此事件返回尚未完成ASR的文本数据（全量）
    C_MMI_EVENT_ASR_COMPLETE,	// 此事件返回完成ASR的全部文本数据（全量）
    C_MMI_EVENT_ASR_END,		// 当ASR结束时触发此事件

    C_MMI_EVENT_LLM_INCOMPLETE,	// 此事件返回尚未处理完成的LLM文本数据（全量）
    C_MMI_EVENT_LLM_COMPLETE,	// 此事件返回处理完成的LLM全部文本数据（全量）

    C_MMI_EVENT_TTS_START,	    // 当开始音频下行时触发此事件
    C_MMI_EVENT_TTS_END,	    // 当音频完成下行时触发此事件

    C_MMI_EVENT_CMD_GENERAL,	// 当触发通用指令时触发此事件，暂未对外，有需要使用联系阿里云
    C_MMI_EVENT_CMD_USER,	    // 当触发用户自定义指令时触发此事件
};

enum {
    C_MMI_MODE_NONE,
    C_MMI_MODE_PUSH2TALK,
    C_MMI_MODE_TAP2TALK,
    C_MMI_MODE_DUPLEX,
    C_MMI_MODE_MAX
};

enum {
    C_MMI_TEXT_MODE_NONE,
    C_MMI_TEXT_MODE_ASR_ONLY,
    C_MMI_TEXT_MODE_LLM_ONLY,
    C_MMI_TEXT_MODE_BOTH
};

enum {
    C_MMI_STREAM_MODE_PCM,
    C_MMI_STREAM_MODE_OPUS_OGG,
    C_MMI_STREAM_MODE_MP3,
    C_MMI_STREAM_MODE_MAX,
};

typedef enum {
    FRAMESIZE_96x96,
    FRAMESIZE_160x120,
    FRAMESIZE_128x128,
    FRAMESIZE_176x144,
    FRAMESIZE_240x176,
    FRAMESIZE_240x240,
    FRAMESIZE_320x240,
    FRAMESIZE_320x320,
    FRAMESIZE_400x296,
    FRAMESIZE_480x320,
    FRAMESIZE_640x480,
    FRAMESIZE_800x600,
    FRAMESIZE_1024x768,
    FRAMESIZE_1280x720,
    FRAMESIZE_1280x1024,
    FRAMESIZE_1600x1200,
    FRAMESIZE_1920x1080,
    FRAMESIZE_720x1280,
    FRAMESIZE_864x1536,
    FRAMESIZE_2560x1440,
    FRAMESIZE_2560x1600,
    FRAMESIZE_1080x1920,
    FRAMESIZE_2560x1920,
    FRAMESIZE_2592x1944,
    FRAMESIZE_INVALID,
} frame_size_t;

typedef enum {
    CAPTURE_FORMAT_JPEG,
    CAPTURE_FORMAT_BMP,
    CAPTURE_FORMAT_PNG,
    CAPTURE_FORMAT_TIF,
    CAPTURE_FORMAT_WEBP,
    CAPTURE_FORMAT_HEIC,
    CAPTURE_FORMAT_JPG = CAPTURE_FORMAT_JPEG,
    CAPTURE_FORMAT_JPE = CAPTURE_FORMAT_JPEG,
    CAPTURE_FORMAT_TIFF = CAPTURE_FORMAT_TIF
} capture_format_t;

typedef enum {
    VIDEO_FORMAT_JPEG,
    VIDEO_FORMAT_BMP,
    VIDEO_FORMAT_PNG,
    VIDEO_FORMAT_TIF,
    VIDEO_FORMAT_WEBP,
    VIDEO_FORMAT_HEIC,
    VIDEO_FORMAT_JPG = VIDEO_FORMAT_JPEG,
    VIDEO_FORMAT_JPE = VIDEO_FORMAT_JPEG,
    VIDEO_FORMAT_TIFF = VIDEO_FORMAT_TIF
} video_format_t;

typedef enum{
    CAMERA_MODE_NONE,
    CAMERA_MODE_NORMAL,
    CAMERA_MODE_FAST,
    CAMERA_MODE_VIDEO,
} camera_mode_t;

typedef enum{
    CAMERA_DATA_BASE64,
    CAMERA_DATA_URL,
} camera_data_type_t;


/**
 * @brief 设置相机模式下的参数
 *
 * @param frame_size 	传入图片的分辨率大小
 * @param capture_format    传入图片格式
 * @param camera_mode	
 *		  相机工作的模式，分为标准模式、极速模式、视频模式以及闲置模式，
 *		  标准模式兼顾功耗和响应速度；极速模式牺牲部分功耗获得更好的响应时间；
 * 		  视频模式将会调用相应的视频开启和关闭事件获取图片数据，
 *		  图片数据将会在视频开启事件后每隔1/fps秒获取一张图片直到视频关闭事件触发；
 *        闲置模式下，相机将不会工作，与相机相关的所有参数和接口都不会生效或被调用。
 *		  在相机接口设置中，视频模式将不会被响应；
 *		  同理，在视频接口设置中，只有视频模式和闲置模式会被响应。
 * @param data_type	
 *		  传入图片数据的形式，分为base64和url形式，需要在初始化时指定。
 * @param timeout_ms
 *        设置相机在结束像机数据采集后最长闲置时间，当闲置时间超过设置值时将会触发C_MMI_EVENT_CAMERA_CLOSE事件，
 *        闲置时间单位为ms。
 */
typedef struct{
    frame_size_t frame_size;
    capture_format_t capture_format;
    camera_mode_t camera_mode;
    camera_data_type_t data_type;
    uint32_t timeout_ms;
} camera_capture_config_t;

/**
 * @brief 设置视频模式下的参数
 *
 * @param frame_size 	  传入视频的分辨率大小
 * @param video_format    传入视频格式，目前只支持图片形式的视频流
 * @param camera_mode	
 *		  相机工作的模式，分为标准模式、极速模式、视频模式以及闲置模式，
 *		  标准模式兼顾功耗和响应速度；极速模式牺牲部分功耗获得更好的响应时间；
 * 		  视频模式将会调用相应的视频开启和关闭事件获取图片数据，
 *		  图片数据将会在视频开启事件后每隔1/fps秒获取一张图片直到视频关闭事件触发；
 *        闲置模式下，相机将不会工作，与相机相关的所有参数和接口都不会生效或被调用。
 *		  在相机接口设置中，视频模式将不会被响应；
 *		  同理，在视频接口设置中，只有视频模式和闲置模式会被响应。
 * @param data_type	
 *		  传入图片数据的形式，分为base64和url形式，需要在初始化时指定。
 * @param fps
 *		  当相机开始录像工作后，SDK将以每1/fps秒一张图片的速度从外界获取图片并上传至云端。
 * @param timeout_ms 
 *        设置相机在结束像机数据采集后最长闲置时间，当闲置时间超过设置值时将会触发C_MMI_EVENT_CAMERA_CLOSE事件，
 *        闲置时间单位为ms。
 */
typedef struct {
    frame_size_t frame_size;
    video_format_t video_format;
    camera_mode_t camera_mode;
    camera_data_type_t data_type;
    uint32_t fps;
    uint32_t timeout_ms;
} camera_video_config_t;


/**
 * @brief 向图片缓冲区写入URL数据，
 *		  URL需要加入鉴权相关信息保证阿里云可以通过链接访问到照片
 *
 * @param url  待写入的数据指针
 * @param url_size 每次调用时写入的URL字符串数据长度（字节）
 * @return uint32_t 实际写入的字节数
 */
uint32_t c_mmi_put_camera_data_url(uint8_t* url, uint32_t url_size); 

/**
 * @brief 向图片缓冲区写入base64编码的图片数据，支持分片输入
 *		  
 * @param data 待写入的数据指针
 * @param size 每次调用时写入的base64编码图片数据长度（字节）
 * @param finish 当前照片数据是否传输完成，0代表当前数据没有传入完成，
 * 				 1代表当前1帧完整图片已经传输完成
 * @return uint32_t 实际写入的字节数
 */
uint32_t c_mmi_put_camera_data_base64(uint8_t* data, uint32_t size, uint8_t finish); 

/**
 * @brief 设置相机模式下的参数
 *
 * @param camera_capture_config_t 	
 *      相机模式设置参数，此参数以最后一次设置为准，并且此参数只有在新一轮对话开始时生效
 * @return 0代表成功，非0代表失败
 */
int32_t c_mmi_set_capture_param(camera_capture_config_t camera_capture_config);

/**
 * @brief 设置视频模式下的参数
 *
 * @param camera_video_config_t 	
 *      视频模式设置参数，此参数以最后一次设置为准，并且此参数只有在新一轮对话开始时生效
 * @return 0代表成功，非0代表失败
 */
int32_t c_mmi_set_video_param(camera_video_config_t camera_video_config);

/**
 * @brief 设置相机数据的环形缓冲区
 *
 * 配置相机放置相机数据的环形缓冲区
 * 
 * @param camera_ringbuffer_size   相机数据缓冲区大小，单位字节
 * @return int32_t 成功返回0，非0表示失败。
 */
int32_t c_mmi_set_camera_data_ringbuffer(uint32_t camera_ringbuffer_size);

/**
 * @brief 停止录像
 *
 * 当相机在录像时可调用，此接口可以通知SDK结束录像，调用此接口后，SDK会迅速调用C_MMI_EVENT_CAMERA_VIDEO_STOP事件，结束录像。
 * 
 * @return int32_t 成功返回0，非0表示失败。
 */
int32_t c_mmi_camera_video_stop(void);

/********************************************************************************/

typedef int32_t(*c_mmi_event_callback)(uint32_t event, void *param);

/**
 * @brief mmi事件回调函数类型
 *
 * 当mmi模块发生状态变化或事件时触发的回调函数
 * 
 * @param event 事件类型，取值为C_MMI_EVENT_xxx系列宏定义
 * @param param 事件参数，根据事件类型不同指向不同数据结构
 * @return int32_t 返回0表示处理成功，非0表示处理失败
 */
typedef int32_t(*C_MMI_EVENT_callback)(uint32_t event, void *param);

/**
 * @brief 注册mmi事件回调函数
 *
 * 必须在初始化前注册回调函数，否则可能返回错误
 * 
 * @param cb 需要注册的回调函数指针，传入NULL将注销回调
 * @return int32_t 成功返回0，非0表示失败。
 */
int32_t c_mmi_register_event_callback(C_MMI_EVENT_callback cb);

/**
 * @brief 设置环形缓冲区音频模式
 *
 * 配置录音和播放使用独立的环形缓冲区，缓冲区大小建议不小于16k
 * 
 * @param recorder_rb_size 录音缓冲区大小，单位字节
 * @param player_rb_size   播放缓冲区大小，单位字节
 * @return int32_t 成功返回0，非0表示失败。
 */
int32_t c_mmi_set_audio_mode_ringbuffer(uint32_t recorder_rb_size, uint32_t player_rb_size);

/**
 * @brief 向录音缓冲区写入数据
 *
 * 当音频模式为C_mmi_AUDIO_MODE_RINGBUFFER时使用
 * 
 * @param data 待写入的数据指针
 * @param size 数据长度（字节）
 * @return uint32_t 实际写入的字节数
 */
uint32_t c_mmi_put_recorder_data(uint8_t *data, uint32_t size);

/**
 * @brief 从播放缓冲区读取数据
 *
 * 当音频模式为C_mmi_AUDIO_MODE_RINGBUFFER时使用
 * 
 * @param data 用于存储读取数据的缓冲区
 * @param size 可用缓冲区大小（字节）
 * @return uint32_t 实际读取的字节数
 */
uint32_t c_mmi_get_player_data(uint8_t *data, uint32_t size);

int32_t c_mmi_set_upstream_mode(uint8_t stream_mode);
int32_t c_mmi_set_downstream_mode(uint8_t stream_mode);

/**
 * @brief 设置工作模式
 *
 * 配置mmi的工作模式，不同模式对应不同交互方式
 * 
 * @param work_mode 工作模式，可选值：
 *                  - C_MMI_MODE_PUSH2TALK (按住说话)
 *                  - C_MMI_MODE_TAP2TALK (点击说话)
 *                  - C_MMI_MODE_DUPLEX (双工模式)
 * @return int32_t 成功返回0，非0表示失败。
 */
int32_t c_mmi_set_work_mode(uint8_t work_mode);

/**
 * @brief 设置文本处理模式
 *
 * 配置ASR和LLM的文本处理模式，控制语音转文字和文字处理的行为
 * 
 * @param text_mode 文本模式，可选值：
 *                  - C_mmi_TEXT_MODE_NONE (禁用文本)
 *                  - C_mmi_TEXT_MODE_ASR_ONLY (仅ASR)
 *                  - C_mmi_TEXT_MODE_LLM_ONLY (仅LLM)
 *                  - C_mmi_TEXT_MODE_BOTH (两者都启用)
 * @return int32_t 成功返回0，非0表示失败。
 */
int32_t c_mmi_set_text_mode(uint8_t text_mode);

/**
 * @brief 设置语音ID
 *
 * 配置使用的语音合成角色ID，最大长度受C_mmi_VOICE_ID_LEN限制
 * 
 * @param voice_id 指向语音ID字符串的指针
 * @return int32_t 成功返回0，非0表示失败。
 */
int32_t c_mmi_set_voice_id(char *voice_id);

int32_t c_mmi_set_ip(char *ip);
int32_t c_mmi_set_postion(double longitude, double latitude);
int32_t c_mmi_set_city(char *city);

/**
 * 获取需要通过websocket发送的数据
 * 
 * 本函数用于根据指定的类型获取数据，准备通过websocket进行发送
 * 它会根据传入的类型参数，将相应类型的数据填充到提供的数据缓冲区中
 * 
 * @param type 用于返回websocket数据类型，如：WS_DATA_TYPE_TEXT、WS_DATA_TYPE_BINARY
 * @param data 指向一个uint8_t数组的指针，该数组用于存储获取的数据
 * @param size 表示数据数组的最大容量，以字节为单位
 * 
 * @return 返回实际填充到数据数组中的字节数
 */
uint32_t c_mmi_get_send_data(uint8_t *type, uint8_t *data, uint32_t size);

/**
 * 分析接收到的websocket数据函数
 * 
 * 此函数根据提供的数据类型和数据内容，分析接收到的数据包
 * 它的主要作用是解析数据内容，以便进一步处理或使用
 * 
 * @param type websocket数据类型，如：WS_DATA_TYPE_TEXT、WS_DATA_TYPE_BINARY
 * @param data 指向接收到的数据的指针，数据的内容将根据type参数进行解析
 * @param size 数据的长度，以字节为单位，用于确定数据的范围
 * 
 * @return 返回实际解析的字节数
 */
uint32_t c_mmi_analyze_recv_data(uint8_t type, uint8_t *data, uint32_t size);

/**
 * 启动语音识别功能。
 * 
 * 本函数用于初始化并启动语音识别模块，使系统能够接收和处理用户的语音输入。
 * 在需要开始语音交互时调用此函数以激活语音识别引擎。
 * 
 * @return 成功启动语音识别时返回0，否则返回非零的错误码。
 */
int32_t c_mmi_start_speech(void);

/**
 * 停止语音识别功能。
 * 
 * 本函数用于停止已启动的语音识别模块，终止对用户语音输入的监听和处理。
 * 在不需要继续进行语音交互时调用此函数，以释放资源并避免不必要的语音输入。
 * 
 * @return 成功停止语音识别时返回0，否则返回非零的错误码。
 */
int32_t c_mmi_stop_speech(void);

int32_t c_mmi_pause_task(void);

int32_t c_mmi_data_deinit(void);

int32_t aliyun_sdk_test(void);

#ifdef __cplusplus
}
#endif

#endif
