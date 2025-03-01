#pragma once

#include "agora_rtc_api.h"


#define CONFIG_AGORA_APP_ID "xxxxxxxxxxxxxxxxxxxxxxxxxxx" // Please replace with your own APP ID

#define CONFIG_CUSTOMER_KEY "8620fd479140455388f99420fd307363"
#define CONFIG_CUSTOMER_SECRET "492c18dcdb0a43c5bb10cc1cd217e802"

#define CONFIG_LICENSE_PID "00F8D46F55D34580ADD8A4827F822646"

// Agora Master Server URL
#define CONFIG_MASTER_SERVER_URL "https://app.agoralink-iot-cn.sd-rtn.com"

// Agora Slave Server URL
#define CONFIG_SLAVE_SERVER_URL "https://api.agora.io/agoralink/cn/api"

// Found product key form device manager platform
#define CONFIG_PRODUCT_KEY "EJIJEIm68gl5b5lI4"


#define CONFIG_CHANNEL_NAME "7258_01"

#define CONFIG_AGORA_TOKEN "" // Please turn on token when the product is mass produced.

#define CONFIG_AGORA_UID        (150200)

// #define CONFIG_AUDIO_ONLY
// #define CONFIG_USE_G722_CODEC
//#define CONFIG_USE_G711U_CODEC


//#define CONFIG_UVC_CAMERA  /* config CONFIG_USB_UVC in cp1 */
#define CONFIG_DVP_CAMERA
/* dual stream feature */
// #define CONFIG_ENABLE_DUAL_STREAM

/* for test */
// #define CONFIG_ENABLE_APP_DATA_BACK



#define BANDWIDTH_ESTIMATE_MIN_BITRATE   (500000)
#define BANDWIDTH_ESTIMATE_MAX_BITRATE   (2000000)
#define BANDWIDTH_ESTIMATE_START_BITRATE (800000)

#if defined(CONFIG_USE_G711U_CODEC)   //G711U
#define CONFIG_AUDIO_CODEC_TYPE   		AUDIO_CODEC_TYPE_G711U
#define CONFIG_PCM_FRAME_LEN            320
#define CONFIG_PCM_SAMPLE_RATE          8000
#define CONFIG_PCM_CHANNEL_NUM          1
#define CONFIG_SEND_PCM_DATA
#elif defined(CONFIG_USE_G711A_CODEC) // G711A
#define CONFIG_AUDIO_CODEC_TYPE   		AUDIO_CODEC_TYPE_G711A
#define CONFIG_PCM_FRAME_LEN            320
#define CONFIG_PCM_SAMPLE_RATE          8000
#define CONFIG_PCM_CHANNEL_NUM          1
#define CONFIG_SEND_PCM_DATA
#elif defined(CONFIG_USE_G722_CODEC)  // G722
#define CONFIG_AUDIO_CODEC_TYPE   		AUDIO_CODEC_TYPE_G722
#define CONFIG_PCM_FRAME_LEN            640
#define CONFIG_PCM_SAMPLE_RATE          16000
#define CONFIG_PCM_CHANNEL_NUM          1
#define CONFIG_SEND_PCM_DATA
#else                                // DISABLE
#define CONFIG_AUDIO_CODEC_TYPE   		AUDIO_CODEC_DISABLED
#define CONFIG_PCM_FRAME_LEN            160
#define CONFIG_PCM_SAMPLE_RATE          8000
#define CONFIG_PCM_CHANNEL_NUM          1
// #define CONFIG_SEND_PCM_DATA
#endif
#define CONFIG_AUDIO_FRAME_DURATION_MS     20  // except OPUS
  // (CONFIG_PCM_FRAME_LEN * 1000 / CONFIG_PCM_SAMPLE_RATE / CONFIG_PCM_CHANNEL_NUM /sizeof(int16_t))

#define DEFAULT_SDK_LOG_PATH "io.agora.rtc_sdk"