#ifndef __UNISOUND_IF_H__
#define __UNISOUND_IF_H__

#include <stdbool.h>
#include <stdint.h>
#include <common/bk_include.h>
#include <common/bk_err.h>

// /* Ring buffer return codes */
// #define RB_OK    BK_OK
// #define RB_FAIL  BK_FAIL

#ifdef __cplusplus
extern "C" {
#endif

/* Ring buffer operations */
int unisound_rb_init(void);
void unisound_rb_deinit(void);

/* Algorithm operations */
uint32_t unisound_algo_get_workspace_size(void);
void unisound_algo_init(void* work_buf_ptr,uint32_t workspace_size);
void unisound_algo_process(int16_t* mic, int16_t* ref, int16_t* output);
bk_err_t unisound_kws_init(void* kws_buf_ptr, uint32_t kws_buf_size);
void unisound_kws_recognize(signed char* mic_data, int len);

#ifdef __cplusplus
}
#endif

#endif /* __UNISOUND_IF_H__ */
