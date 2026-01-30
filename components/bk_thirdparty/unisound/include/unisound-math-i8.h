#ifndef LIBUMD_UNISOUND_MATH_I8_H_
#define LIBUMD_UNISOUND_MATH_I8_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

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
int32_t* cbos_i8_mul_mv(int32_t m, int32_t n, const int8_t* a, const int8_t* b,
                        int32_t* c);

#ifdef __cplusplus
}
#endif

#endif  // LIBUMD_UNISOUND_MATH_I8_H_
