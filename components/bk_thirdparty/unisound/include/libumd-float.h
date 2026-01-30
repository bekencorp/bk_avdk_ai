/*
 * Copyright 2020 Unisound AI Technology Co., Ltd.
 * Author: Li Peng, Liu Zhiming
 * All Rights Reserved.
 */

#ifndef LIBUMD_LIBUMD_FLOAT_H_
#define LIBUMD_LIBUMD_FLOAT_H_

#include "libumd-typedefs.h"
/**
 * @brief           浮点快速实数傅里叶变换
 * @param[in,out]   a                   输入输出数据地址
 * @param[in]       n                   数据长度
 * @param[in]       out_sort_type       输出数据排列方式
 * @return          执行状态
 * @retval          0                   成功
 * @retval          1                   失败
 */
int32_t UmdFloatFft(float* a, int32_t n, UmdFftSortType out_sort_type);

/**
 * @brief           浮快速实数逆傅里叶变换
 * @param[in,out]   a                   输入输出数据地址
 * @param[in]       n                   数据长度
 * @param[in]       out_sort_type       输入数据排列方式
 * @return          执行状态
 * @retval          0                   成功
 * @retval          1                   失败
 */
int32_t UmdFloatIfft(float* a, int32_t n, UmdFftSortType in_sort_type);

#endif  // LIBUMD_LIBUMD_FLOAT_H_
