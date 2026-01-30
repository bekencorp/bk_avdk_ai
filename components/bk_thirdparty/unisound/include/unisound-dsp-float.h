/*
 * Copyright 2020 Unisound AI Technology Co., Ltd.
 * Author: Li Peng, Liu Zhiming
 * All Rights Reserved.
 */

#ifndef LIBUMD_UNISOUND_DSP_FLOAT_H_
#define LIBUMD_UNISOUND_DSP_FLOAT_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief power of spectrum
 *        b[i]=(a[i].real)^2 + (a[i].image)^2
 *
 * @param[in]  n   - size of spectrum vector
 * @param[in]  a   - spectrum complex vector
 * @param[out] b   - power of spectrum vector, real vector
 * @return power vector b
 */
float* cbos_c_power_spectrum_v(int32_t n, const float* a, float* b);

/**
 * @brief energy of vector
 *        energy += a[i]^2
 *
 * @param[in]  n - size of vector
 * @param[in]  a - real vector
 * @param[out] b - energy value
 * @return energy b
 */
float* cbos_f_energy_v(int32_t n, const float* a, float* b);

/**
 * @brief scalar of vector's reciprocal
 *        b[i] = alpha / a[i]
 *
 * @param[in]  n      - size of vector
 * @param[in]  alpha  - scalar value
 * @param[in]  a      - vector, each item more than zero
 * @param[out] b      - operation result
 * @return vector b
 */
float* cbos_f_scal_reci_v(int32_t n, float alpha, const float* a, float* b);

/**
 * @brief filter a vector with a FIR filter
 *        b[i] = fir[i]*a[i] + fir[i - 1]*a[i-1] +...+ fir[i-N]a[i-N]
 *
 * @param[in]  m   - size of vector
 * @param[in]  n   - size of filter
 * @param[in]  a   - vector
 * @param[in]  fir - FIR filter coefficients, reversual
 * @param[out] b   - filtered result
 * @return vector b
 * @remarks reverse fir coefficients before calling this interface
 */
float* cbos_f_fir_filter_vv(int32_t m, int32_t n, const float* a,
                            const float* rev_fir, float* b);

/**
 * @brief Fast Fourier Transform
 *
 * @param[in] n - sizeof of FFT
 * @return 0 success, other fail
 */
int32_t cbos_c_fft_init(int32_t n);

/**
 * @brief fast Fourier transform
 *
 * @param[in]  n - size of vector
 * @param[in]  a - transform vector, real vector
 * @param[out] b - tranform result, comlex vector
 * @return vector b
 */
float* cbos_c_fft_v(int32_t n, const float* a, float* b);

/**
 * @brief inverse fast Fourier transform
 *
 * @param[in]  n - size of vector
 * @param[in]  a - transform complex vector
 * @param[out] b - transform result, real vector
 * @return vector b
 */
float* cbos_c_ifft_v(int32_t n, const float* a, float* b);

/**
 * @brief Fast Fourier Transform
 *
 * @parm[in] handle - handle of resource
 * @return 0 success, other fail
 */
int32_t cbos_c_fft_release(int32_t n);

#ifdef __cplusplus
}
#endif

#endif  // LIBUMD_UNISOUND_DSP_FLOAT_H_
