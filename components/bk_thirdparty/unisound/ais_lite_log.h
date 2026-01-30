/*
 * Author: Zhu Jian bo
 */

/*
 * 说明: log相关API的调试以及打印输出，
 * 采用LOG_MOD模块名，LOG_LEVEL_INFO的等级，
 * 
 * 如发现未打印请检查：
 * 1.模块配置
 * 2.等级配置
 * 3.AIS_LITE_LOG_DISABLED 编译选项是否定义
 * 
 * 增加新模块：
 * 1.在enum ais_lite_log_module 中添加新模块
 * 2.在g_ais_lite_log_module_info[LOG_MOD_MAX] 中添加使能标志以及打印字符串
 */

#ifndef AIS_LITE_LOG_H_
#define AIS_LITE_LOG_H_

#include <stdint.h>
#include <stddef.h>


#ifdef __cplusplus
extern "C" {
#endif

#if defined(__GNUC__)
    #define __FUNC__ ((const char*)(__PRETTY_FUNCTION__))
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 19901L
    #define __FUNC__ ((const char*)(__func__))
#else
    #define __FUNC__ ((const char*)(__FUNCTION__))
#endif


#if defined(_WIN32) || defined(_WIN64)
    #define _WINDOWS
#endif

#ifndef _WINDOWS
    #ifdef AIS_LITE_LOG_API_HIDDEN
        #define AIS_LITE_LOG_EXPORT __attribute__((visibility("hidden")))
    #else
        #define AIS_LITE_LOG_EXPORT __attribute__((visibility("default")))
    #endif

#else  // _WINDOWS
    #ifdef AIS_LITE_LOG_API_HIDDEN
        #define AIS_LITE_LOG_EXPORT __declspec(dllimport)
    #else  // AIS_LITE_LOG_API_HIDDEN
        #define AIS_LITE_LOG_EXPORT __declspec(dllexport)
    #endif
#endif


/* log 等级定义 */
typedef enum ais_lite_log_level {
  LOG_LEVEL_VERBOSE = 0,
  LOG_LEVEL_DEBUG,
  LOG_LEVEL_INFO,
  LOG_LEVEL_WARNING,
  LOG_LEVEL_ERROR,
  LOG_LEVEL_OFF, /* 6:log off */

  LOG_LEVEL_MAX
} ais_lite_log_level;

typedef struct ais_lite_log_level_info {
  // 当前优先级
  ais_lite_log_level cur_log_level;

  // 不同等级的打印字样
  const char* const level_string[LOG_LEVEL_MAX];
} ais_lite_log_level_info;

/*
 * @Description: 设置 Log 打印等级
 * @Input params: level：等级
 * @Output params: 无
 * @Return: 返回值为0表示成功，其他值表示失败
 */
AIS_LITE_LOG_EXPORT
int ais_lite_log_level_set(ais_lite_log_level level);

/*
 * @Description: 获取 Log 打印等级
 * @Input params: 无
 * @Output params: level：等级
 * @Return: 返回值为0表示成功，其他值表示失败
 */
AIS_LITE_LOG_EXPORT
int ais_lite_log_level_get(ais_lite_log_level* level);

/************************************************************************/

/* log 模块定义 */
typedef enum ais_lite_log_module {
  LOG_MOD_SELF = 0,  // 用于log模块自身的打印
  LOG_MOD_ENCRYPT,
  LOG_MOD_DIC,
  LOG_MOD_KWS,
  LOG_MOD_TTS,
  LOG_MOD_DSP2D,
  LOG_MOD_DSP2D_LITE,
  LOG_MOD_SSPCAR,
  LOG_MOD_SSPLINEAR,
  LOG_MOD_SSPHYBIRD,
  LOG_MOD_SSPTXZ,
  LOG_MOD_VADNN,
  LOG_MOD_SED,
  LOG_MOD_JNI,
  LOG_MOD_SENSITIVE_INFO,
  LOG_MOD_APP_SDK, //当上层应用使用aislite log时填写的模块名
  LOG_MOD_VPR,
  LOG_MOD_NLU,
  LOG_MOD_SPL,

  LOG_MOD_MAX

} ais_lite_log_module;

typedef struct ais_lite_log_module_info {
  // 当前模块输出是否使能
  int cur_module_enable;

  // 当前模块的打印字样
  const char* const module_string;
} ais_lite_log_module_info;

/*
 * @Description: 设置 Log 打印需要输出的某个模块
 * @Input params: module：模块名
 * @Output params: 无
 * @Return: 返回值为0表示成功，其他值表示失败
 */
AIS_LITE_LOG_EXPORT
int ais_lite_log_module_enable(ais_lite_log_module module);

/*
 * @Description: 设置 Log 打印关闭输出的某个模块
 * @Input params: module：模块名
 * @Output params: 无
 * @Return: 返回值为0表示成功，其他值表示失败
 */
AIS_LITE_LOG_EXPORT
int ais_lite_log_module_disenable(ais_lite_log_module module);

/*
 * @Description: 打印出 当前Log 打印会输出哪些模块的信息
 * @Input params: 无
 * @Output params: 无
 * @Return: 返回值为0表示成功，其他值表示失败
 * @note: 采用LOG_MOD模块名，LOG_LEVEL_INFO的等级进行打印
 */
AIS_LITE_LOG_EXPORT
int ais_lite_log_module_dump(void);



/************************************************************************/

/*
 * @Description: 合成一条完整 log, 并发送队列
 * @Input params: level：ComLogLevel
 *                module：模块名
 *                function：函数名
 *                line：行号
 *                format：标准格式
 * @Output params:
 * @Return:
 */
AIS_LITE_LOG_EXPORT
void ais_lite_log_dump(ais_lite_log_level level,
                       ais_lite_log_module module,
                       const char* function,
                       uint32_t line,
                       const char* format, ...);

/*
 * 为了避免在 release 产品中 log 相关代码增加 binary
 * 的大小以及增加执行时间，通过预编译选项来控制除 LOG_ERROR 级别外的 log
 * 代码的编译。
 */
#ifndef AIS_LITE_LOG_DISABLED

/*
 * @Description: VERBOSE 等级日志
 * @Input params:
 * @Output params:
 * @Return:
 */
#define AIS_LITE_LOG_VERBOSE(module, format, args...)                          \
  do {                                                                     \
    ais_lite_log_dump(LOG_LEVEL_VERBOSE, module, __FUNC__, __LINE__, format, ##args); \
  } while (0)

/*
 * @Description: DEBUG 等级日志
 * @Input params:
 * @Output params:
 * @Return:
 */
#define AIS_LITE_LOG_DEBUG(module, format, args...)                          \
  do {                                                                   \
    ais_lite_log_dump(LOG_LEVEL_DEBUG, module, __FUNC__, __LINE__, format, ##args); \
  } while (0)

/*
 * @Description: INFO 等级日志
 * @Input params:
 * @Output params:
 * @Return:
 */
#define AIS_LITE_LOG_INFO(module, format, args...)                          \
  do {                                                                  \
    ais_lite_log_dump(LOG_LEVEL_INFO, module, __FUNC__, __LINE__, format, ##args); \
  } while (0)

/*
 * @Description: WARNING 等级日志
 * @Input params:
 * @Output params:
 * @Return:
 */
#define AIS_LITE_LOG_WARNING(module, format, args...)                          \
  do {                                                                     \
    ais_lite_log_dump(LOG_LEVEL_WARNING, module, __FUNC__, __LINE__, format, ##args); \
  } while (0)

#else  // AIS_LITE_LOG_DISABLED

#define AIS_LITE_LOG_VERBOSE(module, format, args...)
#define AIS_LITE_LOG_DEBUG(module, format, args...)
#define AIS_LITE_LOG_INFO(module, format, args...)
#define AIS_LITE_LOG_WARNING(module, format, args...)

#endif  // AIS_LITE_LOG_DISABLED

/*
 * @Description: ERROR 等级日志
 * @Input params:
 * @Output params:
 * @Return:
 */
#define AIS_LITE_LOG_ERROR(module, format, args...)                          \
  do {                                                                   \
    ais_lite_log_dump(LOG_LEVEL_ERROR, module, __FUNC__, __LINE__, format, ##args); \
  } while (0)



#ifdef __cplusplus
}
#endif

#endif  // AIS_LITE_LOG_H_
