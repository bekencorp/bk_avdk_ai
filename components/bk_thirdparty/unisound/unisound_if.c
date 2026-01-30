#include <components/system.h>
#include <os/os.h>
#include <os/mem.h>
#include <driver/uart.h>

#include "include/ual-ssp-args.h"
#include "include/ual-ssp-errno.h"
#include "include/ual-ssp-id.h"
#include "include/ual-ssp-mode.h"
#include "include/ual-ssp.h"

#include "include/ofa_consts.h"
#include "include/lp_asrfix.h"
#include "include/grammar.h"

#include "osal/osal-log.h"
#include "osal/osal-memory.h"
#include "osal/osal-stdio.h"
#include "osal/osal-string.h"
#include "osal/osal-wav.h"

#include "ais-lite-ual-ofa.h"
#include "unisound_if.h"

#define RB_OK    BK_OK
#define RB_FAIL  BK_FAIL
#define UNISOUND_RB_SIZE (640*4)
#define UNISOUND_FRAME_SIZE 256
#define UNISOUND_PROCESS_SIZE 320

typedef struct {
    char *buffer;
    int size;
    int read_pos;
    int write_pos;
    int data_size;
} ringbuf_t;

typedef ringbuf_t* ringbuf_handle_t;

static ringbuf_handle_t g_unisound_mic_rb = NULL;
static ringbuf_handle_t g_unisound_ref_rb = NULL;
static ringbuf_handle_t g_unisound_out_rb = NULL;

static ringbuf_handle_t rb_create(int size) {
    ringbuf_handle_t rb = (ringbuf_handle_t)os_malloc(sizeof(ringbuf_t));
    if (!rb) {
        return NULL;
    }

    rb->buffer = (char *)os_malloc(size);
    if (!rb->buffer) {
        os_free(rb);
        return NULL;
    }

    rb->size = size;
    rb->read_pos = 0;
    rb->write_pos = 0;
    rb->data_size = 0;

    return rb;
}

static void rb_destroy(ringbuf_handle_t rb) {
    if (rb) {
        if (rb->buffer) {
            os_free(rb->buffer);
        }
        os_free(rb);
    }
}

static int rb_write(ringbuf_handle_t rb, const char *data, int len, int timeout) {
    if (!rb || !data || len <= 0) {
        return RB_FAIL;
    }

    if (len > rb->size - rb->data_size) {
        os_printf("rb_write fail\r\n");
        return RB_FAIL;
    }

    int first_part = rb->size - rb->write_pos;
    if (first_part > len) {
        os_memcpy(rb->buffer + rb->write_pos, data, len);
        rb->write_pos += len;
    } else {
        os_memcpy(rb->buffer + rb->write_pos, data, first_part);
        os_memcpy(rb->buffer, data + first_part, len - first_part);
        rb->write_pos = len - first_part;
    }

    rb->data_size += len;
    return RB_OK;
}

static int rb_read(ringbuf_handle_t rb, char *data, int len, int timeout) {
    if (!rb || !data || len <= 0) {
        return RB_FAIL;
    }

    if (len > rb->data_size) {
        os_printf("rb_read fail\r\n");
        return RB_FAIL;
    }

    int first_part = rb->size - rb->read_pos;
    if (first_part > len) {
        os_memcpy(data, rb->buffer + rb->read_pos, len);
        rb->read_pos += len;
    } else {
        os_memcpy(data, rb->buffer + rb->read_pos, first_part);
        os_memcpy(data + first_part, rb->buffer, len - first_part);
        rb->read_pos = len - first_part;
    }

    rb->data_size -= len;
    return RB_OK;
}

static int rb_bytes_filled(ringbuf_handle_t rb) {
    return rb ? rb->data_size : 0;
}

int unisound_rb_init(void) {
    if (g_unisound_mic_rb != NULL) {
        return RB_OK;
    }

    g_unisound_mic_rb = rb_create(UNISOUND_RB_SIZE);
    if (g_unisound_mic_rb == NULL) {
        os_printf("create mic ring buffer failed: out of memory\r\n");
        return RB_FAIL;
    }

    g_unisound_ref_rb = rb_create(UNISOUND_RB_SIZE);
    if (g_unisound_ref_rb == NULL) {
        rb_destroy(g_unisound_mic_rb);
        g_unisound_mic_rb = NULL;
        os_printf("create ref ring buffer failed: out of memory\r\n");
        return RB_FAIL;
    }

    g_unisound_out_rb = rb_create(UNISOUND_RB_SIZE);
    if (g_unisound_out_rb == NULL) {
        rb_destroy(g_unisound_mic_rb);
        g_unisound_mic_rb = NULL;
        rb_destroy(g_unisound_ref_rb);
        g_unisound_ref_rb = NULL;
        os_printf("create out ring buffer failed: out of memory\r\n");
        return RB_FAIL;
    }

    os_printf("create ring buffers ok\r\n");
    return RB_OK;
}

void unisound_rb_deinit(void) {
    if (g_unisound_mic_rb) {
        rb_destroy(g_unisound_mic_rb);
        g_unisound_mic_rb = NULL;
    }
    if (g_unisound_ref_rb) {
        rb_destroy(g_unisound_ref_rb);
        g_unisound_ref_rb = NULL;
    }
    if (g_unisound_out_rb) {
        rb_destroy(g_unisound_out_rb);
        g_unisound_out_rb = NULL;
    }
}

uint32_t unisound_algo_get_workspace_size()
{
    int32_t workspace_size = 0;
    if (UalSspGetWorkspaceSize(UAL_SSP_MODE_AEC_MCLP_ENHANCE, &workspace_size) != UAL_SSP_OK) {
        os_printf("UalSspGetWorkspaceSize failed\r\n");
        return 0;
    } 
    return workspace_size;
}

static void *kws_handle = NULL;
uint32 unisound_kws_buf[20*1024/4];

// Performance monitoring variables
static uint32_t kws_end_time = 0;          // End time
static uint32_t kws_total_time = 0;        // Total processing time (milliseconds)
static uint32_t kws_start_time = 0;        // Start time
static uint32_t kws_process_count = 0;     // Process count counter


void unisound_algo_init(void* work_buf_ptr, uint32_t workspace_size)
{
    void* mem = NULL;
    int16 temp_buf[UNISOUND_PROCESS_SIZE];

    // mem = (void*)os_malloc(workspace_size);
    mem = work_buf_ptr;
    if (mem == NULL) {
        os_printf("work_buf_ptr is NULL, unisound_algo_init fail\r\n");
        return;
    }

    os_memset(mem, 0, workspace_size);

    if (UalSspInit(UAL_SSP_MODE_AEC_MCLP_ENHANCE, mem, workspace_size) != UAL_SSP_OK) {
        os_printf("ual ssp init fail\r\n");
        os_free(mem);
        return;
    }

    if (unisound_rb_init() != RB_OK) {
        os_printf("ring buffer init fail\r\n");
        return;
    }

    os_memset(temp_buf, 0, UNISOUND_PROCESS_SIZE * sizeof(int16_t));
    rb_write(g_unisound_out_rb, (char *)temp_buf, UNISOUND_PROCESS_SIZE * sizeof(int16_t), 0);
    rb_write(g_unisound_out_rb, (char *)temp_buf, UNISOUND_PROCESS_SIZE * sizeof(int16_t), 0);
    // os_printf("mic_filled3 %d bytes %d \r\n", rb_bytes_filled(g_unisound_out_rb));

    os_printf("ual ssp init ok\r\n");
}

void unisound_algo_process(int16_t* mic, int16_t* ref, int16_t* output)
{
    int32_t out_samples = 0;
    int32_t out_len = 0;
    int16_t temp_out_buf[640];
    int16_t * temp_out = NULL;
    int ret;

    if (!mic || !ref || !output) {
        os_printf("unisound_algo_process: invalid parameters\r\n");
        return;
    }

    if (!g_unisound_mic_rb || !g_unisound_ref_rb || !g_unisound_out_rb) {
        os_printf("unisound_algo_process: ring buffers not initialized\r\n");
        return;
    }

    temp_out = temp_out_buf;
    // os_printf("mic_filled4 %d bytes %d \r\n", rb_bytes_filled(g_unisound_out_rb),rtos_get_time());
 
    ret = rb_read(g_unisound_out_rb, (char *)output, UNISOUND_PROCESS_SIZE * sizeof(int16_t), 0);
    if (ret != RB_OK) {
        os_printf("unisound_algo_process: read out buffer failed\r\n");
        return;
    }

    ret = rb_write(g_unisound_mic_rb, (char *)mic, UNISOUND_PROCESS_SIZE * sizeof(int16_t), 0);
    if (ret != RB_OK) {
        os_printf("unisound_algo_process: write mic buffer failed\r\n");
        return;
    }

    ret = rb_write(g_unisound_ref_rb, (char *)ref, UNISOUND_PROCESS_SIZE * sizeof(int16_t), 0);
    if (ret != RB_OK) {
        os_printf("unisound_algo_process: write ref buffer failed\r\n");
        return;
    }
    
    // os_printf("mic_filled5 %d %d bytes %d \r\n", rb_bytes_filled(g_unisound_out_rb),rb_bytes_filled(g_unisound_mic_rb),rtos_get_time());
 
    while(rb_bytes_filled(g_unisound_mic_rb) >= UNISOUND_FRAME_SIZE*sizeof(int16_t))
    {
        ret = rb_read(g_unisound_mic_rb, (char *)mic, UNISOUND_FRAME_SIZE * sizeof(int16_t), 0);
        if (ret != RB_OK) {
            os_printf("unisound_algo_process: read mic buffer failed\r\n");
            return;
        }

        ret = rb_read(g_unisound_ref_rb, (char *)ref, UNISOUND_FRAME_SIZE * sizeof(int16_t), 0);
        if (ret != RB_OK) {
            os_printf("unisound_algo_process: read ref buffer failed\r\n");
            return;
        }

        if (UalSspProcess(mic, ref, UNISOUND_FRAME_SIZE, &temp_out, &out_samples, &out_len) != UAL_SSP_OK ) {
            os_printf("UalSspProcess failed %d %d \r\n",out_samples,out_len);
        }

        ret = rb_write(g_unisound_out_rb, (char *)temp_out, UNISOUND_FRAME_SIZE * sizeof(int16_t), 0);
        if (ret != RB_OK) {
            os_printf("unisound_algo_process: write out buffer failed\r\n");
            return;
        }

        // os_printf("mic_filled %d bytes %d %d %d %d\r\n", rb_bytes_filled(g_unisound_out_rb),rb_bytes_filled(g_unisound_mic_rb),out_samples,out_len,rtos_get_time());
    }
}

void OsalMemTestStart();
size_t OsalMemUsedSize();
void OsalMemTestEnd();

void unisound_memshow()
{
	uint32_t total_size,free_size,mini_size;
	total_size = rtos_get_total_heap_size();
	free_size  = rtos_get_free_heap_size();
	mini_size  = rtos_get_minimum_free_heap_size();
	os_printf("unisound heap\t%d\t%d\t%d\t%d\r\n",  total_size,free_size,mini_size,total_size-mini_size);
}

static const char g_lib_type[] = "ai-kws-off";

static int write_authCode_cb(char* aiCodeType, char* authCode_buf, int buf_len, char* authMsg_buf, int authMsg_buf_len)
{
	os_printf("info [%s:%d] tpye=%s buf_len = %d authCode_buf is \n%s\n",__func__, __LINE__, aiCodeType, buf_len, authCode_buf);
	os_printf("info [%s:%d] tpye=%s buf_len = %d authCode_buf is \n%s\n",__func__, __LINE__, aiCodeType, authMsg_buf_len, authMsg_buf);

	return 0;
}

// Device activation information configuration
// Note: This configuration must be kept in sync with g_device_info in components/bk_unisound_asr/src/bk_unisound_usrkey_http.c
// If you modify the configuration here (e.g., APP_KEY, APP_SECRET, etc.), please synchronize the corresponding configuration in bk_unisound_usrkey_http.c
static DeviceInfo g_device_info = {
    .deviceInfo = {
        [DEVICE_INFO_APP_KEY] = "11111111111111111111111111111111",
        [DEVICE_INFO_APP_SECRET] = "11111111111111111111111111111111",
        [DEVICE_INFO_UNIQUE_ID] = "eth0",

        [DEVICE_INFO_IMEI] = "111",
        [DEVICE_INFO_MAC] = "",
        [DEVICE_INFO_REMARK] = "33",
    },

    .ais_lite_status = 0,

    .read_authCode_cb = NULL,
    .write_authCode_cb = write_authCode_cb,
    .get_time_cb = NULL,
    .get_cloud_encryption_cb = NULL,

    .ais_lite_handle = NULL,
};

bk_err_t unisound_kws_init(void* kws_buf_ptr, uint32_t kws_buf_size)
{
    bk_err_t ret = BK_FAIL;

	unisound_memshow();
	/* AisLiteUalOFAInitializeFromBuffer = UalOFAInitializeHandle + UalOFALoadFromBuffer */
	kws_handle = AisLiteUalOFAInitializeFromBuffer(global_kws_lp_acoutstic_model, global_kws_lp_grammar, g_lib_type, (char *)kws_buf_ptr, &g_device_info);

	const char* version = AisLiteUalOFAGetVersion(kws_handle); 
	os_printf("\r\nUnisound KWS Version: %s %x\n", version,kws_handle);

	char *test_buf_ptr = OsalMalloc(32);
	os_printf("test 0x%x\r\n",test_buf_ptr);
	int default_am_id = AisLiteUalOFAGetActiveAmId(kws_handle);

	AisLiteUalOFASetOptionInt(kws_handle, KWS_SET_BUNCH_FRAME_NUMBER, 12);

	const char* domain = "wakeup";
//	const char* domain = "ivm";

	char *decoder_pool = (char *)unisound_kws_buf;
	AisLiteUalOFAReset(kws_handle);

	int status = AisLiteUalOFAStart(kws_handle, domain, default_am_id, decoder_pool, 20*1024);

	if (status != 0) {
		os_printf("AisLiteUalOFAStart failed with status: %d\n", status);
        ret = BK_FAIL;
        goto exit;
	}
    ret = BK_OK;
exit:
    return ret;
}

uint32 g_dump_kws_data_flag = 0;
uint8_t g_unisound_wakeup_detected = 0;

/**
 * e.g. "prUalOFARecognize status= 0 <s> <wakeup_> 拜拜 阿米 诺 </wakeup_> </s> -3.17"
 */
static int parse_wakeup_result(const char* result_str, char* wakeup_word, int wakeup_word_len, float* score)
{
    if (!result_str) {
        return -1;
    }

    const char* wakeup_start_tag = OsalStrstr(result_str, "<wakeup_>");
    if (!wakeup_start_tag) {
        return -1;
    }

    const char* wakeup_word_start = wakeup_start_tag + OsalStrlen("<wakeup_>");
    const char* wakeup_end_tag = OsalStrstr(wakeup_word_start, "</wakeup_>");
    if (!wakeup_end_tag) {
        return -1;
    }

    while (wakeup_word_start < wakeup_end_tag && (*wakeup_word_start == ' ' || *wakeup_word_start == '\t')) {
        wakeup_word_start++;
    }

    while (wakeup_end_tag > wakeup_word_start && (*(wakeup_end_tag - 1) == ' ' || *(wakeup_end_tag - 1) == '\t')) {
        wakeup_end_tag--;
    }

    int wakeup_word_size = wakeup_end_tag - wakeup_word_start;
    if (wakeup_word && wakeup_word_len > 0) {
        int copy_len = (wakeup_word_size < wakeup_word_len - 1) ? wakeup_word_size : (wakeup_word_len - 1);
        OsalMemcpy(wakeup_word, wakeup_word_start, copy_len);
        wakeup_word[copy_len] = '\0';
    }

    const char* str_end = result_str + OsalStrlen(result_str);
    const char* score_start = NULL;
    const char* score_end = str_end;
    int is_negative = 0;

    for (const char* p = str_end - 1; p >= result_str; p--) {
        if ((*p >= '0' && *p <= '9') || *p == '.') {
            if (score_end == str_end) {
                score_end = p + 1;
            }
            if (!score_start) {
                score_start = p;
            }
        } else if (*p == '-') {
            if (score_start) {
                is_negative = 1;
                score_start = p;
                break;
            }
        } else if (*p == '+') {
            if (score_start) {
                score_start = p;
                break;
            }
        } else if (score_start) {
            if (*p != ' ' && *p != '\t' && *p != '\n' && *p != '\r') {
                break;
            }
        }
    }

    if (score && score_start && score_end > score_start) {
        char score_buf[32];
        int score_len = score_end - score_start;
        if (score_len < sizeof(score_buf)) {
            OsalMemcpy(score_buf, score_start, score_len);
            score_buf[score_len] = '\0';
            *score = 0.0f;
            float factor = 1.0f;
            int decimal_found = 0;
            int decimal_pos = -1;
            int start_idx = (score_buf[0] == '-' || score_buf[0] == '+') ? 1 : 0;
            
            for (int i = start_idx; i < score_len; i++) {
                if (score_buf[i] == '.') {
                    decimal_pos = i;
                    decimal_found = 1;
                    break;
                }
            }
            if (decimal_found) {
                for (int i = decimal_pos - 1; i >= start_idx; i--) {
                    if (score_buf[i] >= '0' && score_buf[i] <= '9') {
                        *score += (score_buf[i] - '0') * factor;
                        factor *= 10.0f;
                    }
                }
                factor = 0.1f;
                for (int i = decimal_pos + 1; i < score_len; i++) {
                    if (score_buf[i] >= '0' && score_buf[i] <= '9') {
                        *score += (score_buf[i] - '0') * factor;
                        factor *= 0.1f;
                    }
                }
            } else {
                for (int i = score_len - 1; i >= start_idx; i--) {
                    if (score_buf[i] >= '0' && score_buf[i] <= '9') {
                        *score += (score_buf[i] - '0') * factor;
                        factor *= 10.0f;
                    }
                }
            }
            if (is_negative) {
                *score = -(*score);
            }
        }
    }
    const char* wakeup_word_full = "拜拜 阿米 诺";
    const char* wakeup_word_hi = "嗨 阿米 诺";
    int len_baibai = OsalStrlen(wakeup_word_full);
    int len_hi = OsalStrlen(wakeup_word_hi);
    
    if (wakeup_word_size >= len_baibai && 
        (OsalStrncmp(wakeup_word_start, wakeup_word_full, len_baibai) == 0) && (*score > -2.95f)) {
        g_unisound_wakeup_detected = 2;
    } else if (wakeup_word_size >= len_hi && 
        (OsalStrncmp(wakeup_word_start, wakeup_word_hi, len_hi) == 0) && (*score > -2.95f)) {
        g_unisound_wakeup_detected = 1;
    } else {
        g_unisound_wakeup_detected = 0;
    }

    return 0;
}

void unisound_kws_recognize(signed char* mic_data, int len)
{
    int status = 0;
    uint32_t tick1,tick2;

    __maybe_unused_var(tick1);
    __maybe_unused_var(tick2);

    tick1 =rtos_get_time();

    if(g_dump_kws_data_flag == 1)
    {
        // Dump magic number first
        uint8_t magic_number[8] = {0x5A, 0x5B, 0x5A, 0x5B, 0x5A, 0x5B, 0x5A, 0x5B};
        bk_uart_write_bytes(UART_ID_1, (const void*)magic_number, 8);
        // Dump mic_data using UART
        bk_uart_write_bytes(UART_ID_1, (const void*)mic_data, 512);
    }
    if(g_dump_kws_data_flag == 0)
    {
      // Record start time
      kws_start_time = rtos_get_time();    
      status = AisLiteUalOFARecognize(kws_handle, mic_data, 512); 
      // Record end time and calculate statistics
      kws_end_time = rtos_get_time();
      kws_total_time += (kws_end_time - kws_start_time);
      kws_process_count++;
      
      // Output average time statistics every 1000 processes
      if((kws_process_count == 1000)&&(g_dump_kws_data_flag == 0))  
      {
          float avg_time = (float)kws_total_time / 1000.0f;
          bk_printf("kws total_time: %d ms, avg_time: %.3f ms\n", kws_total_time, avg_time);         
          // Reset counter
          kws_process_count = 0;
          kws_total_time = 0;
      }
    }
    tick2 =rtos_get_time();
    if(status==2){
        const char* result_const = AisLiteUalOFAGetResult(kws_handle);
        __maybe_unused_var(result_const);
         if(g_dump_kws_data_flag == 0)
         {
            bk_printf("prUalOFARecognize status= %s\n",result_const);

            char wakeup_word[128] = {0};
            float score = 0.0f;
            if (parse_wakeup_result(result_const, wakeup_word, sizeof(wakeup_word), &score) == 0) {
                bk_printf("Wakeup word: %s, Score: %.2f\n", wakeup_word, score);
            } else {
                g_unisound_wakeup_detected = 0;
            }
         }
    }
   // os_printf("unisound_kws_recognize %d %d %d \r\n",status,tick2-tick1,rtos_get_time());
}

