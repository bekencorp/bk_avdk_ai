/*
 * Copyright 2020 Unisound AI Technology Co., Ltd.
 * Author: Hao Peng
 * All Rights Reserved.
 */

#ifndef INCLUDE_UAL_SSP_ARGS_H_
#define INCLUDE_UAL_SSP_ARGS_H_

#ifdef __cplusplus
extern "C" {
#endif

/* SSP ID: SSP_ID_AEC_SET_MODE */
#define SSP_AEC_MODE_ASR 0
#define SSP_AEC_MODE_PHONE_CALL 1

/* SSP ID: SSP_ID_BSS_SET_ACTIVITY_TIME */
typedef struct ActivityTimeInfo {
  float start_sec;  // 取自kws
  float end_sec;    // 取自kws
} ActivityTimeInfo;

#ifdef __cplusplus
}
#endif

#endif  // INCLUDE_UAL_SSP_ARGS_H_
