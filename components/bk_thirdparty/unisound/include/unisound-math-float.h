/*
 * Copyright 2020 Unisound AI Technology Co., Ltd.
 * Author: Li Peng, Liu Zhiming
 * All Rights Reserved.
 */

#ifndef LIBUMD_UNISOUND_MATH_FLOAT_H_
#define LIBUMD_UNISOUND_MATH_FLOAT_H_

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
float* cbos_f_addc_v(int32_t n, float cnt, const float* a, float* b);

/**
 * @brief const number subtract a vector
 *        b[i] = cnt - a[i]
 *
 * @param[in]  n   - size of vector
 * @param[in]  cnt - const number
 * @param[in]  a   - vector
 * @param[out] b   - operate result
 * @return vector b
 */
float* cbos_f_csub_v(int32_t n, float cnt, const float* a, float* b);

/**
 * @brief a vector subtract const number
 *        b[i] =  a[i] - cnt
 *
 * @param[in]  n   - size of vector
 * @param[in]  cnt - const number
 * @param[in]  a   - vector
 * @param[out] b   - operate result
 * @return vector b
 */
float* cbos_f_subc_v(int32_t n, float cnt, const float* a, float* b);

/**
 * @brief summation of vevtor
 *
 * @param[in]  n - size of vevtor
 * @param[in]  a - vector
 * @param[out] b - sum value
 * @return summation value b
 */
float* cbos_f_sum_v(int32_t n, const float* a, float* b);

/**
 * @brief norm of vector
 *
 * @param[in]  n - size of vector
 * @param[in]  a - vector
 * @param[out] b - nore value, a float value
 * @return norm value b
 */
float* cbos_f_norm_v(int32_t n, const float* a, float* b);

/**
 * @brief maximum value for each vector item
 *        c[i] = max(a[i], b[i])
 *
 * @param[in] n  - size of vector
 * @param[in] a  - vector
 * @param[in] b  - vector
 * @param[out] c - result vector
 * @return vector c
 */
float* cbos_f_max_vv(int32_t n, const float* a, const float* b, float* c);

/**
 * @brief maximum value for vector and const value item
 *        c[i] = max(a[i], b)
 *
 * @param[in] n  - len of vector
 * @param[in] a  - vector
 * @param     b  - value
 * @param[out] c - result vector
 * @return vector c
 */
float* cbos_f_maxc_vv(int32_t n, const float* a, const float b, float* c);

/**
 * @brief minimum value for each vector
 *        c[i] = min(a[i], b[i])
 *
 * @param[in] n  - size of vector
 * @param[in] a  - vector
 * @param[in] b  - vector
 * @param[out] c - result vector
 * @return vector c
 */
float* cbos_f_min_vv(int32_t n, const float* a, const float* b, float* c);

/**
 * @brief minimum value for vector and const value item
 *        c[i] = min(a[i], b)
 *
 * @param[in] n  - size of vector
 * @param[in] a  - vector
 * @param     b  - value
 * @param[out] c - result vector
 * @return vector c
 */
float* cbos_f_minc_vv(int32_t n, const float* a, const float b, float* c);

/**
 * @brief addition of two vector
 *
 * @param[in]  n - size of vector
 * @param[in]  a - vector
 * @param[in]  b - vector
 * @param[out] c - result vector
 * @return vector c
 */
float* cbos_f_add_vv(int32_t n, const float* a, const float* b, float* c);

/**
 * @brief subtraction of two vector
 *
 * @param[in]  n - size of vector
 * @param[in]  a - vector
 * @param[in]  b - vector
 * @param[out] c - result vector
 * @return vector c
 */
float* cbos_f_sub_vv(int32_t n, const float* a, const float* b, float* c);

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
float* cbos_f_mul_vv(int32_t n, const float* a, const float* b, float* c);

/**
 * @brief division of two vector
 *        c[i] = a[i] / b[i]
 *
 * @param[in]  n - size of vector
 * @param[in]  a - dividend
 * @param[in]  b - divisor
 * @param[out] c - result vector
 * @return vector c
 */
float* cbos_f_div_vv(int32_t n, const float* a, const float* b, float* c);

/**
 * @brief division of const value and vector
 *        c[i] = a / b[i]
 *
 * @param[in]  n - size of vector
 * @param[in]  a - dividend
 * @param[in]  b - divisor
 * @param[out] c - result vector
 * @return vector c
 */
float* cbos_f_cdiv_vv(int32_t n, const float a, const float* b, float* c);

/**
 * @brief sqrt of vector
 *        b[i] = sqrtf(a[i])
 * @param[in]  n - sizeof vector
 * @param[in]  a - vector
 * @param[out] b - square root
 */
float* cbos_f_sqrtf_v(int32_t n, const float* a, float* b);

/**
 * @brief logf of vector
 *       b[i] = logf(a[i])
 *
 * @param[in]  n    - sizeof vector
 * @param[in]  a    - vector
 * @param[out]  b   - logf vector of a
 * @return vector b
 */
float* cbos_f_logf_v(int32_t n, const float* a, float* b);

/**
 * @brief expf of vector
 *       b[i] = expf(a[i])
 *
 * @param[in]  n    - sizeof vector
 * @param[in]  a    - vector
 * @param[out]  b   - expf vector of a
 * @return vector b
 */
float* cbos_f_expf_v(int32_t n, const float* a, float* b);

/**
 * @brief vector int16_t to vector float
 *       b[i] = (float)(a[i])
 *
 * @param[in]  n    - sizeof vector
 * @param[in]  a    - vector
 * @param[out] b    - vector
 * @return vector b
 */
float* cbos_short_to_float_v(int32_t n, const int16_t* a, float* b);

/**
 * @brief vector float to vector int32_t
 *       b[i] = (int16_t)(a[i])
 *
 * @param[in]  n    - sizeof vector
 * @param[in]  a    - vector
 * @param[out] b    - vector
 * @return vector b
 */
int16_t* cbos_float_to_short_v(int32_t n, const float* a, int16_t* b);

/**
 * @brief vector int32_t to vector float
 *       b[i] = (float)(a[i])
 *
 * @param[in]  n    - sizeof vector
 * @param[in]  a    - vector
 * @param[out] b    - vector
 * @return vector b
 */
float* cbos_int_to_float_v(int32_t n, const int32_t* a, float* b);

/**
 * @brief vector float to vector int32_t
 *       b[i] = (int32_t)(a[i])
 *
 * @param[in]  n    - sizeof vector
 * @param[in]  a    - vector
 * @param[out] b    - vector
 * @return vector b
 */
int32_t* cbos_float_to_int_v(int32_t n, const float* a, int32_t* b);

/**
 * @brief addition of two complex vector with single precision
 *
 * @param[in]  n - size of of vector
 * @param[in]  a - complex vector
 * @param[in]  b - complex vector
 * @param[out] c - result vector
 * @return vector c
 */
float* cbos_c_add_vv(int32_t n, const float* a, const float* b, float* c);

/**
 * @brief subtraction of two complex vector with single precision
 *
 * @param[in]   n - size of vector
 * @param[in]   a - complex vector
 * @param[in]   b - complex ector
 * @param[out]  c - result vector
 * @return vector c
 */
float* cbos_c_sub_vv(int32_t n, const float* a, const float* b, float* c);

/**
 * @brief multiplication of two complex vector with single precision
 *
 * @param[in]   n - size of vector
 * @param[in]   a - complex vector
 * @param[in]   b - complex vector
 * @param[out]  c - result vector
 * @return vector c
 */
float* cbos_c_mul_vv(int32_t n, const float* a, const float* b, float* c);

/**
 * @brief conjugation operation with single precision
 *
 * @param[in]  n - size of vector
 * @param[in]  a - complex vector
 * @param[out] b - conjugate vector
 * @return conjugate vector b
 */
float* cbos_c_conj_v(int32_t n, const float* a, float* b);

/**
 * @brief multiplication of conjutation of vector a and vector b
 *
 * @param[in]   n - size of vector
 * @param[in]   a - complex vector
 * @param[in]   b - complex vector
 * @param[out]  c - result vector
 * @return vector c
 */
float* cbos_c_conj_mul_vv(int32_t n, const float* a, const float* b, float* c);

/**
 * @brief scalar of vector
 *        b[i] = alpha * a[i]
 *
 * @param[in]  n     - size of vector
 * @param[in]  alpha - scalar value
 * @param[in]  a     - vector
 * @param[out] b     - scalar result vector
 * @return vector b
 */
float* cbos_f_scal_v(int32_t n, const float alpha, const float* a, float* b);

/**
 * @brief scalar of complex vector
 *        b[i] = alpha * a[i]
 *
 * @param[in]  n     - size of vector
 * @param[in]  alpha - scalar value
 * @param[in]  a     - complex vector
 * @param[out] b     - scalar result vector
 * @return vector b
 */
float* cbos_c_fscal_v(int32_t n, const float alpha, const float* a, float* b);

/**
 * @brief scalar of complex vector with a scale vector
 *        b[i] = alpha[i] * a[i]
 *
 * @param[in]  n     - size of vector
 * @param[in]  alpha - scale vector
 * @param[in]  a     - complex vector
 * @param[out] b     - scalar result complex vector
 * @return venctor b
 */
float* cbos_c_fscal_vv(int32_t n, const float* alpha, const float* a, float* b);

/**
 *@brief multiplication of matrix and vector with float
 *
 * @param[in] m          - row of matrix
 * @param[in] n          - column of matrix
 * @param[in] a          - matrix
 * @param[in] b          - vector
 * @param[out] c         - result vector
 * @return vector c
 * */
float* cbos_f_mul_mv(int32_t m, int32_t n, const float* a, const float* b,
                     float* c);

/**
 *@brief multiplication of two vectors with float
 *
 * @param[in]  n          - size of vector
 * @param[in]  a          - vector
 * @param[in]  b          - vector
 * @param[out] c          - dot result, a float value
 * @return dot value
 * */
float* cbos_f_dot_vv(int32_t n, const float* a, const float* b, float* c);

/**
 *@brief multiplication of two vectors with complex type
 *
 * @param[in]  n          - size of vector
 * @param[in]  a          - vector
 * @param[in]  b          - vector
 * @param[out] c          - dot result, a complex value
 * @return dot value
 * */
float* cbos_c_dot_vv(int32_t n, const float* a, const float* b, float* c);

/**
 *@brief multiplication of two vectors with  conjugate complex type
 *
 * @param[in]  n          - size of vector
 * @param[in]  a          - vector
 * @param[in]  b          - vector
 * @param[out] c          - dot result, a complex value
 * @return dot value
 * */
float* cbos_c_conj_dot_vv(int32_t n, const float* a, const float* b, float* c);

/**
 * @brief scalar of complex vector
 *        b[i] = alpha * a[i]
 *
 * @param[in]  n     - size of vector
 * @param[in]  alpha - scalar value, complex type
 * @param[in]  a     - complex vector
 * @param[out] b     - scalar result vector
 * @return vector b
 */
float* cbos_c_cscal_v(int32_t n, const float* alpha, const float* a, float* b);

/**
 * @brief mod of complex vector
 *       b[i] = sqrt(a[i].real * a[i].real + a[i].image * a[i].image)
 *
 * @param[in]  n    - sizeof vector
 * @param[in]  a    - complex vector
 * @param[out]  b    - mod vector of a
 * @return vector b
 */
float* cbos_c_mod_v(int32_t n, const float* a, float* b);

/**
 * @brief multiplication of transpose of vector a and vector b
 *      c = a^T * b
 *
 * @param[in]  n    - sizeof vector
 * @param[in]  a    - real vector
 * @param[in]  b    - real vector
 * @param[out] c    - cross-product, a matrix
 * @return vector c
 */
float* cbos_f_trans_mul_vv(int32_t n, const float* a, const float* b, float* c);

/**
 * @brief multiplication of transpose of vector a and vector b
 *      c = conj(a) * b
 *
 * @param[in]  n    - sizeof vector
 * @param[in]  a    - complex vector
 * @param[in]  b    - complex vector
 * @param[out] c    - cross-product, a matrix
 * @return vector c
 */
float* cbos_c_trans_mul_vv(int32_t n, const float* a, const float* b, float* c);

/**
 * @brief 将数组dst[n]中所有元素设置为value
 *
 * @param[in]  n    - 数组元素个数
 * @param[in]  value    - 需要被设置的值
 * @param[out] dst    - 被设置的数组
 * 
 * @return 被设置的数组
 */
float* cbos_f_set_v(int32_t n, const float value, float* dst);

#ifdef __cplusplus
}
#endif

#endif  // LIBUMD_UNISOUND_MATH_FLOAT_H_
