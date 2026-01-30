/*
 * Author: Zhu Jian bo
 */
#ifndef AIS_LITE_ENCRYPT_MAIN_H_
#define AIS_LITE_ENCRYPT_MAIN_H_

#include "ais_lite_encrypt_api_attr.h"
#include "ais_lite_encrypt_data_struct.h"



#ifdef __cplusplus
extern "C" {
#endif


//#define AIS_LITE_ENCRYPT_EXPORT __attribute__((visibility("default")))

/*
 * @Description: 授权模块入口函数
 * 
 * @Input params: deviceInfo : 用户信息
 *                libAiCodeType : 需要授权的引擎id
 * 
 * @Output params: 无
 * 
 * @Return: 返回值为0表示成功，其他值表示失败
 * 
 * @note: 
 */
AIS_LITE_ENCRYPT_EXPORT
int ais_lite_encrypt_create(DeviceInfo* deviceInfo, const char* libAiCodeType);


/*
 * @Description: 授权模块出口函数
 * 
 * @Input params: deviceInfo : 用户信息
 *                libAiCodeType : 需要授权的引擎id
 * 
 * @Output params: 无
 * 
 * @Return: 返回值为0表示成功，其他值表示失败
 * 
 * @note: 
 */
AIS_LITE_ENCRYPT_EXPORT
int ais_lite_encrypt_release(DeviceInfo* deviceInfo, const char* libAiCodeType);







/*
 * @Description: 计算需要设置到引擎内部的加密验证信息
 *               使用socket http方式进行post
 * @Input params: output_len: output缓冲区大小(单位:字节)
 * @Output params: output:用于计算的字符串.长度 需要大于或等于 50 个字节,并清0
 * @Return: 返回值为0表示成功，其他值表示失败
 */
AIS_LITE_ENCRYPT_EXPORT
int ais_lite_self_encrypt(char* output, int output_len);




#ifdef __cplusplus
}
#endif

#endif  // AIS_LITE_ENCRYPT_MAIN_H_