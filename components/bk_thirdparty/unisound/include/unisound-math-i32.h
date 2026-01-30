#ifndef LIBUMD_UNISOUND_MATH_I32_H_
#define LIBUMD_UNISOUND_MATH_I32_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief a vector add const number
 *        b[i] =  a[i] + cnt
 *
 * @param[in]  n   - size of vector
 * @param[in]  cnt - const number
 * @param[in]  a   - vector
 * @param[out] b   - operate result
 * @return vector b
 */
int32_t* cbos_i32_addc_v(int32_t n, const int32_t cnt, int32_t* a, int32_t* b);

/**
 * @brief addition of two vector
 *
 * @param[in]  n - size of vector
 * @param[in]  a - vector
 * @param[in]  b - vector
 * @param[out] c - result vector
 * @return vector c
 */
int32_t* cbos_i32_add_vv(int32_t n, const int32_t* a, const int32_t* b,
                         int32_t* c);

/**
 * @brief multiplication of two vector
 *        c[i] = a[i] * b[i]
 *
 * @param[in]  n - size of vector
 * @param[in]  a - vector
 * @param[in]  b - vector
 * @param[out] c - result vector
 * @return vector c
 */
int32_t* cbos_i32_mul_vv(int32_t n, int32_t* a, int32_t* b, int32_t* c);

/**
 * @brief 将数组dst[n]中所有元素设置为value
 *
 * @param[in]  n    - 数组元素个数
 * @param[in]  value    - 需要被设置的值
 * @param[out] dst    - 被设置的数组
 * 
 * @return 被设置的数组
 */
int32_t* cbos_i32_set_v(int32_t n, const int32_t value, int32_t* dst);

#ifdef __cplusplus
}
#endif

#endif  // LIBUMD_UNISOUND_MATH_I32_H_
