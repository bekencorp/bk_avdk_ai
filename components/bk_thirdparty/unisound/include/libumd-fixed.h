/*
 * Copyright 2020 Unisound AI Technology Co., Ltd.
 * Author: Li Peng, Liu Zhiming
 * All Rights Reserved.
 */

#ifndef LIBUMD_LIBUMD_FIXED_H_
#define LIBUMD_LIBUMD_FIXED_H_

#include "libumd-typedefs.h"

/**
 * @brief        8 位有符号数矩阵和 8 位无符号数矩阵乘法 C = alpha * op(A) *
 * op(B) + beta * C
 * @details      dimesion: alpha 和 beta 为标量，op(A) 为矩阵 (m*k), op(B)
 * 为矩阵 (k*n), C 为矩阵 (m*n)
 * @param[in]    layout  矩阵主序
 * @param[in]    transa  矩阵 A 是否转置
 * @param[in]    transb  矩阵 B 是否转置
 * @param[in]    m       矩阵 op(A) 的行数
 * @param[in]    n       矩阵 op(B) 的列数
 * @param[in]    k       矩阵 op(A) 的列数
 * @param[in]    alpha   乘法系数
 * @param[in]    a       8 位有符号数矩阵 A 的内存地址
 * @param[in]    lda     矩阵 A 的引导维度
 * @param[in]    b       8 位无符号数矩阵 B 的内存地址
 * @param[in]    ldb     矩阵 B 的引导维度
 * @param[in]    beta    乘法系数
 * @param[out]   c       矩阵 C 的内存地址
 * @param[in]    ldc     矩阵 C 的引导维度
 * @return       void
 */
void UmdIgemmS8U8S32(UmdLayout layout, UmdTranspose transa, UmdTranspose transb,
                     int32_t m, int32_t n, int32_t k, int32_t alpha,
                     const int8_t* a, int32_t lda, const uint8_t* b,
                     int32_t ldb, int32_t beta, int32_t* c, int32_t ldc);
/**
 * @brief        8 位有符号数矩阵和 8 位有符号数矩阵乘法 C = alpha * op(A) *
 * op(B) + beta * C
 * @details      dimesion: alpha 和 beta 为标量，op(A) 为矩阵 (m*k), op(B)
 * 为矩阵 (k*n), C 为矩阵 (m*n)
 * @param[in]    layout  矩阵主序
 * @param[in]    transa  矩阵 A 是否转置
 * @param[in]    transb  矩阵 B 是否转置
 * @param[in]    m       矩阵 op(A) 的行数
 * @param[in]    n       矩阵 op(B) 的列数
 * @param[in]    k       矩阵 op(A) 的列数
 * @param[in]    alpha   乘法系数
 * @param[in]    a       8 位有符号数矩阵 A 的内存地址
 * @param[in]    lda     矩阵 A 的引导维度
 * @param[in]    b       8 位有符号数矩阵 B 的内存地址
 * @param[in]    ldb     矩阵 B 的引导维度
 * @param[in]    beta    乘法系数
 * @param[out]   c       矩阵 C 的内存地址
 * @param[in]    ldc     矩阵 C 的引导维度
 * @return       void
 */
void UmdIgemmS8S8S32(UmdLayout layout, UmdTranspose transa, UmdTranspose transb,
                     int32_t m, int32_t n, int32_t k, int32_t alpha,
                     const int8_t* a, int32_t lda, const int8_t* b, int32_t ldb,
                     int32_t beta, int32_t* c, int32_t ldc);

/**
 * @brief        16 位有符号数矩阵和 16 位有符号数矩阵乘法 C = alpha * op(A) *
 * op(B) + beta * C
 * @details      dimesion: alpha 和 beta 为标量，op(A) 为矩阵 (m*k), op(B)
 * 为矩阵 (k*n), C 为矩阵 (m*n)
 * @param[in]    layout  矩阵主序
 * @param[in]    transa  矩阵 A 是否转置
 * @param[in]    transb  矩阵 B 是否转置
 * @param[in]    m       矩阵 op(A) 的行数
 * @param[in]    n       矩阵 op(B) 的列数
 * @param[in]    k       矩阵 op(A) 的列数
 * @param[in]    alpha   乘法系数
 * @param[in]    a       16 位有符号数矩阵 A 的内存地址
 * @param[in]    lda     矩阵 A 的引导维度
 * @param[in]    b       16 位有符号数矩阵 B 的内存地址
 * @param[in]    ldb     矩阵 B 的引导维度
 * @param[in]    beta    乘法系数
 * @param[out]   c       矩阵 C 的内存地址
 * @param[in]    ldc     矩阵 C 的引导维度
 * @return       void
 */
void UmdIgemmS16S16S32(UmdLayout layout, UmdTranspose transa,
                       UmdTranspose transb, int32_t m, int32_t n, int32_t k,
                       int32_t alpha, const int16_t* a, int32_t lda,
                       const int16_t* b, int32_t ldb, int32_t beta, int32_t* c,
                       int32_t ldc);
/**
 * @brief           定点快速实数傅里叶变换
 * @param[in,out]   a                   输入输出数据地址
 * @param[in]       n                   数据长度
 * @param[in]       out_sort_type       输出数据排列方式
 * @param[in]       shift               中间结果右移位数
 * @return          执行状态
 * @retval          0                   成功
 * @retval          1                   失败
 */
int32_t UmdFixedFft(int32_t* a, int32_t n, UmdFftSortType out_sort_type,
                int32_t shift);

/**
 * @brief           定点快速实数逆傅里叶变换
 * @param[in,out]   a                   输入输出数据地址
 * @param[in]       n                   数据长度
 * @param[in]       out_sort_type       输入数据排列方式
 * @param[in]       shift               中间结果右移位数
 * @return          执行状态
 * @retval          0                   成功
 * @retval          1                   失败
 */
int32_t UmdFixedIfft(int32_t* a, int32_t n, UmdFftSortType in_sort_type,
                 int32_t shift);

/**
 * @brief           32 位有符号数的除以一个数和 16 位有符号数的加法运算
 * @details         y = x*1/alpha + y
 * @param[in]       n       参与运算的 x 和 y 的元素个数
 * @param[in]       alpha   乘法系数
 * @param[in]       x       32 位有符号数向量 x 的内存地址
 * @param[in]       incx    向量 x 的步长
 * @param[in,out]   y       16 位有符号数向量 y 的内存地址
 * @param[in]       incy    向量 y 的步长
 * @return          void
 */
void UmdIraxpyS32S16(int32_t n, int32_t alpha, int32_t* x, int32_t incx,
                     int16_t* y, int32_t incy);

/**
 * @brief            32 位有符号数除以一个数和 8 位有符号数的加法运算
 * @details          y = x*1/alpha + y
 * @param[in]        n       参与运算的 x 和 y 的元素个数
 * @param[in]        alpha   乘法系数
 * @param[in]        x       32 位有符号数向量 x 的内存地址
 * @param[in]        incx    向量 x 的步长
 * @param[in,out]    y       8 位有符号数向量 y 的内存地址
 * @param[in]        incy    向量 y 的步长
 * @return           void
 */
void UmdIraxpyS32S8(int32_t n, int32_t alpha, int32_t* x, int32_t incx,
                    int8_t* y, int32_t incy);

/**
 * @brief            32 位有符号数右移 nshift 位转换为 8 位有符号数
 * @details          y = (x >> nshift) + y
 * @param[in]        n       参与运算的 x 和 y 的元素个数
 * @param[in]        nshift   乘法系数
 * @param[in]        x       32 位有符号数向量 x 的内存地址
 * @param[in]        incx    向量 x 的步长
 * @param[in,out]    y       8 位有符号数向量 y 的内存地址
 * @param[in]        incy    向量 y 的步长
 * @return           void
 */
void UmdIrshiftS32S8(int32_t n, uint32_t nshift, int32_t* x, int32_t incx,
                     int8_t* y, int32_t incy);

/**
 * @brief             32 位有符号数右移 nshift 位转换为 16 位有符号数
 * @details           y = x >> nshift + y
 * @param[in]         n       参与运算的 x 和 y 的元素个数
 * @param[in]         nshift   乘法系数
 * @param[in]         x       32 位有符号数向量 x 的内存地址
 * @param[in]         incx    向量 x 的步长
 * @param[in,out]     y       16 位有符号数向量 y 的内存地址
 * @param[in]         incy    向量 y 的步长
 * @return            void
 */
void UmdIrshiftS32S16(int32_t n, uint32_t nshift, int32_t* x, int32_t incx,
                      int16_t* y, int32_t incy);

#endif  // LIBUMD_LIBUMD_FIXED_H_
