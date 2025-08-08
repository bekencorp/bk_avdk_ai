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

#ifndef __LIB_C_STORAGE_H__
#define __LIB_C_STORAGE_H__

#ifdef __cplusplus
extern "C"{
#endif

#include "c_utils.h"

#define C_APP_ID_STR_LEN        (32)
#define C_DN_STR_LEN_MAX        (32)

#define C_NONCE_LEN             (13)
#define C_APP_SECRET_LEN        (16)
#define C_DEVICE_SECRET_LEN     (16)

#define C_DIALOG_ID_LEN         (36)

/**
 * 检查设备是否已注册
 * 
 * @return 返回设备注册状态，1表示已注册，0表示未注册或发生错误
 * 
 * 此函数用于检查设备是否已完成注册流程
 */
uint8_t c_storage_device_is_registered(void);

/**
 * 保存配置
 * 
 * @return 返回操作结果，非零表示失败
 * 
 * 此函数用于保存之前设置的配置信息只有当调用此函数后，所设置的配置信息才会被实际写入存储
 */
int32_t c_storage_save(void);

/**
 * 重置配置
 * 
 * 此函数用于清除所有已保存的设备配置信息
 * 调用此函数后，配置将恢复为默认状态，需要重新设置相关参数
 * 
 * @return 返回操作结果，非零表示失败
 */
int32_t c_storage_reset(void);

/**
 * 设置应用ID
 * 
 * @param app_id_str 应用ID，由阿里云颁发，字符串格式
 * @return 返回操作结果，非零表示失败
 * 
 * 此函数用于设置应用ID，完成设置后需要调用c_storage_save进行保存
 */
int32_t c_storage_set_app_id_str(char *app_id_str);

/**
 * 设置应用密钥appSecret
 * 
 * @param app_secret_str 应用密钥，由阿里云颁发，字符串格式
 * @return 返回操作结果，非零表示失败
 * 
 * 此函数用于设置应用密钥，完成设置后需要调用c_storage_save进行保存
 */
int32_t c_storage_set_app_secret_str(char *app_secret_str);

/**
 * 设置设备名称DN
 * 
 * @param dn 设备名称DN，用户可自行设定，长度不超过32字符
 * @return 返回操作结果，非零表示失败
 * 
 * 此函数用于设置设备名称DN，完成设置后需要调用c_storage_save进行保存
 */
int32_t c_storage_set_device_name(char *device_name);


int32_t c_storage_get_app_id_str(char *app_id_str);
int32_t c_storage_get_app_secret(uint8_t *app_secret);
int32_t c_storage_get_device_name(char *deviceName);

#ifdef __cplusplus
}
#endif

#endif
