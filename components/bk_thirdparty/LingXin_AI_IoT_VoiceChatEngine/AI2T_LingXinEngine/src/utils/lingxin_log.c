#include "lingxin_log.h"
#include <stdarg.h>
#include <stdio.h>
#include "lingxin_version.h"
#include <stdbool.h>
#include <string.h>
#include "lingxin_system_time.h"

#ifdef __ANDROID__
#include <android/log.h>
#include <jni.h>
#endif

// 添加beken平台头文件
#include "components/log.h"
// 日志级别定义，与beken平台保持一致
#define LINGXIN_LOG_LEVEL_NONE    0
#define LINGXIN_LOG_LEVEL_ERROR   1
#define LINGXIN_LOG_LEVEL_WARN    2
#define LINGXIN_LOG_LEVEL_INFO    3
#define LINGXIN_LOG_LEVEL_DEBUG   4

// 默认日志级别
#ifndef LINGXIN_LOG_LEVEL
#define LINGXIN_LOG_LEVEL LINGXIN_LOG_LEVEL_DEBUG
#endif
// 获取不带扩展名的文件名
static const char* get_module_name(const char* filepath) {
    static char module_name[64];
    // 提取文件名部分
    const char* filename = strrchr(filepath, '/');
    if (!filename) filename = strrchr(filepath, '\\');
    filename = filename ? filename + 1 : filepath;
    
    // 去掉扩展名
    const char* dot = strrchr(filename, '.');
    int len = dot ? (dot - filename) : strlen(filename);
    if (len >= sizeof(module_name)) len = sizeof(module_name) - 1;
    strncpy(module_name, filename, len);
    module_name[len] = '\0';
    
    return module_name;
}


static void _log_internal(const char *level, const char *moduleName, int line,  const char *format, va_list args)
{

#ifdef __ANDROID__
    char buffer[1024];
    int len = snprintf(buffer, sizeof(buffer), "[%s] [%s] [%s:%d]: %s", LINGXIN_VERSION, level, moduleName, line, format);

    if (len < sizeof(buffer))
    {
        // 缓冲区足够，直接输出
        __android_log_print(ANDROID_LOG_DEBUG, "LINGXIN", "%s", buffer);
    }
    else
    {
        // 缓冲区不够，做截断处理，为性能考虑，不能动态申请内存
        int prefix_len = snprintf(buffer, sizeof(buffer), "[%s] [%s] [%s:%d]: ", LINGXIN_VERSION, level, moduleName, line);
        if (prefix_len < sizeof(buffer))
        {
            vsnprintf(buffer + prefix_len, sizeof(buffer) - prefix_len, format, args);
        }
        __android_log_print(ANDROID_LOG_DEBUG, "LINGXIN", "%s", buffer);
    }
#else

    // 格式化时间字符串,[mm-dd hh:mm:ss.milliseconds]
    char time_str[32];

    LINGXIN_TIME lingxin_time = {0};
    get_current_time(&lingxin_time);
    snprintf(time_str, sizeof(time_str), "%02d-%02d %02d:%02d:%02d.%03ld",
             lingxin_time.mon, lingxin_time.day,
             lingxin_time.hour, lingxin_time.min, lingxin_time.sec, lingxin_time.mill_sec);
    // 输出格式：[time] [version] [level] [module:line]: log内容
#if 1
    // 使用beken平台的日志输出函数
    char log_buffer[512];
    int prefix_len = snprintf(log_buffer, sizeof(log_buffer), "[%s] [%s] [%s] [%s:%d]: ",
                             time_str, LINGXIN_VERSION, level, moduleName, line);
    if (prefix_len < sizeof(log_buffer)) {
        vsnprintf(log_buffer + prefix_len, sizeof(log_buffer) - prefix_len, format, args);
    }

    int bk_level = LINGXIN_LOG_LEVEL_NONE;
    if (strcmp(level, "E") == 0) {
        bk_level = LINGXIN_LOG_LEVEL_ERROR;
    } else if (strcmp(level, "W") == 0) {
        bk_level = LINGXIN_LOG_LEVEL_WARN;
    } else if (strcmp(level, "I") == 0) {
        bk_level = LINGXIN_LOG_LEVEL_DEBUG;
    } else if (strcmp(level, "D") == 0) {
        bk_level = LINGXIN_LOG_LEVEL_INFO;
    }
    if (bk_level <= LINGXIN_LOG_LEVEL) {
        bk_printf_ext(bk_level, (char*)moduleName, "%s\n", log_buffer);
    }
#else
    printf("[%s] [%s] [%s] [%s:%d]: ", time_str, LINGXIN_VERSION, level, moduleName, line);
    vprintf(format, args);
    printf("\n");
#endif
#endif
}

static char *level_int_to_char(int level)
{
    char *levelChar = "";
    switch (level)
    {
    case LINGXIN_INFO:
        levelChar = "I";
        break;
    case LINGXIN_DEBUG:
        levelChar = "D";
        break;
    case LINGXIN_WARN:
        levelChar = "W";
        break;
    case LINGXIN_ERROR:
        levelChar = "E";
        break;
    default:
        break;
    }
    return levelChar;
}

void _lingxin_log_print_internal_(int level, const char *filePath, int line, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    _log_internal(level_int_to_char(level), get_module_name(filePath), line, format, args);
    va_end(args);
}