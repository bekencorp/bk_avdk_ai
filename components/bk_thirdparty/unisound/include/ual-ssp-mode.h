/*
 * Copyright 2019 Unisound AI Technology Co., Ltd.
 * Author: Hao Peng
 * All Rights Reserved.
 */

#ifndef INCLUDE_UAL_SSP_MODE_H_
#define INCLUDE_UAL_SSP_MODE_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int32_t SspMode;

#define UAL_SSP_MODE_NONE 0         /* 无 */
#define UAL_SSP_MODE_AEC_ONLY 1     /* 仅 AEC */
#define UAL_SSP_MODE_ENHANCE_ONLY 2 /* 单麦降噪 */
#define UAL_SSP_MODE_AEC_ENHANCE 3  /* 单麦降噪 + AEC */
#define UAL_SSP_MODE_FBF 4          /* 定向拾音 */
#define UAL_SSP_MODE_OMN 5          /* 全向降噪 */
#define UAL_SSP_MODE_DLE 6          /* 通话降噪 */
#define UAL_SSP_MODE_BYPASS 7       /* 透传模式 */
#define UAL_SSP_MODE_LOWPOWER 8 /* 低功耗模式: AEC + 去混响 + 双麦降噪 */
#define UAL_SSP_MODE_LP_NO_AEC 9 /* 低功耗模式: 去混响 + 双麦降噪 */
#define UAL_SSP_MODE_FENHANCE 10 /* 单麦降噪，频域输出 */
#define UAL_SSP_MODE_FBF_2MIC 11 /* 双麦定向降噪模式，当前适用智能穿戴 */
#define UAL_SSP_MODE_AEC_FBF_2MIC 12 /* AEC 模式 + 双麦定向降噪*/
#define UAL_SSP_MODE_DLNR 13         /* 单通道 DL 降噪模式 */
#define UAL_SSP_MODE_AEC_MCLP_ENHANCE 14 /*全向降噪: AEC + 去混响 + 单通道平稳噪声抑制*/
#define UAL_SSP_MODE_AEC_DLNR 15
#define UAL_SSP_MODE_MCLP 24
#define UAL_SSP_MODE_MCLP_ENHANCE 25 /*全向降噪: 去混响 + 单通道平稳噪声抑制*/


#ifdef __cplusplus
}
#endif

#endif  // INCLUDE_UAL_SSP_MODE_H_
