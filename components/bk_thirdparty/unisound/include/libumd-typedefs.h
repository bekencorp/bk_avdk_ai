/*
 * Copyright 2020 Unisound AI Technology Co., Ltd.
 * Author: Li Peng, Liu Zhiming
 * All Rights Reserved.
 */

#ifndef LIBUMD_LIBUMD_TYPEDEFS_H_
#define LIBUMD_LIBUMD_TYPEDEFS_H_

#include <stdint.h>

/**
 * @brief 矩阵主序，行主序 or 列主序
 */
typedef enum {
  kUmdRowMajor = 0,  ///< row-major arrays
  kUmdColMajor = 1   ///< column-major arrays
} UmdLayout;

/**
 * @brief 矩阵变换方式，不转置 or 转置 or 共轭
 */
typedef enum {
  kUmdNoTrans = 0,   ///< trans='N'
  kUmdTrans = 1,     ///< trans='T'
  kUmdConjTrans = 2  ///< trans='C'
} UmdTranspose;

/**
 * @brief FFT复数排序方式
 */
typedef enum {
  kUmdFftAlter = 0,      ///< real image sort by alter
  kUmdFftMirror = 1,     ///< real image sort by mirror
  kUmdFftConjAlter = 2,  ///< real image sort by conj mirror
} UmdFftSortType;
#endif  // LIBUMD_LIBUMD_TYPEDEFS_H_
