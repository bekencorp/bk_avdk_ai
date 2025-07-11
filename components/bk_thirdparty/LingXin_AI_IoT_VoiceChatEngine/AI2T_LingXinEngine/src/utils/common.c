#include "lingxin_common.h"
#include "lingxin_websocket.h"
#include "lingxin_tls_utils.h"
#include <stdarg.h>
#include <sys/time.h>
#include <time.h>

#ifdef __ANDROID__
#include <android/log.h>
#include <jni.h>

#endif

bool enableLog = false;
void setLogEnable(bool enabled) { enableLog = enabled; }
bool isLogEnabled() { return enableLog; }

void logPrintf(const char *format, ...)
{
  if (!isLogEnabled())
  {
    return;
  }

#ifdef __ANDROID__
  va_list args;           // 声明一个 va_list 类型的变量
  va_start(args, format); // 初始化 args，使其指向可变参数列表的第一个参数

  // 构建格式化的字符串
  char buffer[1024];
  vsnprintf(buffer, sizeof(buffer), format, args);

  // 使用 __android_log_print 输出到 Logcat
  __android_log_print(ANDROID_LOG_DEBUG, "AI_IOT_SDK", "---%s---", buffer);
  va_end(args); // 结束可变参数列表的处理
#else
  // 获取当前时间
  time_t now = time(NULL);
  struct tm *tm_info = localtime(&now);
  char time_str[26]; // 用于存储时间字符串
  // 格式化时间字符串
  strftime(time_str, sizeof(time_str), "%Y/%m/%d %H:%M:%S", tm_info);
  va_list args;           // 声明一个 va_list 类型的变量
  va_start(args, format); // 初始化 args，使其指向可变参数列表的第一个参数
  // 使用 vprintf 函数，将可变参数列表传递给 logPrintf
  printf("%s : ", time_str);
  printf("--------");
  vprintf(format, args);
  printf("--------\n");
  va_end(args); // 结束可变参数列表的处理
#endif
}

// android 4.4不支持srand，故自己实现随机算法
// static uint32_t lcg_rand(uint32_t *seed)
// {
//   // LCG算法核心
//   *seed = (*seed) * 1103515245 + 12345;
//   return *seed;
// }

// char *generateUUID(int length)
// {
//   const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
//   int charset_len = sizeof(charset) - 1;

//   char *uuid = (char *)malloc(length + 1);

//   // 局部种子
//   uint32_t seed = (uint32_t)time(NULL);

//   for (int i = 0; i < length; ++i)
//   {
//     uuid[i] = charset[lcg_rand(&seed) % charset_len];
//   }
//   uuid[length] = '\0';
//   return uuid;
// }

char *generateUUID(int length)
{
  const char charset[] =
      "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
  int charset_len = sizeof(charset) - 1;

  char *uuid = (char *)malloc(length + 1);
  // 初始化随机数生成器
  srand((unsigned int)time(NULL));
  for (int i = 0; i < length; ++i)
  {
    uuid[i] = charset[rand() % charset_len];
  }
  uuid[length] = '\0'; // 确保字符串以 null 结尾
  return uuid;
}

char *snprintfWithMalloc(const char *format, ...)
{
  if (!format)
  {
    logPrintf("Error: Format string is NULL");
    return NULL;
  }

  va_list args;

  va_start(args, format);
  int size = vsnprintf(NULL, 0, format, args);
  va_end(args);

  if (size < 0)
  {
    logPrintf("Error: Failed to calculate message size.");
    return NULL;
  }
  char *json_message = malloc(size + 1); // +1 用于 '\0'
  if (!json_message)
  {
    logPrintf("Error: Failed to allocate memory");
    return NULL;
  }

  va_start(args, format);
  int written = vsnprintf(json_message, size + 1, format, args);
  va_end(args);

  if (written < 0 || written >= size + 1)
  {
    logPrintf("Error: Failed to construct JSON message.");
    free(json_message); // 释放内存
    return NULL;
  }

  return json_message;
}

static char *generateTimestampMS()
{
  // 这里不是真正的毫秒数，近似毫秒
  long long timestampMsLong = (long long)time(NULL) * 1000;
  char *timestamp = snprintfWithMalloc("%lld", timestampMsLong);
  return timestamp;
}

WebsocketConfig *createWebsocketConfig(void *handler, const char *sn,
                                       const char *appKey, const char *appId, const char *path,
                                       WebSocketEventListener listener)
{
  WebsocketConfig *config = (WebsocketConfig *)malloc(sizeof(WebsocketConfig));
  char *timestamp = generateTimestampMS();
  config->header_signature = generateSignature(sn, appKey, appId, timestamp);
  config->header_sn = strdup(sn);
  config->header_app_id = strdup(appId);
  config->header_timestamp = timestamp;
  config->protocol = PROTOCOL_WEBSOCKET;
  config->host = REQUEST_URL; // math-daily.edu-aliyun.com
  config->path = path;
  config->port = REQUEST_PORT;
  config->listener = listener;
  config->userContext = handler; // Pass the user data to the WebSocket event listener
  return config;
}

HttpConfig *createHttpConfig(const char *appId, const char *sn, const char *appKey,
                             const char *path, const char *reqBody, HttpHeader *headers)
{
  HttpConfig *config = (HttpConfig *)calloc(1, sizeof(HttpConfig));
  if (!config)
  {
    logPrintf("Failed to allocate memory for HttpConfig");
    return NULL;
  }
  char *timestamp = generateTimestampMS();
  headers->signature = generateSignature(sn, appKey, appId, timestamp);
  headers->timestamp = timestamp;
  headers->app_id = appId;
  headers->sn = sn;
  config->headers = headers;
  config->host = REQUEST_URL;
  config->path = path;
  config->port = REQUEST_PORT;
  config->protocol = PROTOCOL_HTTP;
  config->post_data = reqBody;
  return config;
}