/*
 * Copyright 2020 Unisound AI Technology Co., Ltd.
 * Author: Hao Peng
 * All Rights Reserved.
 */

#ifndef INCLUDE_UAL_SSP_ERRNO_H_
#define INCLUDE_UAL_SSP_ERRNO_H_

#ifdef __cplusplus
extern "C" {
#endif

#define UAL_SSP_OK 0
#define UAL_SSP_ERR -1
/* 函数入参为空 */
#define UAL_SSP_INPUT_NULL -2
/* ID不支持 */
#define UAL_SSP_ID_ERR -3
/* 当前版本不支持该模式*/
#define UAL_SSP_MODE_NOT_SUPPORT -4
#define UAL_SSP_MALLOC_FAIL -5
#define UAL_SSP_NO_OUTPUT -6
#define UAL_SSP_EXPIRED -7

#ifdef __cplusplus
}
#endif

#endif  // INCLUDE_UAL_SSP_ERRNO_H_
