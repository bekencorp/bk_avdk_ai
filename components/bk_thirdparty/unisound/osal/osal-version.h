/*
 * Copyright 2019 Unisound AI Technology Co., Ltd.
 * Author: 
 * All rights reserved.
 */

#ifndef OSAL_OSAL_VERSION_H_
#define OSAL_OSAL_VERSION_H_

#if defined(_WIN32) || defined(_WIN64)
  #define _WINDOWS
#endif

#ifndef _WINDOWS
  #ifdef OSAL_API_HIDDEN
    #define OSAL_EXPORT __attribute__((visibility("hidden")))
  #else
    #define OSAL_EXPORT __attribute__((visibility("default")))
  #endif

#else
  #ifdef DLL_EXPORT
    #define OSAL_EXPORT __declspec(dllexport)
  #else
    #define OSAL_EXPORT __declspec(dllimport)
  #endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define OSAL_VERSION "OSAL-v4.15.0-0e8995d"

#define OSAL_SYSTEM_TYPE "OSAL-SYSTEM-bk"

/*
 * @Description: 获取版本号
 * @Input params:
 * @Output params:
 * @Return:
 */
OSAL_EXPORT const char* OsalGetVersion();

/*
 * @Description: 获取系统类型信息
 * @Input params:
 * @Output params:
 * @Return:
 */
OSAL_EXPORT const char* OsalGetSystemType();

#ifdef __cplusplus
}
#endif

#endif  // OSAL_OSAL_VERSION_H_
