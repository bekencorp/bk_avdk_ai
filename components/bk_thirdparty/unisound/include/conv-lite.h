/*
 * Copyright 2020 Unisound AI Technology Co., Ltd.
 * Author: Li Peng, Liu Zhiming
 * All Rights Reserved.
 */

#ifndef CONV_LITE_H__
#define CONV_LITE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>


int32_t* cbos_i_Conv2412(int32_t n, const int8_t* a, const int8_t* b, const int8_t* c, int32_t* d);
//void Conv2412_8x8(const int8_t* kernel, const int8_t* in1, const int8_t* in2,
//                  size_t i_w, int32_t* out);



int32_t* cbos_ic_dot(int32_t n, const int8_t* a, const int8_t* b, int32_t* c);
//int32_t DotProduct_8x8(const int8_t* a, const int8_t* b, size_t len);



int32_t* cbos_ic_scalsum(int32_t n, const int8_t alpha, const int8_t*a, int32_t* c);
//int32_t MADD_8x8(int32_t n, const int8_t alpha, const int8_t* a);



int32_t* cbos_i_sla(int32_t n, const int32_t shift, const int32_t* a, int32_t* b);
//void SLAA32x4(int32_t n, const int32_t shift, const int32_t* a, int32_t* b);



int32_t* cbos_i_sra(int32_t n, const int32_t shift, const int32_t* a, int32_t* b);
//void SRAA32x4(int32_t n, const int32_t shift, const int32_t* a, int32_t* b);



/**
 * @brief 卷积，kernel size 为 2 x 4，stride 为 (1, 2)，输入高度为 2
 * @param[in] kernel 卷积核，大小为 2 x 4
 * @param[in] in1    输入的第一行
 * @param[in] in2    输入的第二行
 * @param[in] i_w    输入列数，需为 2 的整数倍
 */
float* cbos_f_Conv2412(int32_t n, const float* a, const float* b, const float* c, float* d);
//void Conv2412(const float* kernel, const float* in1, const float* in2,
//              size_t i_w, float* out);



/**
 * @brief 卷积，kernel size 为 2 x 8，stride 为 (1, 2)，输入高度为 2
 * @param[in] kernel 卷积核，大小为 2 x 4
 * @param[in] in1    输入的第一行
 * @param[in] in2    输入的第二行
 * @param[in] i_w    输入列数，需为 2 的整数倍
 */
float* cbos_f_Conv2812(int32_t n, const float* a, const float* b, const float* c, float* d);
//void Conv2812(const float* kernel, const float* in1, const float* in2,
//              size_t i_w, float* out);



#ifdef __cplusplus
}
#endif

#endif  // CONV_LITE_H__
