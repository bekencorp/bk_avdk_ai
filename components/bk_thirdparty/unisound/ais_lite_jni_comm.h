
/*
 * Author: Zhao Xing Hua
 */


#ifndef AIS_LITE_JNI_COMM_H_
#define AIS_LITE_JNI_COMM_H_

#include <stdint.h>
#include <stddef.h>
#include "ais_lite_encrypt_data_struct.h"
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32) || defined(_WIN64)
    #define _WINDOWS
#endif

#ifndef _WINDOWS
    #ifdef LIB_UAL_JNI_COMM_API_HIDDEN
        #define LIB_UAL_JNI_COMM_API_EXPORT __attribute__((visibility("hidden")))
    #else
        #define LIB_UAL_JNI_COMM_API_EXPORT __attribute__((visibility("default")))
    #endif

#else
    #ifdef DLL_EXPORT
        #define LIB_UAL_JNI_COMM_API_EXPORT __declspec(dllexport)
    #else
        #define LIB_UAL_JNI_COMM_API_EXPORT __declspec(dllimport)
    #endif
#endif


/*
 * @Description: JNI层对用户传进来的DeviceInfo进行单独资源申请组装并返回
 *               
 * @Input params: 
 * @Output params: 
 * @Return: 返回值不会NULL表示成功，否则表示失败
 */
LIB_UAL_JNI_COMM_API_EXPORT
DeviceInfo* ais_lite_jni_deviceInfo_integration(const char* DappKey, const char* DappSecret, const char* Dunique_id, const char* Dimei, const char* Dmac, const char* Dremark);


/*
 * @Description: JNI层对用户传进来的DeviceInfo进行资源释放
 *               
 * @Input params: 
 * @Output params: 
 * @Return: 返回值不会NULL表示成功，否则表示失败
 */
LIB_UAL_JNI_COMM_API_EXPORT
int ais_lite_jni_deviceInfo_free(DeviceInfo* info);



#ifdef __cplusplus
extern "C" {
#endif



#ifdef __cplusplus
}
#endif

#endif  // AIS_LITE_JNI_COMM_H_