/*
 * Copyright 2021 Unisound AI Technology Co., Ltd.
 * Author: Zhu Jian BO
 * All Rights Reserved.
 */

#ifndef AIS_LITE_VERSION_H_
#define AIS_LITE_VERSION_H_


#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32) || defined(_WIN64)
    #define _WINDOWS
#endif

#ifndef _WINDOWS
    #ifdef AIS_LITE_VERSION_API_HIDDEN
        #define AIS_LITE_VERSION_EXPORT __attribute__((visibility("hidden")))
    #else
        #define AIS_LITE_VERSION_EXPORT __attribute__((visibility("default")))
    #endif

#else  // _WINDOWS
    #ifdef DLL_EXPORT
        #define AIS_LITE_VERSION_EXPORT __declspec(dllexport)
    #else  // DLL_EXPORT
        #define AIS_LITE_VERSION_EXPORT __declspec(dllimport)
#endif
#endif


/*
 * @Description: 获取当前ais-lite版本号
 * @Input params:
 * @Output params:
 * @Return:
 */
AIS_LITE_VERSION_EXPORT
const char* ais_lite_get_version();


/*
 * @Description: 获取当前ais-lite git信息
 * @Input params:
 * @Output params:
 * @Return:
 */
AIS_LITE_VERSION_EXPORT
const char* ais_lite_get_git_info();


#ifdef __cplusplus
}
#endif

#endif  // AIS_LITE_VERSION_H_
