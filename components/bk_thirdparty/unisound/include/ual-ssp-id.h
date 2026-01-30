/*
 * Copyright 2020 Unisound AI Technology Co., Ltd.
 * Author: Hao Peng
 * All Rights Reserved.
 */

#ifndef INCLUDE_UAL_SSP_ID_H_
#define INCLUDE_UAL_SSP_ID_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* WARNING: 修改该文件后必须同步修改 SspId.java! */
/*
 * SSP type id 格式定义
 * 32          24         16                        0
 * |<--------->|<--------->|<---------------------->|
 *   MAIN_TYPE    SUB_TYPE        COMMAND  ID
 */

/* List of main type */
#define SSP_MAIN_ENGINE ((uint8_t)0x20)

/* List of sub type */
#define SSP_SUBTYPE_KIT ((uint8_t)0x00)
#define SSP_SUBTYPE_AEC ((uint8_t)0x01)
#define SSP_SUBTYPE_ENHANCE ((uint8_t)0x02)
#define SSP_SUBTYPE_BSS ((uint8_t)0x03)
#define SSP_SUBTYPE_ANC ((uint8_t)0x04)
#define SSP_SUBTYPE_DEREVERB ((uint8_t)0x05)
#define SSP_SUBTYPE_FFT ((uint8_t)0x06)
#define SSP_SUBTYPE_IFFT ((uint8_t)0x07)
#define SSP_SUBTYPE_BYPASS ((uint8_t)0x08)
#define SSP_SUBTYPE_MCLP ((uint8_t)0x09)
#define SSP_SUBTYPE_GSC ((uint8_t)0x0A)
#define SSP_SUBTYPE_NULL ((uint8_t)0xFF)

#define SSP_GET_MODULE_BY_ID(id) ((id)&0xFFFF0000)

#define SSP_ID_KIT \
  (((SSP_MAIN_ENGINE & 0xFF) << 24) | (SSP_SUBTYPE_KIT & 0xFF) << 16)
#define SSP_ID_KIT_GET_MIC_NUM (SSP_ID_KIT | (uint16_t)0x0001)
#define SSP_ID_KIT_GET_REF_NUM (SSP_ID_KIT | (uint16_t)0x0002)

/* ID of FFT */
#define SSP_ID_FFT \
  (((SSP_MAIN_ENGINE & 0xFF) << 24) | (SSP_SUBTYPE_FFT & 0xFF) << 16)
/* ID of IFFT */
#define SSP_ID_IFFT \
  (((SSP_MAIN_ENGINE & 0xFF) << 24) | (SSP_SUBTYPE_IFFT & 0xFF) << 16)
#define SSP_ID_IFFT_SET_DISABLE (SSP_ID_IFFT | (uint16_t)0x01)
/* ID of AEC */
#define SSP_ID_AEC \
  (((SSP_MAIN_ENGINE & 0xFF) << 24) | (SSP_SUBTYPE_AEC & 0xFF) << 16)
/* 设置 AEC 非使能，args 类型：int */
#define SSP_ID_AEC_SET_DISABLE (SSP_ID_AEC | (uint16_t)0x01)
/* 设置 AEC 工作模式，args 类型：int 0 为识别，1 为通话 */
#define SSP_ID_AEC_SET_MODE (SSP_ID_AEC | (uint16_t)0x02)
/* 获取当前 AEC 工作模式，0 为识别模式，1 为通话模式 */
#define SSP_ID_AEC_GET_MODE (SSP_ID_AEC | (uint16_t)0x03)

/* ID of ENHANCE */
#define SSP_ID_ENHANCE \
  (((SSP_MAIN_ENGINE & 0xFF) << 24) | (SSP_SUBTYPE_ENHANCE & 0xFF) << 16)

/* ID of BSS */
#define SSP_ID_BSS \
  (((SSP_MAIN_ENGINE & 0xFF) << 24) | (SSP_SUBTYPE_BSS & 0xFF) << 16)
/* 设置 BSS 工作模式 ，args 类型：int */
#define SSP_ID_BSS_SET_MODE (SSP_ID_BSS | (uint16_t)0x0001)
/* 设置 BSS 唤醒时间区间，args 类型：ActivityTimeInfo */
#define SSP_ID_BSS_SET_ACTIVITY_TIME (SSP_ID_BSS | (uint16_t)0x0002)

/* ID of GSC */
#define SSP_ID_GSC \
  (((SSP_MAIN_ENGINE & 0xFF) << 24) | (SSP_SUBTYPE_GSC & 0xFF) << 16)
/* 设置 Mic 坐标，args 类型：float* 排列格式为 x1, y1, x2, y2 */
#define SSP_ID_GSC_SET_POSITIONS (SSP_ID_GSC | (uint16_t)0x0001)
/* 设置定向角度，args 类型：float*, 取值范围 0-360 度 */
#define SSP_ID_GSC_SET_DIRECTIONS (SSP_ID_GSC | (uint16_t)0x0002)
/* 获取 VAD 信息，args 类型：int, 0 为无语音，1 为有语音 */
#define SSP_ID_GSC_GET_VAD_STATUS (SSP_ID_GSC | (uint16_t)0x0003)

/* ID of MCLP */
#define SSP_ID_MCLP \
  (((SSP_MAIN_ENGINE & 0xFF) << 24) | (SSP_SUBTYPE_MCLP & 0xFF) << 16)

/* ID of ANC */
#define SSP_ID_ANC \
  (((SSP_MAIN_ENGINE & 0xFF) << 24) | (SSP_SUBTYPE_ANC & 0xFF) << 16)

/* ID of DEREVERB */
#define SSP_ID_DEREVERB \
  (((SSP_MAIN_ENGINE & 0xFF) << 24) | (SSP_SUBTYPE_DEREVERB & 0xFF) << 16)

/* ID of BYPASS */
#define SSP_ID_BYPASS \
  (((SSP_MAIN_ENGINE & 0xFF) << 24) | (SSP_SUBTYPE_BYPASS & 0xFF) << 16)

/* ID of NULL */
#define SSP_ID_NULL \
  (((SSP_MAIN_ENGINE & 0xFF) << 24) | (SSP_SUBTYPE_NULL & 0xFF) << 16)

#ifdef __cplusplus
}
#endif

#endif  // INCLUDE_UAL_SSP_ID_H_
