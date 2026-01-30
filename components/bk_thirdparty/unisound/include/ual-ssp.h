/*
 * Copyright 2020 Unisound AI Technology Co., Ltd.
 * Author: Hao Peng
 * All Rights Reserved.
 */

#ifndef INCLUDE_UAL_SSP_H_
#define INCLUDE_UAL_SSP_H_

#include <stdint.h>
#include "ual-ssp-mode.h"

#if defined(_WIN32) || defined(_WIN64)
#define _WINDOWS
#endif

#ifndef _WINDOWS
#define SSP_EXPORT __attribute__((visibility("hidden")))
#else
#ifdef DLL_EXPORT
#define SSP_EXPORT __declspec(dllexport)
#else
#define SSP_EXPORT __declspec(dllimport)
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 * @Description: 计算对应工作模式所需要的内存
 * @Input params:
 *                 mode：参考 ual-ssp-mode.h
 * @Output params: size：单位 bytes
 * @Return: 成功：0
 *          失败：错误码
 */
SSP_EXPORT int32_t UalSspGetWorkspaceSize(SspMode mode, int32_t* size);

/*
 * @Description: SSP 引擎初始化
 * @Input params:
 *                memory_addr：外部传入的内存地址
 *                memory_size：内存大小
 * @Output params:
 * @Return: 成功：0
 *          失败：错误码
 */
SSP_EXPORT int32_t UalSspInit(SspMode mode, void* memory_addr,
                              int32_t memory_size);
/*
 * @Description: SSP 引擎重置
 * @Input params:
 *                mode：需要启用的工作模式
 *                memory_addr：外部内存的地址
 *                memory_size：外部内存大小
 * @Output params:
 * @Return: 成功：0
 *          失败：错误码
 */
SSP_EXPORT int32_t UalSspReset(SspMode mode, void* memory_addr,
                               int32_t memory_size);
/*
 * @Description: 处理当前数据
 * @Input params:
 *                mic: 麦克数据
 *                ref: 参考通道数据
 *                samples: mic&ref 数据的采样点数 (* sizeof(int16_t) 为字节数）
 * @Output params: out: 输出降噪后的数据
 *                 out_samples: 输出的采样点数
 *                 out_size：输出数据的字节数
 * @Return: 成功：0
 *          失败：错误码
 */
SSP_EXPORT int32_t UalSspProcess(int16_t* mic, int16_t* ref, int16_t samples,
                                 int16_t** out, int32_t* out_samples,
                                 int32_t* out_size);

/*
 * @Description: Set 属性
 * @Input params:
 *                id：参考 ual-ssp-id.h
 *                args：属性参数，类型参考 ual-ssp-args.h
 * @Output params: 无
 * @Return: 成功：0
 *          失败：错误码
 */
SSP_EXPORT int32_t UalSspSet(int32_t id, void* args);

/*
 * @Description: Get 属性
 * @Input params:
 *                id：参考 ual-ssp-id.h
 *                args：属性参数，类型参考 ual-ssp-args.h
 * @Output params: 无
 * @Return: 成功：0
 *          失败：错误码
 */
SSP_EXPORT int32_t UalSspGet(int32_t id, void* args);

/*
 * @Description: 释放init阶段申请的资源
 * @Input params: 无
 * @Output params: 无
 * @Return:
 */
SSP_EXPORT void UalSspRelease();

/*
 * @Description: 获取 Ssp 当前版本号
 * @Input params: 无
 * @Output params: 无
 * @Return: 版本号字符串
 */
SSP_EXPORT const char* UalSspVersion();

#ifdef __cplusplus
}
#endif

#endif  // INCLUDE_UAL_SSP_H_

