// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include "stdio.h"
#include "sys_driver.h"
#include "aud_intf_private.h"

#include "media_evt.h"
#include "media_mailbox_list_util.h"
#include <driver/pwr_clk.h>
#if (CONFIG_CACHE_ENABLE)
#include "cache.h"
#endif
#include "aud_tras.h"
#include <modules/audio_process.h>
#include "audio_debug.h"

#if CONFIG_AUD_INTF_SUPPORT_MP3
#include "modules/mp3dec.h"
#endif

#define AUD_INTF_TAG "aud_intf"

#define LOGI(...) BK_LOGI(AUD_INTF_TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(AUD_INTF_TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(AUD_INTF_TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(AUD_INTF_TAG, ##__VA_ARGS__)

/* check aud_intf busy status */
#define CHECK_AUD_INTF_BUSY_STA() do {\
		if (aud_intf_info.api_info.busy_status) {\
			return BK_ERR_AUD_INTF_BUSY;\
		}\
		aud_intf_info.api_info.busy_status = true;\
	} while(0)


#define CONFIG_AUD_RX_COUNT_DEBUG
#ifdef CONFIG_AUD_RX_COUNT_DEBUG
#define AUD_RX_DEBUG_INTERVAL (1000 * 2)
#endif

#ifdef CONFIG_AUD_RX_COUNT_DEBUG
typedef struct {
	beken_timer_t timer;
	uint32_t rx_size;
} aud_rx_count_debug_t;

static aud_rx_count_debug_t aud_rx_count = {0};
#endif


#if CONFIG_AUD_INTF_SUPPORT_G722
const char g722_silence_data[] =
{
    0xFA, 
    0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 
    0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 
    0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 0xFA, 0xF7, 0xF7, 0xF7, 0xF7, 0xF7, 
    0xF7, 0xF7, 0xF8, 0xF7, 0xFA, 0xF7, 0xF8, 0xF8, 0xFA, 0xFA, 0xF7, 0xF7, 0xF8, 0xF7, 0xFA, 0xF8, 
    0xF7, 0xFA, 0xF8, 0xF7, 0xFA, 0xF7, 0xF8, 0xF8, 0xFA, 0xDB, 0xF0, 0xF8, 0xF8, 0xF8, 0xF8, 0xFA, 
    0xDB, 0xF1, 0xF8, 0xF8, 0xFA, 0xFA, 0xF8, 0xDE, 0xF3, 0xFA, 0xF8, 0xF8, 0xFA, 0xFA, 0xDE, 0xF0, 
    0xFA, 0xFA, 0xF8, 0xF8, 0xFA, 0xDB, 0xF1, 0xFA, 0xFA, 0xF8, 0xF8, 0xFA, 0xDC, 0xF2, 0xFA, 0xF8, 
    0xF8, 0xFA, 0xFA, 0xDE, 0xF0, 0xFA, 0xFA, 0xF8, 0xF8, 0xFA, 0xD8, 0xF3, 0xFA, 0xF8, 0xF8, 0xFA, 
    0xDE, 0xF3, 0xFA, 0xF8, 0xF8, 0xFA, 0xDE, 0xF3, 0xFA, 0xF8, 0xF8, 0xFA, 0xDE, 0xF2, 0xFA, 0xF8, 
    0xF8, 0xFA, 0xDE, 0xF2, 0xFA, 0xF8, 0xF8, 0xFA, 0xDE, 0xF2, 0xFA, 0xF8, 0xF8, 0xFA, 0xDE, 
};
#endif

static uint32_t aud_intf_aec_vad_flag = 0;
static uint32_t aud_intf_aec_silence_flag = 0;



//aud_intf_all_setup_t aud_all_setup;
aud_intf_info_t aud_intf_info = DEFAULT_AUD_INTF_CONFIG();

static beken_semaphore_t aud_intf_task_sem = NULL;

/* extern api */
static bk_err_t aud_intf_voc_write_spk_data(uint8_t *dac_buff, uint32_t size);
static void aec_vad_status_set(int val);

static void *audio_intf_malloc(uint32_t size)
{
#if CONFIG_PSRAM_AS_SYS_MEMORY
	return psram_malloc(size);
#else
	return os_malloc(size);
#endif
}

static void audio_intf_free(void *mem)
{
	os_free(mem);
}

bk_err_t mailbox_media_aud_send_msg(media_event_t event, void *param)
{
	bk_err_t ret = BK_OK;

	ret = msg_send_req_to_media_app_mailbox_sync(event, (uint32_t)param, NULL);
	if (ret != kNoErr)
	{
		LOGE("%s, %d, fail, ret: 0x%x\n", __func__, __LINE__, ret);
	}

	aud_intf_info.api_info.busy_status = false;
	return ret;
}


bk_err_t aud_intf_send_msg(aud_intf_event_t op, uint32_t data, uint32_t size)
{
	bk_err_t ret;
	aud_intf_msg_t msg;

	msg.op = op;
	msg.data = data;
	msg.size = size;
	if (aud_intf_info.aud_intf_msg_que) {
		ret = rtos_push_to_queue(&aud_intf_info.aud_intf_msg_que, &msg, BEKEN_NO_WAIT);
		if (kNoErr != ret) {
			LOGE("%s, %d, aud_tras_send_int_msg fail \n", __func__, __LINE__);
			return kOverrunErr;
		}

		return ret;
	}
	return kNoResourcesErr;
}

static bk_err_t aud_intf_deinit(void)
{
	bk_err_t ret;
	aud_intf_msg_t msg;

	msg.op = AUD_INTF_EVENT_EXIT;
	msg.data = 0;
	msg.size = 0;
	if (aud_intf_info.aud_intf_msg_que) {
		ret = rtos_push_to_queue_front(&aud_intf_info.aud_intf_msg_que, &msg, BEKEN_NO_WAIT);
		if (kNoErr != ret) {
			LOGE("%s, %d, audio send msg: AUD_INTF_EVENT_EXIT fail \n", __func__, __LINE__);
			return kOverrunErr;
		}

		ret = rtos_get_semaphore(&aud_intf_task_sem, BEKEN_WAIT_FOREVER);
		if (ret != BK_OK)
		{
			LOGE("%s, %d, rtos_get_semaphore\n", __func__, __LINE__);
			return BK_FAIL;
		}

		if(aud_intf_task_sem)
		{
			rtos_deinit_semaphore(&aud_intf_task_sem);
			aud_intf_task_sem = NULL;
		}

		return ret;
	}
	return kNoResourcesErr;
}

static void aud_intf_main(beken_thread_arg_t param_data)
{
	bk_err_t ret = BK_OK;

	rtos_set_semaphore(&aud_intf_task_sem);

	aud_intf_msg_t msg;
	while (1) {
		ret = rtos_pop_from_queue(&aud_intf_info.aud_intf_msg_que, &msg, BEKEN_WAIT_FOREVER);
		if (kNoErr == ret) {
			switch (msg.op) {
				case AUD_INTF_EVENT_IDLE:
					break;

				case AUD_INTF_EVENT_MIC_TX:
					if (aud_intf_info.drv_info.setup.aud_intf_tx_mic_data) {
						aud_intf_info.drv_info.setup.aud_intf_tx_mic_data((unsigned char *)msg.data, msg.size);
					}
					break;
				case AUD_INTF_EVENT_VAD_FLAG_UPDATE:
					if (aud_intf_info.drv_info.setup.aud_intf_update_vad_flag) {
						aud_intf_info.drv_info.setup.aud_intf_update_vad_flag(msg.data);
					}
					aec_vad_status_set(msg.data);
					break;

				case AUD_INTF_EVENT_SPK_RX:
					if (aud_intf_info.drv_info.setup.aud_intf_rx_spk_data) {
						aud_intf_info.drv_info.setup.aud_intf_rx_spk_data(msg.size);
					}
					break;

				case AUD_INTF_EVENT_UAC_STATE:
					if (aud_intf_info.aud_intf_uac_connect_state_cb) {
						aud_intf_info.aud_intf_uac_connect_state_cb((uint8_t)msg.data);
					}
					break;

#if CONFIG_AUD_INTF_SUPPORT_SPK_PLAY_FINISH_NOTIFY
				case AUD_INTF_EVENT_SPK_PLAY_FINISH:
					if (aud_intf_info.voc_info.spk_play_finish_notify) {
						aud_intf_info.voc_info.spk_play_finish_notify(aud_intf_info.voc_info.spk_play_finish_usr_data);
					}
					break;
#endif

#if CONFIG_AUD_INTF_SUPPORT_PROMPT_TONE_PLAY_FINISH_NOTIFY
				case AUD_INTF_EVENT_PROMPT_TONE_PLAY_FINISH:
					if (aud_intf_info.voc_info.prompt_tone_play_finish_notify) {
						aud_intf_info.voc_info.prompt_tone_play_finish_notify(aud_intf_info.voc_info.prompt_tone_play_finish_usr_data);
					}
					break;
#endif

				case AUD_INTF_EVENT_EXIT:
					goto aud_intf_exit;
					break;

				default:
					break;
			}
		}
	}

aud_intf_exit:

	/* delete msg queue */
	ret = rtos_deinit_queue(&aud_intf_info.aud_intf_msg_que);
	if (ret != kNoErr) {
		LOGE("%s, %d, delete message queue fail\n", __func__, __LINE__);
	}
	aud_intf_info.aud_intf_msg_que = NULL;
	LOGI("%s, %d, delete aud_intf_int_msg_que \n", __func__, __LINE__);

	/* delete task */
	aud_intf_info.aud_intf_thread_hdl = NULL;

	rtos_set_semaphore(&aud_intf_task_sem);

	rtos_delete_thread(NULL);
}


bk_err_t bk_aud_intf_set_mode(aud_intf_work_mode_t work_mode)
{
	bk_err_t ret = BK_OK;

	if (aud_intf_info.drv_info.setup.work_mode == work_mode)
		return BK_ERR_AUD_INTF_OK;

	CHECK_AUD_INTF_BUSY_STA();
	aud_intf_work_mode_t temp = aud_intf_info.drv_info.setup.work_mode;
	aud_intf_info.drv_info.setup.work_mode = work_mode;

	/* create task to handle tx and rx message */
//	aud_intf_info.api_info.busy_status = true;

	if (work_mode == AUD_INTF_WORK_MODE_GENERAL || work_mode == AUD_INTF_WORK_MODE_VOICE) {
		if (aud_intf_task_sem == NULL) {
			ret = rtos_init_semaphore(&aud_intf_task_sem, 1);
			if (ret != BK_OK)
			{
				LOGE("%s, %d, create audio intf task semaphore failed \n", __func__, __LINE__);
				goto fail;
			}
		}

		if (!aud_intf_info.aud_intf_msg_que) {
			ret = rtos_init_queue(&aud_intf_info.aud_intf_msg_que,
								  "aud_intf_int_que",
								  sizeof(aud_intf_msg_t),
								  10);
			if (ret != kNoErr) {
				LOGE("%s, %d, create aud_intf_msg_que fail \n", __func__, __LINE__);
				aud_intf_info.aud_intf_msg_que = NULL;
				goto fail;
			}
			LOGD("%s, %d, create aud_intf_msg_que complete \n", __func__, __LINE__);
		}

		if (!aud_intf_info.aud_intf_thread_hdl) {
			ret = rtos_create_thread(&aud_intf_info.aud_intf_thread_hdl,
								 6,
								 "aud_intf",
								 (beken_thread_function_t)aud_intf_main,
								 2048,
								 NULL);
			if (ret != kNoErr) {
				LOGE("%s, %d, create audio transfer task fail \n", __func__, __LINE__);
				rtos_deinit_queue(&aud_intf_info.aud_intf_msg_que);
				aud_intf_info.aud_intf_msg_que = NULL;
				aud_intf_info.aud_intf_thread_hdl = NULL;
				goto fail;
			}

			ret = rtos_get_semaphore(&aud_intf_task_sem, BEKEN_WAIT_FOREVER);
			if (ret != BK_OK)
			{
				LOGE("%s, %d, rtos_get_semaphore\n", __func__, __LINE__);
				goto fail;
			}

			LOGD("%s, %d, create audio transfer driver task complete \n", __func__, __LINE__);
		}

		if (work_mode == AUD_INTF_WORK_MODE_VOICE) {
			/* init audio transfer task */
			aud_tras_setup_t aud_tras_setup_cfg = {0};
			aud_tras_setup_cfg.aud_tras_send_data_cb = aud_intf_info.drv_info.setup.aud_intf_tx_mic_data;
			ret = aud_tras_init(&aud_tras_setup_cfg);
			if (ret != BK_OK) {
				LOGE("%s, %d, aud_tras_init fail \n", __func__, __LINE__);
				goto fail;
			}
		}

		ret = mailbox_media_aud_send_msg(EVENT_AUD_SET_MODE_REQ, (void *)aud_intf_info.drv_info.setup.work_mode);
		if (ret != BK_OK) {
			LOGE("%s, %d, set mode fail, ret: %d \n", __func__, __LINE__, ret);
			goto fail;
		}
	} else if (work_mode == AUD_INTF_WORK_MODE_NULL) {
		ret = mailbox_media_aud_send_msg(EVENT_AUD_SET_MODE_REQ, (void *)aud_intf_info.drv_info.setup.work_mode);
		if (ret != BK_OK) {
			LOGE("%s, %d, set mode fail, ret: %d \n", __func__, __LINE__, ret);
			return ret;
		}

		aud_intf_deinit();
		aud_tras_deinit();
	} else {
		//TODO nothing
	}

	aud_intf_info.api_info.busy_status = false;

	return ret;

fail:
	aud_intf_info.drv_info.setup.work_mode = temp;

	aud_tras_deinit();

	if (aud_intf_info.aud_intf_thread_hdl && aud_intf_info.aud_intf_msg_que) {
		aud_intf_deinit();
	}

	if(aud_intf_task_sem)
	{
		rtos_deinit_semaphore(&aud_intf_task_sem);
		aud_intf_task_sem = NULL;
	}

	aud_intf_info.api_info.busy_status = false;

	return ret;
}

bk_err_t bk_aud_intf_set_mic_gain(uint8_t value)
{
	bk_err_t ret = BK_ERR_AUD_INTF_STA;

	/* check value range */
	if (value > 0x3F)
		return BK_ERR_AUD_INTF_PARAM;

	CHECK_AUD_INTF_BUSY_STA();

	uint8_t temp = 0;

	/*check mic status */
	if (aud_intf_info.mic_status != AUD_INTF_MIC_STA_NULL || aud_intf_info.voc_status != AUD_INTF_VOC_STA_NULL) {
		if (aud_intf_info.mic_status != AUD_INTF_MIC_STA_NULL) {
			temp = aud_intf_info.mic_info.mic_gain;
			aud_intf_info.mic_info.mic_gain = value;
			ret = mailbox_media_aud_send_msg(EVENT_AUD_MIC_SET_GAIN_REQ, &aud_intf_info.mic_info.mic_gain);
			if (ret != BK_OK)
				aud_intf_info.mic_info.mic_gain = temp;
		}

		if (aud_intf_info.voc_status != AUD_INTF_VOC_STA_NULL) {
			temp = aud_intf_info.voc_info.aud_setup.adc_gain;
			aud_intf_info.voc_info.aud_setup.adc_gain = value;
			ret = mailbox_media_aud_send_msg(EVENT_AUD_VOC_SET_MIC_GAIN_REQ, &aud_intf_info.voc_info.aud_setup.adc_gain);
			if (ret != BK_OK)
				aud_intf_info.voc_info.aud_setup.adc_gain = temp;
		}
	}

	aud_intf_info.api_info.busy_status = false;
	return ret;
}

bk_err_t bk_aud_intf_set_mic_samp_rate(uint32_t samp_rate)
{
	bk_err_t ret = BK_ERR_AUD_INTF_STA;

	CHECK_AUD_INTF_BUSY_STA();

	/*check mic status */
	if (aud_intf_info.mic_status != AUD_INTF_MIC_STA_NULL) {
		uint32_t temp = aud_intf_info.mic_info.samp_rate;
		aud_intf_info.mic_info.samp_rate = samp_rate;
		ret = mailbox_media_aud_send_msg(EVENT_AUD_MIC_SET_SAMP_RATE_REQ, (void *)aud_intf_info.mic_info.samp_rate);
		if (ret != BK_OK)
			aud_intf_info.mic_info.samp_rate = temp;
	}

	aud_intf_info.api_info.busy_status = false;
	return ret;
}

bk_err_t bk_aud_intf_set_mic_chl(aud_intf_mic_chl_t mic_chl)
{
	bk_err_t ret = BK_ERR_AUD_INTF_STA;

	CHECK_AUD_INTF_BUSY_STA();

	/*check mic status */
	if (aud_intf_info.mic_status != AUD_INTF_MIC_STA_NULL) {
		aud_intf_mic_chl_t temp = aud_intf_info.mic_info.mic_chl;
		aud_intf_info.mic_info.mic_chl = mic_chl;
		ret = mailbox_media_aud_send_msg(EVENT_AUD_MIC_SET_CHL_REQ, (void *)aud_intf_info.mic_info.mic_chl);
		if (ret != BK_OK)
			aud_intf_info.mic_info.mic_chl = temp;
	}

	aud_intf_info.api_info.busy_status = false;
	return ret;
}

bk_err_t bk_aud_intf_set_spk_gain(uint32_t value)
{
	bk_err_t ret = BK_ERR_AUD_INTF_STA;

	CHECK_AUD_INTF_BUSY_STA();
	uint16_t temp = 0;

	/*check mic status */
	if (aud_intf_info.spk_status != AUD_INTF_SPK_STA_NULL || aud_intf_info.voc_status != AUD_INTF_VOC_STA_NULL) {
		if (aud_intf_info.spk_status != AUD_INTF_SPK_STA_NULL) {
			/* check value range */
			if (aud_intf_info.spk_info.spk_type == AUD_INTF_SPK_TYPE_BOARD) {
				if (value > 0x3F) {
					LOGE("%s, %d, the spk_gain out of range:0x00-0x3F \n", __func__, __LINE__);
					aud_intf_info.api_info.busy_status = false;
					return BK_ERR_AUD_INTF_PARAM;
				}
			} else {
				if (value > 100) {
					LOGE("%s, %d, the spk_gain out of range:0-100 \n", __func__, __LINE__);
					aud_intf_info.api_info.busy_status = false;
					return BK_ERR_AUD_INTF_PARAM;
				}
			}
			temp = aud_intf_info.spk_info.spk_gain;
			aud_intf_info.spk_info.spk_gain = value;
			ret = mailbox_media_aud_send_msg(EVENT_AUD_SPK_SET_GAIN_REQ, &aud_intf_info.spk_info.spk_gain);
			if (ret != BK_OK)
				aud_intf_info.spk_info.spk_gain = temp;
		}

		if (aud_intf_info.voc_status != AUD_INTF_VOC_STA_NULL) {
			/* check value range */
			if (aud_intf_info.voc_info.spk_type == AUD_INTF_SPK_TYPE_BOARD) {
				if (value > 0x3F) {
					LOGE("%s, %d, the spk_gain out of range:0x00-0x3F \n", __func__, __LINE__);
					aud_intf_info.api_info.busy_status = false;
					return BK_ERR_AUD_INTF_PARAM;
				}
			} else {
				if (value > 100) {
					LOGE("%s, %d, the spk_gain out of range:0-100 \n", __func__, __LINE__);
					aud_intf_info.api_info.busy_status = false;
					return BK_ERR_AUD_INTF_PARAM;
				}
			}
			temp = aud_intf_info.voc_info.aud_setup.dac_gain;
			aud_intf_info.voc_info.aud_setup.dac_gain = value;
			LOGI("%s, %d, set spk_gain: %d \n", __func__, __LINE__, aud_intf_info.voc_info.aud_setup.dac_gain);
			//return aud_tras_drv_send_msg(AUD_TRAS_DRV_VOC_SET_SPK_GAIN, &aud_intf_info.voc_info.aud_setup.dac_gain);
			//return aud_intf_send_msg_with_sem(AUD_TRAS_DRV_VOC_SET_SPK_GAIN, &aud_intf_info.voc_info.aud_setup.dac_gain, BEKEN_WAIT_FOREVER);
			ret = mailbox_media_aud_send_msg(EVENT_AUD_VOC_SET_SPK_GAIN_REQ, &aud_intf_info.voc_info.aud_setup.dac_gain);
			if (ret != BK_OK)
				aud_intf_info.voc_info.aud_setup.dac_gain = temp;
		}
	}

	aud_intf_info.api_info.busy_status = false;
	return ret;
}

bk_err_t bk_aud_intf_set_spk_samp_rate(uint32_t samp_rate)
{
	bk_err_t ret = BK_ERR_AUD_INTF_STA;

	CHECK_AUD_INTF_BUSY_STA();

	/*check spk status */
	if (aud_intf_info.spk_status != AUD_INTF_SPK_STA_NULL) {
		uint32_t temp = aud_intf_info.spk_info.samp_rate;
		aud_intf_info.spk_info.samp_rate = samp_rate;
		//return aud_intf_send_msg_with_sem(AUD_TRAS_DRV_SPK_SET_SAMP_RATE, &aud_intf_info.spk_info.samp_rate, BEKEN_WAIT_FOREVER);
		ret = mailbox_media_aud_send_msg(EVENT_AUD_SPK_SET_SAMP_RATE_REQ, (void *)aud_intf_info.spk_info.samp_rate);
		if (ret != BK_OK)
			aud_intf_info.spk_info.samp_rate = temp;
	}

	aud_intf_info.api_info.busy_status = false;
	return ret;
}

bk_err_t bk_aud_intf_set_spk_chl(aud_intf_spk_chl_t spk_chl)
{
	bk_err_t ret = BK_ERR_AUD_INTF_STA;

	CHECK_AUD_INTF_BUSY_STA();

	/*check spk status */
	if (aud_intf_info.spk_status != AUD_INTF_SPK_STA_NULL) {
		aud_intf_spk_chl_t temp = aud_intf_info.spk_info.spk_chl;
		aud_intf_info.spk_info.spk_chl = spk_chl;
		ret = mailbox_media_aud_send_msg(EVENT_AUD_SPK_SET_CHL_REQ, (void *)aud_intf_info.spk_info.spk_chl);
		if (ret != BK_OK)
			aud_intf_info.spk_info.spk_chl = temp;
	}

	aud_intf_info.api_info.busy_status = false;
	return ret;
}

bk_err_t bk_aud_intf_register_uac_connect_state_cb(void *cb)
{
	bk_err_t ret = BK_ERR_AUD_INTF_STA;

	CHECK_AUD_INTF_BUSY_STA();

	ret = mailbox_media_aud_send_msg(EVENT_AUD_UAC_REGIS_CONT_STATE_CB_REQ, cb);
	if (ret == BK_OK) {
		aud_intf_info.aud_intf_uac_connect_state_cb = cb;
	}

	return ret;
}

bk_err_t bk_aud_intf_uac_auto_connect_ctrl(bool enable)
{
	bk_err_t ret = BK_ERR_AUD_INTF_STA;

	if (aud_intf_info.uac_auto_connect == enable) {
		return BK_ERR_AUD_INTF_OK;
	}

	/* check if aud_intf driver init */
	if (aud_intf_info.drv_status != AUD_INTF_DRV_STA_IDLE && aud_intf_info.drv_status != AUD_INTF_DRV_STA_WORK) {
		return BK_ERR_AUD_INTF_STA;
	}

	CHECK_AUD_INTF_BUSY_STA();
	bool temp = aud_intf_info.uac_auto_connect;
	aud_intf_info.uac_auto_connect = enable;
	ret = mailbox_media_aud_send_msg(EVENT_AUD_UAC_AUTO_CONT_CTRL_REQ, (void *)aud_intf_info.uac_auto_connect);
	if (ret != BK_OK) {
		aud_intf_info.uac_auto_connect = temp;
	}

	return ret;
}

bk_err_t bk_aud_intf_set_aec_para(aud_intf_voc_aec_para_t aec_para, uint32_t value)
{
	bk_err_t ret = BK_OK;
//	bk_err_t err = BK_ERR_AUD_INTF_FAIL;
	aud_intf_voc_aec_ctl_t *aec_ctl = NULL;

	/*check aec status */
	if (aud_intf_info.voc_status == AUD_INTF_VOC_STA_NULL)
		return BK_ERR_AUD_INTF_STA;

	CHECK_AUD_INTF_BUSY_STA();

	aec_ctl = audio_intf_malloc(sizeof(aud_intf_voc_aec_ctl_t));
	if (aec_ctl == NULL) {
		aud_intf_info.api_info.busy_status = false;
		return BK_ERR_AUD_INTF_MEMY;
	}
	aec_ctl->op = aec_para;
	aec_ctl->value = value;

	//TODO
	//cpy value before set

	switch (aec_para) {
		case AUD_INTF_VOC_AEC_MIC_DELAY:
			aud_intf_info.voc_info.aec_setup->mic_delay = value;
			break;

		case AUD_INTF_VOC_AEC_EC_DEPTH:
			aud_intf_info.voc_info.aec_setup->ec_depth = value;
			break;

		case AUD_INTF_VOC_AEC_REF_SCALE:
			aud_intf_info.voc_info.aec_setup->ref_scale = value;
			break;

		case AUD_INTF_VOC_AEC_VOICE_VOL:
			aud_intf_info.voc_info.aec_setup->voice_vol = value;
			break;

		case AUD_INTF_VOC_AEC_TXRX_THR:
			aud_intf_info.voc_info.aec_setup->TxRxThr = value;
			break;

		case AUD_INTF_VOC_AEC_TXRX_FLR:
			aud_intf_info.voc_info.aec_setup->TxRxFlr = value;
			break;

		case AUD_INTF_VOC_AEC_NS_LEVEL:
			aud_intf_info.voc_info.aec_setup->ns_level = value;
			break;

		case AUD_INTF_VOC_AEC_NS_PARA:
			aud_intf_info.voc_info.aec_setup->ns_para = value;
			break;

		case AUD_INTF_VOC_AEC_DRC:
			aud_intf_info.voc_info.aec_setup->drc = value;
			break;

		case AUD_INTF_VOC_AEC_INIT_FLAG:
			aud_intf_info.voc_info.aec_setup->init_flags = value;
			break;

		default:
			break;
	}

	ret = mailbox_media_aud_send_msg(EVENT_AUD_VOC_SET_AEC_PARA_REQ, aec_ctl);

	audio_intf_free(aec_ctl);
	return ret;
}

bk_err_t bk_aud_intf_get_aec_para(void)
{
	/*check aec status */
	if (aud_intf_info.voc_status == AUD_INTF_VOC_STA_NULL)
		return BK_ERR_AUD_INTF_STA;

	CHECK_AUD_INTF_BUSY_STA();
	return mailbox_media_aud_send_msg(EVENT_AUD_VOC_GET_AEC_PARA_REQ, NULL);
}

bk_err_t bk_aud_intf_mic_init(aud_intf_mic_setup_t *setup)
{
	bk_err_t ret = BK_OK;
	CHECK_AUD_INTF_BUSY_STA();

	aud_intf_info.mic_info.mic_chl = setup->mic_chl;
	aud_intf_info.mic_info.samp_rate = setup->samp_rate;
	aud_intf_info.mic_info.frame_size = setup->frame_size;
	aud_intf_info.mic_info.mic_gain = setup->mic_gain;
	aud_intf_info.mic_info.mic_type = setup->mic_type;

	ret = mailbox_media_aud_send_msg(EVENT_AUD_MIC_INIT_REQ, &aud_intf_info.mic_info);
	if (ret == BK_OK)
		aud_intf_info.mic_status = AUD_INTF_MIC_STA_IDLE;
	return ret;
}

bk_err_t bk_aud_intf_mic_deinit(void)
{
	bk_err_t ret = BK_OK;
	CHECK_AUD_INTF_BUSY_STA();
	ret = mailbox_media_aud_send_msg(EVENT_AUD_MIC_DEINIT_REQ, NULL);
	if (ret == BK_OK)
		aud_intf_info.mic_status = AUD_INTF_MIC_STA_NULL;
	return ret;
}

bk_err_t bk_aud_intf_mic_start(void)
{
	bk_err_t ret = BK_OK;
	CHECK_AUD_INTF_BUSY_STA();
	ret = mailbox_media_aud_send_msg(EVENT_AUD_MIC_START_REQ, NULL);
	if (ret == BK_OK)
		aud_intf_info.mic_status = AUD_INTF_MIC_STA_START;
	return ret;
}

bk_err_t bk_aud_intf_mic_pause(void)
{
	bk_err_t ret = BK_OK;
	CHECK_AUD_INTF_BUSY_STA();
	ret = mailbox_media_aud_send_msg(EVENT_AUD_MIC_PAUSE_REQ, NULL);
	if (ret == BK_OK)
		aud_intf_info.mic_status = AUD_INTF_MIC_STA_STOP;
	return ret;
}

bk_err_t bk_aud_intf_mic_stop(void)
{
	bk_err_t ret = BK_OK;
	CHECK_AUD_INTF_BUSY_STA();
	ret = mailbox_media_aud_send_msg(EVENT_AUD_MIC_STOP_REQ, NULL);
	if (ret == BK_OK)
		aud_intf_info.mic_status = AUD_INTF_MIC_STA_STOP;
	return ret;
}

static void aud_intf_spk_deconfig(void)
{
	aud_intf_info.spk_info.spk_chl = AUD_INTF_SPK_CHL_LEFT;
	aud_intf_info.spk_info.samp_rate = 8000;
	aud_intf_info.spk_info.frame_size = 0;
	aud_intf_info.spk_info.spk_gain = 0;
	ring_buffer_clear(aud_intf_info.spk_info.spk_rx_rb);
	if (aud_intf_info.spk_info.spk_rx_ring_buff) {
		audio_intf_free(aud_intf_info.spk_info.spk_rx_ring_buff);
		aud_intf_info.spk_info.spk_rx_ring_buff = NULL;
	}

	if (aud_intf_info.spk_info.spk_rx_rb) {
		audio_intf_free(aud_intf_info.spk_info.spk_rx_rb);
		aud_intf_info.spk_info.spk_rx_rb = NULL;
	}
}

bk_err_t bk_aud_intf_spk_init(aud_intf_spk_setup_t *setup)
{
	bk_err_t ret = BK_ERR_AUD_INTF_OK;
	bk_err_t err = BK_ERR_AUD_INTF_FAIL;
	//os_printf("bk_aud_intf_spk_init \r\n");
	CHECK_AUD_INTF_BUSY_STA();

	aud_intf_info.spk_info.spk_chl = setup->spk_chl;
	aud_intf_info.spk_info.samp_rate = setup->samp_rate;
	aud_intf_info.spk_info.frame_size = setup->frame_size;
	aud_intf_info.spk_info.spk_gain = setup->spk_gain;
	aud_intf_info.spk_info.work_mode = setup->work_mode;
	aud_intf_info.spk_info.spk_type = setup->spk_type;
	aud_intf_info.spk_info.fifo_frame_num = 4;			//default: 4

	aud_intf_info.spk_info.spk_rx_ring_buff = audio_intf_malloc(aud_intf_info.spk_info.frame_size * aud_intf_info.spk_info.fifo_frame_num);
	if (aud_intf_info.spk_info.spk_rx_ring_buff == NULL) {
		LOGE("%s, %d, malloc spk_rx_ring_buff fail \n", __func__, __LINE__);
		err = BK_ERR_AUD_INTF_MEMY;
		goto aud_intf_spk_init_exit;
	}

	aud_intf_info.spk_info.spk_rx_rb = audio_intf_malloc(sizeof(RingBufferContext));
	if (aud_intf_info.spk_info.spk_rx_rb == NULL) {
		LOGE("%s, %d, malloc spk_rx_rb fail \n", __func__, __LINE__);
		err = BK_ERR_AUD_INTF_MEMY;
		goto aud_intf_spk_init_exit;
	}

	ring_buffer_init(aud_intf_info.spk_info.spk_rx_rb, (uint8_t *)aud_intf_info.spk_info.spk_rx_ring_buff, aud_intf_info.spk_info.frame_size * aud_intf_info.spk_info.fifo_frame_num, DMA_ID_MAX, RB_DMA_TYPE_NULL);
	//ret = aud_intf_send_msg_with_sem(AUD_TRAS_DRV_SPK_INIT, &aud_intf_info.spk_info, BEKEN_WAIT_FOREVER);
	ret = mailbox_media_aud_send_msg(EVENT_AUD_SPK_INIT_REQ, &aud_intf_info.spk_info);
	if (ret != BK_ERR_AUD_INTF_OK) {
		err = ret;
		goto aud_intf_spk_init_exit;
	}

	aud_intf_info.spk_status = AUD_INTF_SPK_STA_IDLE;

	return BK_ERR_AUD_INTF_OK;

aud_intf_spk_init_exit:
	if (aud_intf_info.spk_info.spk_rx_ring_buff != NULL)
		audio_intf_free(aud_intf_info.spk_info.spk_rx_ring_buff);
	if (aud_intf_info.spk_info.spk_rx_rb != NULL)
		audio_intf_free(aud_intf_info.spk_info.spk_rx_rb);
	aud_intf_info.api_info.busy_status = false;
	return err;
}

bk_err_t bk_aud_intf_spk_deinit(void)
{
	bk_err_t ret = BK_OK;
	CHECK_AUD_INTF_BUSY_STA();
	ret = mailbox_media_aud_send_msg(EVENT_AUD_SPK_DEINIT_REQ, NULL);
	if (ret == BK_OK)
		aud_intf_info.spk_status = AUD_INTF_SPK_STA_NULL;

	aud_intf_spk_deconfig();

	return ret;
}

bk_err_t bk_aud_intf_spk_start(void)
{
	bk_err_t ret = BK_OK;
	CHECK_AUD_INTF_BUSY_STA();
	ret = mailbox_media_aud_send_msg(EVENT_AUD_SPK_START_REQ, NULL);
	if (ret == BK_OK)
		aud_intf_info.spk_status = AUD_INTF_SPK_STA_START;
	return ret;
}

bk_err_t bk_aud_intf_spk_pause(void)
{
	bk_err_t ret = BK_OK;
	CHECK_AUD_INTF_BUSY_STA();
	ret = mailbox_media_aud_send_msg(EVENT_AUD_SPK_PAUSE_REQ, NULL);
	if (ret == BK_OK)
		aud_intf_info.spk_status = AUD_INTF_SPK_STA_STOP;
	return ret;
}

bk_err_t bk_aud_intf_spk_stop(void)
{
	bk_err_t ret = BK_OK;
	CHECK_AUD_INTF_BUSY_STA();
	ret = mailbox_media_aud_send_msg(EVENT_AUD_SPK_STOP_REQ, NULL);
	if (ret == BK_OK)
		aud_intf_info.spk_status = AUD_INTF_SPK_STA_STOP;
	return ret;
}

static void aud_intf_voc_deconfig(void)
{
	/* audio deconfig */
	aud_intf_info.voc_info.samp_rate = 8000;	//default value
	aud_intf_info.voc_info.aud_setup.adc_gain = 0;
	aud_intf_info.voc_info.aud_setup.dac_gain = 0;
	aud_intf_info.voc_info.aud_setup.mic_frame_number = 0;
	aud_intf_info.voc_info.aud_setup.mic_samp_rate_points = 0;
	aud_intf_info.voc_info.aud_setup.speaker_frame_number = 0;
	aud_intf_info.voc_info.aud_setup.speaker_samp_rate_points = 0;

	/* aec deconfig */
	if (aud_intf_info.voc_info.aec_enable && aud_intf_info.voc_info.aec_setup) {
		audio_intf_free(aud_intf_info.voc_info.aec_setup);
	}
	aud_intf_info.voc_info.aec_setup = NULL;
	aud_intf_info.voc_info.aec_enable = false;

	/* tx deconfig */
	aud_intf_info.voc_info.tx_info.buff_length = 0;
	aud_intf_info.voc_info.tx_info.tx_buff_status = false;
	aud_intf_info.voc_info.tx_info.ping.busy_status = false;
	if (aud_intf_info.voc_info.tx_info.ping.buff_addr) {
		audio_intf_free(aud_intf_info.voc_info.tx_info.ping.buff_addr);
		aud_intf_info.voc_info.tx_info.ping.buff_addr = NULL;
	}

	aud_intf_info.voc_info.tx_info.pang.busy_status = false;
	if (aud_intf_info.voc_info.tx_info.pang.buff_addr) {
		audio_intf_free(aud_intf_info.voc_info.tx_info.pang.buff_addr);
		aud_intf_info.voc_info.tx_info.pang.buff_addr = NULL;
	}

	/* rx deconfig */
	aud_intf_info.voc_info.rx_info.rx_buff_status = false;
	if (aud_intf_info.voc_info.rx_info.decoder_ring_buff) {
		audio_intf_free(aud_intf_info.voc_info.rx_info.decoder_ring_buff);
		aud_intf_info.voc_info.rx_info.decoder_ring_buff = NULL;
	}

	if (aud_intf_info.voc_info.rx_info.decoder_rb) {
		audio_intf_free(aud_intf_info.voc_info.rx_info.decoder_rb);
		aud_intf_info.voc_info.rx_info.decoder_rb = NULL;
	}

#if CONFIG_AUD_INTF_SUPPORT_OPUS
	if (aud_intf_info.voc_info.rx_info.decoder_len_ring_buff) {
		audio_intf_free(aud_intf_info.voc_info.rx_info.decoder_len_ring_buff);
		aud_intf_info.voc_info.rx_info.decoder_len_ring_buff = NULL;
	}

	if (aud_intf_info.voc_info.rx_info.decoder_len_rbc) {
		audio_intf_free(aud_intf_info.voc_info.rx_info.decoder_len_rbc);
		aud_intf_info.voc_info.rx_info.decoder_len_rbc = NULL;
	}
#endif
	//os_memset(&aud_intf_info.voc_info.aud_codec_setup,0,sizeof(aud_intf_info.voc_info.aud_codec_setup));

	aud_intf_info.voc_info.rx_info.frame_num = 0;
	aud_intf_info.voc_info.rx_info.frame_size = 0;
	aud_intf_info.voc_info.rx_info.fifo_frame_num = 0;
	aud_intf_info.voc_info.rx_info.rx_buff_seq_tail = 0;
	aud_intf_info.voc_info.rx_info.aud_trs_read_seq = 0;
}

#ifdef CONFIG_AUD_RX_COUNT_DEBUG
static void aud_rx_lost_count_dump(void *param)
{
    uint32_t rx_size_temp = aud_rx_count.rx_size;
	aud_rx_count.rx_size = aud_rx_count.rx_size / 1024 / (AUD_RX_DEBUG_INTERVAL / 1000);

	LOGI("[AUD Rx] size: %d(Bytes), %uKB/s\n", rx_size_temp, aud_rx_count.rx_size);
	aud_rx_count.rx_size = 0;
}
#endif

#if CONFIG_AUD_INTF_SUPPORT_MP3
static uint16_t aud_mp3_dec_get_frame_sample_cnt(uint16_t dac_sample_rate, uint8_t ch_num)
{
    uint16_t dac_sample_cnt = MAX_NSAMP;
    switch (dac_sample_rate) //only support layer3
    {
        case 8000://MPEG2.5,frame sample cnt:576,72ms
        case 11025://MPEG2.5,frame sample cnt:576,52ms
        case 12000://MPEG2.5,frame sample cnt:576,48ms
        case 16000://MPEG2,frame sample cnt:576,36ms
        case 22050://MPEG2,frame sample cnt:576,26ms
        case 24000://MPEG2,frame sample cnt:576,24ms
        {
            break;
        }
        case 32000://MPEG1,frame sample cnt:1152,36ms
        case 44100://MPEG1,frame sample cnt:1152,26ms
        case 48000://MPEG1,frame sample cnt:1152,24ms
        {
            dac_sample_cnt = MAX_NSAMP*MAX_NGRAN;
            break;
        }
        default:
        {
            LOGE("%s,%d unsupported dac sample rate:%d\n",__func__, __LINE__,dac_sample_rate);
            break;
        }
    }

    return (dac_sample_cnt*ch_num);
}
static uint16_t aud_mp3_dec_get_max_input_size_in_byte(uint16_t dac_sample_rate, uint8_t ch_num)
{
    uint16_t max_input_samp_cnt = 0;
    switch (dac_sample_rate) //only support layer3
    {
        case 8000://MPEG2.5,frame sample cnt:576,72ms,max bitrate:160k
        {
            max_input_samp_cnt = 1440;
            break;
        }
        case 11025://MPEG2.5,frame sample cnt:576,52ms,max bitrate:160k
        {
            max_input_samp_cnt = 1044;
            break;
        }
        case 12000://MPEG2.5,frame sample cnt:576,48ms,max bitrate:160k
        {
            max_input_samp_cnt = 960;
            break;
        }
        case 16000://MPEG2,frame sample cnt:576,36ms,max bitrate:160k
        {
            max_input_samp_cnt = 720;
            break;
        }
        case 22050://MPEG2,frame sample cnt:576,26ms,max bitrate:160k
        {
            max_input_samp_cnt = 522;
            break;
        }
        case 24000://MPEG2,frame sample cnt:576,24ms,max bitrate:160k
        {
            max_input_samp_cnt = 480;
            break;
        }
        case 32000://MPEG1,frame sample cnt:1152,36ms,max bitrate:320k
        {
            max_input_samp_cnt = 1440;
            break;
        }
        case 44100://MPEG1,frame sample cnt:1152,26ms,max bitrate:320k
        {
            max_input_samp_cnt = 1044;
            break;
        }
        case 48000://MPEG1,frame sample cnt:1152,24ms,max bitrate:320k
        {
            max_input_samp_cnt = 960;
            break;
        }
        default:
        {
            LOGE("%s,%d unsupported dac sample rate:%d\n",__func__, __LINE__,dac_sample_rate);
            break;
        }
    }

    max_input_samp_cnt += 511;

    if(max_input_samp_cnt%4)
    {
        max_input_samp_cnt += (4 - (max_input_samp_cnt%4));
    }

    return (max_input_samp_cnt*ch_num);
}

#endif

void aud_codec_buf_len_cal(aud_codec_setup_input_t *para, aud_codec_setup_t *output)
{
    output->enc_input_size_in_byte = para->adc_samp_rate*para->enc_frame_len_in_ms/1000*para->enc_data_depth_in_byte;
    output->enc_output_size_in_byte =  para->enc_bitrate*para->enc_frame_len_in_ms/1000/8;

    if(para->enc_vbr_en)
    {
        output->enc_output_size_in_byte = output->enc_output_size_in_byte*2;
    }

    #if CONFIG_AUD_INTF_SUPPORT_MP3
    if(AUD_INTF_VOC_DATA_TYPE_MP3 == output->decoder_type)
    {
        //only support mp3 decoder
        output->dec_input_size_in_byte = aud_mp3_dec_get_max_input_size_in_byte(para->dac_samp_rate,1);
        output->dec_output_size_in_byte = aud_mp3_dec_get_frame_sample_cnt(para->dac_samp_rate,1)*para->dec_data_depth_in_byte;
        output->dec_frame_len_in_ms = aud_mp3_dec_get_frame_sample_cnt(para->dac_samp_rate,1)*1000/para->dac_samp_rate;
        para->dec_frame_len_in_ms = output->dec_frame_len_in_ms;
        LOGI("%s, %d, adc samp:%d,enc bitrate:%d,frame (ms):%d,data depth:%d,vbr:%d\n", __func__, __LINE__,
            output->adc_samp_rate,
            output->enc_bitrate,
            output->enc_frame_len_in_ms,
            output->enc_data_depth_in_byte,
            output->enc_vbr_en);

        LOGI("dac samp:%d,frame (ms):%d,data depth:%d\n",
            output->dac_samp_rate,
            output->dec_frame_len_in_ms,
            output->dec_data_depth_in_byte);

        LOGI("enc in s:%d,out s:%d,dec in s:%d,out s:%d\n",
            output->enc_input_size_in_byte,
            output->enc_output_size_in_byte,
            output->dec_input_size_in_byte,
            output->dec_output_size_in_byte);
    }
    else
    #endif
    {
        output->dec_input_size_in_byte = para->dec_bitrate*para->dec_frame_len_in_ms/1000/8;
        output->dec_output_size_in_byte = para->dac_samp_rate*para->dec_frame_len_in_ms/1000*para->dec_data_depth_in_byte;

        if(para->dec_vbr_en)
        {
            output->dec_input_size_in_byte = output->dec_input_size_in_byte*2;
        }

        LOGI("%s, %d, adc samp:%d,enc bitrate:%d,frame (ms):%d,data depth:%d,vbr:%d\n", __func__, __LINE__,
            output->adc_samp_rate,
            output->enc_bitrate,
            output->enc_frame_len_in_ms,
            output->enc_data_depth_in_byte,
            output->enc_vbr_en);

        LOGI("dac samp:%d,dec bitrate:%d,frame (ms):%d,data depth:%d,vbr:%d\n",
            output->dac_samp_rate,
            output->dec_bitrate,
            output->dec_frame_len_in_ms,
            output->dec_data_depth_in_byte,
            output->dec_vbr_en);

        LOGI("enc in s:%d,out s:%d,dec in s:%d,out s:%d\n",
            output->enc_input_size_in_byte,
            output->enc_output_size_in_byte,
            output->dec_input_size_in_byte,
            output->dec_output_size_in_byte);
    }
}

void bk_aud_intf_aud_codec_init(aud_codec_setup_input_t *input)
{
    aud_intf_info.voc_info.aud_codec_setup.adc_samp_rate = input->adc_samp_rate;
    aud_intf_info.voc_info.aud_codec_setup.enc_bitrate = input->enc_bitrate;
    aud_intf_info.voc_info.aud_codec_setup.enc_frame_len_in_ms = input->enc_frame_len_in_ms;
    aud_intf_info.voc_info.aud_codec_setup.enc_data_depth_in_byte = input->enc_data_depth_in_byte;
    aud_intf_info.voc_info.aud_codec_setup.enc_vbr_en = input->enc_vbr_en;
    aud_intf_info.voc_info.aud_codec_setup.dac_samp_rate = input->dac_samp_rate;
    aud_intf_info.voc_info.aud_codec_setup.dec_bitrate = input->dec_bitrate;
    aud_intf_info.voc_info.aud_codec_setup.dec_frame_len_in_ms = input->dec_frame_len_in_ms;
    aud_intf_info.voc_info.aud_codec_setup.dec_data_depth_in_byte = input->dec_data_depth_in_byte;
    aud_intf_info.voc_info.aud_codec_setup.dec_vbr_en = input->dec_vbr_en;
    aud_intf_info.voc_info.aud_codec_setup.encoder_type = input->encoder_type;
    aud_intf_info.voc_info.aud_codec_setup.decoder_type = input->decoder_type;
    aud_codec_buf_len_cal(input,&aud_intf_info.voc_info.aud_codec_setup);
}

uint32_t bk_aud_get_dec_input_size_in_byte(void)
{
    return aud_intf_info.voc_info.aud_codec_setup.dec_input_size_in_byte;
}

uint32_t bk_aud_get_dec_output_size_in_byte(void)
{
    return aud_intf_info.voc_info.aud_codec_setup.dec_output_size_in_byte;
}

uint32_t bk_aud_get_enc_input_size_in_byte(void)
{
    return aud_intf_info.voc_info.aud_codec_setup.enc_input_size_in_byte;
}

uint32_t bk_aud_get_enc_output_size_in_byte(void)
{
    return aud_intf_info.voc_info.aud_codec_setup.enc_output_size_in_byte;
}

uint32_t bk_aud_get_enc_frame_len_in_ms(void)
{
    return aud_intf_info.voc_info.aud_codec_setup.enc_frame_len_in_ms;
}

uint32_t bk_aud_get_dec_frame_len_in_ms(void)
{
    return aud_intf_info.voc_info.aud_codec_setup.dec_frame_len_in_ms;
}

uint32_t bk_aud_get_encoder_type(void)
{
    return aud_intf_info.voc_info.aud_codec_setup.encoder_type;
}

uint32_t bk_aud_get_decoder_type(void)
{
    return aud_intf_info.voc_info.aud_codec_setup.decoder_type;
}

uint32_t bk_aud_get_adc_sample_rate(void)
{
    return aud_intf_info.voc_info.aud_codec_setup.adc_samp_rate;
}

uint32_t bk_aud_get_dac_sample_rate(void)
{
    return aud_intf_info.voc_info.aud_codec_setup.dac_samp_rate;
}

bk_err_t bk_aud_intf_voc_init(aud_intf_voc_setup_t setup)
{
	bk_err_t ret = BK_OK;
	bk_err_t err = BK_ERR_AUD_INTF_FAIL;
	CHECK_AUD_INTF_BUSY_STA();

#ifdef CONFIG_AUD_RX_COUNT_DEBUG
	if (aud_rx_count.timer.handle != NULL)
	{
		ret = rtos_deinit_timer(&aud_rx_count.timer);
		if (BK_OK != ret)
		{
			LOGE("%s, %d, deinit aud_tx_count time fail \n", __func__, __LINE__);
			err = BK_ERR_AUD_INTF_TIMER;
			goto aud_intf_voc_init_exit;
		}
		aud_rx_count.timer.handle = NULL;
	}

	aud_rx_count.rx_size = 0;

	ret = rtos_init_timer(&aud_rx_count.timer, AUD_RX_DEBUG_INTERVAL, aud_rx_lost_count_dump, NULL);
	if (ret != BK_OK) {
		LOGE("%s, %d, rtos_init_timer fail \n", __func__, __LINE__);
		err = BK_ERR_AUD_INTF_TIMER;
		goto aud_intf_voc_init_exit;
	}
	ret = rtos_start_timer(&aud_rx_count.timer);
	if (ret != BK_OK) {
		LOGE("%s, %d, rtos_start_timer fail \n", __func__, __LINE__);
	}
	LOGI("%s, %d, start audio rx count timer complete \n", __func__, __LINE__);
#endif

	//aud_tras_drv_setup.aud_trs_mode = demo_setup.mode;
	aud_intf_info.voc_info.samp_rate = setup.samp_rate;
	aud_intf_info.voc_info.aec_enable = setup.aec_enable;
	aud_intf_info.voc_info.data_type = setup.data_type;
	/* audio config */
	aud_intf_info.voc_info.aud_setup.adc_gain = setup.mic_gain;	//default: 0x2d
	aud_intf_info.voc_info.aud_setup.dac_gain = setup.spk_gain;	//default: 0x2d
	aud_intf_info.voc_info.aud_setup.mic_samp_rate_points = setup.aud_codec_setup_input.adc_samp_rate*setup.aud_codec_setup_input.enc_frame_len_in_ms/1000;
    aud_intf_info.voc_info.aud_setup.speaker_samp_rate_points = setup.aud_codec_setup_input.dac_samp_rate*setup.aud_codec_setup_input.dec_frame_len_in_ms/1000;

#if CONFIG_AEC_ECHO_COLLECT_MODE_HARDWARE
	aud_intf_info.voc_info.aud_setup.mic_frame_number = 4;
#else
	aud_intf_info.voc_info.aud_setup.mic_frame_number = 2;
#endif
	aud_intf_info.voc_info.aud_setup.speaker_frame_number = 2;
	aud_intf_info.voc_info.aud_setup.spk_mode = setup.spk_mode;
	aud_intf_info.voc_info.mic_en = setup.mic_en;
	aud_intf_info.voc_info.spk_en = setup.spk_en;
	aud_intf_info.voc_info.mic_type = setup.mic_type;
	aud_intf_info.voc_info.spk_type = setup.spk_type;

#if CONFIG_AUD_INTF_SUPPORT_SPK_PLAY_FINISH_NOTIFY
    aud_intf_info.voc_info.spk_play_finish_notify = setup.spk_play_finish_notify;
    aud_intf_info.voc_info.spk_play_finish_usr_data = setup.spk_play_finish_usr_data;
#endif

#if CONFIG_AUD_INTF_SUPPORT_PROMPT_TONE_PLAY_FINISH_NOTIFY
    aud_intf_info.voc_info.prompt_tone_play_finish_notify = setup.prompt_tone_play_finish_notify;
    aud_intf_info.voc_info.prompt_tone_play_finish_usr_data = setup.prompt_tone_play_finish_usr_data;
#endif

	/* aec config */
	if (aud_intf_info.voc_info.aec_enable) {
		aud_intf_info.voc_info.aec_setup = audio_intf_malloc(sizeof(aec_config_t));
		if (aud_intf_info.voc_info.aec_setup == NULL) {
			LOGE("%s, %d, malloc aec_setup fail \n", __func__, __LINE__);
			err = BK_ERR_AUD_INTF_MEMY;
			goto aud_intf_voc_init_exit;
		}
		aud_intf_info.voc_info.aec_setup->init_flags = 0x1f;
		aud_intf_info.voc_info.aec_setup->mic_delay = 4;//25us?
		aud_intf_info.voc_info.aec_setup->ec_depth = setup.aec_cfg.ec_depth;
		aud_intf_info.voc_info.aec_setup->ref_scale = setup.aec_cfg.ref_scale;
		aud_intf_info.voc_info.aec_setup->TxRxThr = setup.aec_cfg.TxRxThr;
		aud_intf_info.voc_info.aec_setup->TxRxFlr = setup.aec_cfg.TxRxFlr;
		aud_intf_info.voc_info.aec_setup->voice_vol = 14;
		aud_intf_info.voc_info.aec_setup->ns_level = setup.aec_cfg.ns_level;
		aud_intf_info.voc_info.aec_setup->ns_para = setup.aec_cfg.ns_para;
		aud_intf_info.voc_info.aec_setup->drc = 15;
	} else {
		aud_intf_info.voc_info.aec_setup = NULL;
	}

		aud_intf_info.voc_info.vad_setup = audio_intf_malloc(sizeof(vad_config_t));
		if (aud_intf_info.voc_info.vad_setup == NULL) {
			LOGE("%s, %d, malloc vad_setup fail \n", __func__, __LINE__);
			err = BK_ERR_AUD_INTF_MEMY;
			goto aud_intf_voc_init_exit;
		}	
		aud_intf_info.voc_info.vad_setup->vad_start_threshold = setup.vad_cfg.vad_start_threshold;
		aud_intf_info.voc_info.vad_setup->vad_stop_threshold = setup.vad_cfg.vad_stop_threshold;
		aud_intf_info.voc_info.vad_setup->vad_silence_threshold = setup.vad_cfg.vad_silence_threshold;
	/* tx config */
	switch (aud_intf_info.voc_info.aud_codec_setup.encoder_type) {
		case AUD_INTF_VOC_DATA_TYPE_G711A:
		case AUD_INTF_VOC_DATA_TYPE_G711U:
		case AUD_INTF_VOC_DATA_TYPE_PCM:
			aud_intf_info.voc_info.tx_info.buff_length = aud_intf_info.voc_info.aud_codec_setup.enc_output_size_in_byte;
			break;

#if CONFIG_AUD_INTF_SUPPORT_G722
		case AUD_INTF_VOC_DATA_TYPE_G722:
			aud_intf_info.voc_info.tx_info.buff_length = aud_intf_info.voc_info.aud_codec_setup.enc_output_size_in_byte;
			break;
#endif
#if CONFIG_AUD_INTF_SUPPORT_OPUS
		case AUD_INTF_VOC_DATA_TYPE_OPUS:
			aud_intf_info.voc_info.tx_info.buff_length = aud_intf_info.voc_info.aud_codec_setup.enc_output_size_in_byte;
			break;
#endif             
		default:
			break;
	}
	aud_intf_info.voc_info.tx_info.ping.busy_status = false;
	aud_intf_info.voc_info.tx_info.ping.buff_addr = audio_intf_malloc(aud_intf_info.voc_info.tx_info.buff_length);
	if (aud_intf_info.voc_info.tx_info.ping.buff_addr == NULL) {
		LOGE("%s, %d, malloc pingpang buffer of tx fail \n", __func__, __LINE__);
		err = BK_ERR_AUD_INTF_MEMY;
		goto aud_intf_voc_init_exit;
	}
    LOGI("%s, %d, malloc aud_intf_info.voc_info.tx_info.ping.buff:%p, size:%d \r\n", __func__, __LINE__, 
         aud_intf_info.voc_info.tx_info.ping.buff_addr, 
         aud_intf_info.voc_info.tx_info.buff_length);
	aud_intf_info.voc_info.tx_info.pang.busy_status = false;
	aud_intf_info.voc_info.tx_info.pang.buff_addr = audio_intf_malloc(aud_intf_info.voc_info.tx_info.buff_length);
	if (aud_intf_info.voc_info.tx_info.pang.buff_addr == NULL) {
		LOGE("%s, %d, malloc pang buffer of tx fail \n", __func__, __LINE__);
		err = BK_ERR_AUD_INTF_MEMY;
		goto aud_intf_voc_init_exit;
	}
    LOGI("%s, %d, malloc aud_intf_info.voc_info.tx_info.pang.buff:%p, size:%d \r\n", __func__, __LINE__, 
         aud_intf_info.voc_info.tx_info.pang.buff_addr, 
         aud_intf_info.voc_info.tx_info.buff_length);
	aud_intf_info.voc_info.tx_info.tx_buff_status = true;

	/* rx config */
	aud_intf_info.voc_info.rx_info.aud_trs_read_seq = 0;
	switch (aud_intf_info.voc_info.aud_codec_setup.decoder_type) {
		case AUD_INTF_VOC_DATA_TYPE_G711A:
		case AUD_INTF_VOC_DATA_TYPE_G711U:
		case AUD_INTF_VOC_DATA_TYPE_PCM:
			aud_intf_info.voc_info.rx_info.frame_size = aud_intf_info.voc_info.aud_codec_setup.dec_input_size_in_byte;
			break;
#if CONFIG_AUD_INTF_SUPPORT_G722
		case AUD_INTF_VOC_DATA_TYPE_G722:
			aud_intf_info.voc_info.rx_info.frame_size = aud_intf_info.voc_info.aud_codec_setup.dec_input_size_in_byte;
			break;
#endif
#if CONFIG_AUD_INTF_SUPPORT_OPUS
		case AUD_INTF_VOC_DATA_TYPE_OPUS:
			aud_intf_info.voc_info.rx_info.frame_size = aud_intf_info.voc_info.aud_codec_setup.dec_input_size_in_byte;
			break;
#endif
#if CONFIG_AUD_INTF_SUPPORT_MP3
		case AUD_INTF_VOC_DATA_TYPE_MP3:
			aud_intf_info.voc_info.rx_info.frame_size = aud_intf_info.voc_info.aud_codec_setup.dec_input_size_in_byte;
			break;
#endif  
		default:
			break;
	}
	aud_intf_info.voc_info.rx_info.frame_num = setup.frame_num;
	aud_intf_info.voc_info.rx_info.rx_buff_seq_tail = 0;
	aud_intf_info.voc_info.rx_info.fifo_frame_num = setup.fifo_frame_num;
	aud_intf_info.voc_info.rx_info.decoder_ring_buff = audio_intf_malloc(aud_intf_info.voc_info.rx_info.frame_size * aud_intf_info.voc_info.rx_info.frame_num + CONFIG_AUD_RING_BUFF_SAFE_INTERVAL);
	if (aud_intf_info.voc_info.rx_info.decoder_ring_buff == NULL) {
		LOGE("%s, %d, malloc decoder ring buffer of rx fail \n", __func__, __LINE__);
		err = BK_ERR_AUD_INTF_MEMY;
		goto aud_intf_voc_init_exit;
	}
	LOGI("%s, %d, malloc decoder_ring_buff:%p, size:%d \r\n", __func__, __LINE__, aud_intf_info.voc_info.rx_info.decoder_ring_buff, aud_intf_info.voc_info.rx_info.frame_size * aud_intf_info.voc_info.rx_info.frame_num);
	aud_intf_info.voc_info.rx_info.decoder_rb = audio_intf_malloc(sizeof(RingBufferContext));
	if (aud_intf_info.voc_info.rx_info.decoder_rb == NULL) {
		LOGE("%s, %d, malloc decoder_rb fail \n", __func__, __LINE__);
		err = BK_ERR_AUD_INTF_MEMY;
		goto aud_intf_voc_init_exit;
	}
	ring_buffer_init(aud_intf_info.voc_info.rx_info.decoder_rb, (uint8_t *)aud_intf_info.voc_info.rx_info.decoder_ring_buff, aud_intf_info.voc_info.rx_info.frame_size * aud_intf_info.voc_info.rx_info.frame_num + CONFIG_AUD_RING_BUFF_SAFE_INTERVAL, DMA_ID_MAX, RB_DMA_TYPE_NULL);
	aud_intf_info.voc_info.rx_info.rx_buff_status = true;

	LOGI("%s, %d, decoder_rb:%p \n", __func__, __LINE__, aud_intf_info.voc_info.rx_info.decoder_rb);

#if CONFIG_AUD_INTF_SUPPORT_OPUS
	aud_intf_info.voc_info.rx_info.decoder_len_ring_buff = audio_intf_malloc(sizeof(uint16) * aud_intf_info.voc_info.rx_info.frame_num + CONFIG_AUD_RING_BUFF_SAFE_INTERVAL);
	if (aud_intf_info.voc_info.rx_info.decoder_len_ring_buff == NULL) {
		LOGE("%s, %d, malloc decoder len ring buffer of rx fail \n", __func__, __LINE__);
		err = BK_ERR_AUD_INTF_MEMY;
		goto aud_intf_voc_init_exit;
	}
	LOGI("%s, %d, malloc decoder_len_ring_buff:%p, size:%d \r\n", __func__, __LINE__, aud_intf_info.voc_info.rx_info.decoder_len_ring_buff, sizeof(uint16) * aud_intf_info.voc_info.rx_info.frame_num);
	aud_intf_info.voc_info.rx_info.decoder_len_rbc = audio_intf_malloc(sizeof(RingBufferContext));
	if (aud_intf_info.voc_info.rx_info.decoder_len_rbc == NULL) {
		LOGE("%s, %d, malloc decoder_rb fail \n", __func__, __LINE__);
		err = BK_ERR_AUD_INTF_MEMY;
		goto aud_intf_voc_init_exit;
	}
	ring_buffer_init(aud_intf_info.voc_info.rx_info.decoder_len_rbc, (uint8_t *)aud_intf_info.voc_info.rx_info.decoder_len_ring_buff, sizeof(uint16) * aud_intf_info.voc_info.rx_info.frame_num + CONFIG_AUD_RING_BUFF_SAFE_INTERVAL, DMA_ID_MAX, RB_DMA_TYPE_NULL);

	LOGI("%s, %d, decoder_len_rbc:%p \n", __func__, __LINE__, aud_intf_info.voc_info.rx_info.decoder_len_rbc);
#endif

	aud_intf_info.voc_info.aud_tx_rb = aud_tras_get_tx_rb();
#if CONFIG_AUD_INTF_SUPPORT_OPUS
    aud_intf_info.voc_info.aud_tx_pkt_len_rb = aud_tras_get_tx_pkt_len_rb();
#endif
	audio_intf_debug_init();

	/* callback config */
//	aud_intf_info.voc_info.aud_tras_drv_voc_event_cb = aud_tras_drv_voc_event_cb_handle;

//	CHECK_AUD_INTF_BUSY_STA();
	ret = mailbox_media_aud_send_msg(EVENT_AUD_VOC_INIT_REQ, &aud_intf_info.voc_info);
	if (ret != BK_OK) {
		err = ret;
		LOGE("%s, %d, fail, err:%d \r\n", __func__, __LINE__, err);
		goto aud_intf_voc_init_exit;
	}

	aud_intf_info.voc_status = AUD_INTF_VOC_STA_IDLE;

	return BK_ERR_AUD_INTF_OK;

aud_intf_voc_init_exit:
#ifdef CONFIG_AUD_RX_COUNT_DEBUG
	if (aud_rx_count.timer.handle) {
		bk_err_t ret = rtos_stop_timer(&aud_rx_count.timer);
		if (ret != BK_OK) {
			LOGE("%s, %d, stop aud_rx_count timer fail \n", __func__, __LINE__);
		}
		ret = rtos_deinit_timer(&aud_rx_count.timer);
		if (ret != BK_OK) {
			LOGE("%s, %d, deinit aud_rx_count timer fail \n", __func__, __LINE__);
		}
		aud_rx_count.timer.handle = NULL;
		aud_rx_count.rx_size = 0;
	}
#endif

	if (aud_intf_info.voc_info.aec_setup != NULL) {
		audio_intf_free(aud_intf_info.voc_info.aec_setup);
		aud_intf_info.voc_info.aec_setup = NULL;
	}
	if (aud_intf_info.voc_info.vad_setup != NULL) {
		audio_intf_free(aud_intf_info.voc_info.vad_setup);
		aud_intf_info.voc_info.vad_setup = NULL;
	}
	if (aud_intf_info.voc_info.tx_info.ping.buff_addr != NULL) {
		audio_intf_free(aud_intf_info.voc_info.tx_info.ping.buff_addr);
		aud_intf_info.voc_info.tx_info.ping.buff_addr = NULL;
	}

	if (aud_intf_info.voc_info.tx_info.pang.buff_addr != NULL) {
		audio_intf_free(aud_intf_info.voc_info.tx_info.pang.buff_addr);
		aud_intf_info.voc_info.tx_info.pang.buff_addr = NULL;
	}

	if (aud_intf_info.voc_info.rx_info.decoder_ring_buff != NULL) {
		audio_intf_free(aud_intf_info.voc_info.rx_info.decoder_ring_buff);
		aud_intf_info.voc_info.rx_info.decoder_ring_buff = NULL;
	}

	if (aud_intf_info.voc_info.rx_info.decoder_rb != NULL) {
		audio_intf_free(aud_intf_info.voc_info.rx_info.decoder_rb);
		aud_intf_info.voc_info.rx_info.decoder_rb = NULL;
	}

#if CONFIG_AUD_INTF_SUPPORT_OPUS
	if (aud_intf_info.voc_info.rx_info.decoder_len_ring_buff != NULL) {
		audio_intf_free(aud_intf_info.voc_info.rx_info.decoder_len_ring_buff);
		aud_intf_info.voc_info.rx_info.decoder_len_ring_buff = NULL;
	}
    
	if (aud_intf_info.voc_info.rx_info.decoder_len_rbc != NULL) {
		audio_intf_free(aud_intf_info.voc_info.rx_info.decoder_len_rbc);
		aud_intf_info.voc_info.rx_info.decoder_len_rbc = NULL;
	}
#endif

	aud_intf_info.api_info.busy_status = false;
	return err;
}

bk_err_t bk_aud_intf_voc_deinit(void)
{
	bk_err_t ret = BK_OK;

	if (aud_intf_info.voc_status == AUD_INTF_VOC_STA_NULL) {
		LOGI("%s, %d, voice is alreay deinit \n", __func__, __LINE__);
		return BK_ERR_AUD_INTF_OK;
	}

	CHECK_AUD_INTF_BUSY_STA();
	ret = mailbox_media_aud_send_msg(EVENT_AUD_VOC_DEINIT_REQ, NULL);
	if (ret != BK_OK) {
		LOGE("%s, %d, fail, result: %d \n", __func__, __LINE__, ret);
	} else {
		aud_intf_voc_deconfig();
		aud_intf_info.voc_status = AUD_INTF_VOC_STA_NULL;
	}

#ifdef CONFIG_AUD_RX_COUNT_DEBUG
	if (aud_rx_count.timer.handle) {
		ret = rtos_stop_timer(&aud_rx_count.timer);
		if (ret != BK_OK) {
			LOGE("%s, %d, stop aud_rx_count timer fail \n", __func__, __LINE__);
		}
		ret = rtos_deinit_timer(&aud_rx_count.timer);
		if (ret != BK_OK) {
			LOGE("%s, %d, deinit aud_rx_count timer fail \n", __func__, __LINE__);
		}
		aud_rx_count.timer.handle = NULL;
		aud_rx_count.rx_size = 0;
	}
#endif

	return ret;
}

bk_err_t bk_aud_intf_voc_start(void)
{
	bk_err_t ret = BK_OK;

	switch (aud_intf_info.voc_status) {
		case AUD_INTF_VOC_STA_NULL:
		case AUD_INTF_VOC_STA_START:
			return BK_ERR_AUD_INTF_OK;

		case AUD_INTF_VOC_STA_IDLE:
		case AUD_INTF_VOC_STA_STOP:
			if (ring_buffer_get_fill_size(aud_intf_info.voc_info.rx_info.decoder_rb)/aud_intf_info.voc_info.rx_info.frame_size < aud_intf_info.voc_info.rx_info.fifo_frame_num) {
				uint8_t *temp_buff = NULL;
				uint32_t temp_size = aud_intf_info.voc_info.rx_info.frame_size * aud_intf_info.voc_info.rx_info.fifo_frame_num - ring_buffer_get_fill_size(aud_intf_info.voc_info.rx_info.decoder_rb);
				temp_buff = audio_intf_malloc(temp_size);
				if (temp_buff == NULL) {
					return BK_ERR_AUD_INTF_MEMY;
				} else {
					uint8_t skip_flag = 0;
					switch (aud_intf_info.voc_info.aud_codec_setup.decoder_type) {
						case AUD_INTF_VOC_DATA_TYPE_G711A:
							os_memset(temp_buff, 0xD5, temp_size);
							break;

						case AUD_INTF_VOC_DATA_TYPE_PCM:
							os_memset(temp_buff, 0x00, temp_size);
							break;

						case AUD_INTF_VOC_DATA_TYPE_G711U:
							os_memset(temp_buff, 0xFF, temp_size);
							break;

#if CONFIG_AUD_INTF_SUPPORT_G722
						case AUD_INTF_VOC_DATA_TYPE_G722:
                        {
                            uint32_t file_size = 0;
                            while(temp_size > file_size) {
                                if ((temp_size - file_size) >= 160) {
							        os_memcpy(temp_buff + file_size, g722_silence_data, 160);
                                    file_size += 160;
                                } else {
                                    os_memcpy(temp_buff + file_size, g722_silence_data, temp_size - file_size);
                                    file_size = temp_size;
                                }
                            }
                        }
							break;
#endif                        
#if CONFIG_AUD_INTF_SUPPORT_OPUS 
                        case AUD_INTF_VOC_DATA_TYPE_OPUS:
                        {
                            skip_flag = 1;
                            break;
                        }
#endif
#if CONFIG_AUD_INTF_SUPPORT_MP3
                        case AUD_INTF_VOC_DATA_TYPE_MP3:
                        {
                            skip_flag = 1;
                            break;
                        }
#endif
						default:
							break;
					}
					LOGE("bk_aud_intf_voc_start:status:%d,size:%d\n",aud_intf_info.voc_status,temp_size);
					if(!skip_flag)
					{
						aud_intf_voc_write_spk_data(temp_buff, temp_size);
					}
					audio_intf_free(temp_buff);
				}
			}
			break;

		default:
			return BK_ERR_AUD_INTF_FAIL;
	}

	CHECK_AUD_INTF_BUSY_STA();
	ret = mailbox_media_aud_send_msg(EVENT_AUD_VOC_START_REQ, NULL);
	if (ret != BK_OK) {
		LOGE("%s, %d, fail, result: %d \n", __func__, __LINE__, ret);
	} else {
		aud_intf_info.voc_status = AUD_INTF_VOC_STA_START;
	}

	return ret;
}

bk_err_t bk_aud_intf_voc_stop(void)
{
	bk_err_t ret = BK_OK;

	switch (aud_intf_info.voc_status) {
		case AUD_INTF_VOC_STA_NULL:
		case AUD_INTF_VOC_STA_IDLE:
		case AUD_INTF_VOC_STA_STOP:
			return BK_ERR_AUD_INTF_STA;
			break;

		case AUD_INTF_VOC_STA_START:
			break;

		default:
			return BK_ERR_AUD_INTF_FAIL;
	}

	CHECK_AUD_INTF_BUSY_STA();
	ret = mailbox_media_aud_send_msg(EVENT_AUD_VOC_STOP_REQ, NULL);
	if (ret != BK_OK) {
		LOGE("%s, %d, fail, result: %d \n", __func__, __LINE__, ret);
	} else {
		aud_intf_info.voc_status = AUD_INTF_VOC_STA_STOP;
	}

	return ret;
}

bk_err_t bk_aud_intf_voc_mic_ctrl(aud_intf_voc_mic_ctrl_t mic_en)
{
	bk_err_t ret = BK_OK;
	if (aud_intf_info.voc_info.mic_en == mic_en) {
		return BK_OK;
	}

	aud_intf_voc_mic_ctrl_t temp = aud_intf_info.voc_info.mic_en;
	aud_intf_info.voc_info.mic_en = mic_en;

	CHECK_AUD_INTF_BUSY_STA();
	ret = mailbox_media_aud_send_msg(EVENT_AUD_VOC_CTRL_MIC_REQ, (void *)aud_intf_info.voc_info.mic_en);
	if (ret != BK_OK)
		aud_intf_info.voc_info.mic_en = temp;

	return ret;
}

bk_err_t bk_aud_intf_voc_spk_ctrl(aud_intf_voc_spk_ctrl_t spk_en)
{
	bk_err_t ret = BK_OK;
	if (aud_intf_info.voc_info.spk_en == spk_en) {
		return BK_OK;
	}

	aud_intf_voc_spk_ctrl_t temp = aud_intf_info.voc_info.spk_en;
	aud_intf_info.voc_info.spk_en = spk_en;

	CHECK_AUD_INTF_BUSY_STA();
	ret = mailbox_media_aud_send_msg(EVENT_AUD_VOC_CTRL_SPK_REQ, (void *)aud_intf_info.voc_info.spk_en);
	if (ret != BK_OK)
		aud_intf_info.voc_info.spk_en = temp;

	return ret;
}

bk_err_t bk_aud_intf_voc_aec_ctrl(bool aec_en)
{
	bk_err_t ret = BK_OK;
	if (aud_intf_info.voc_info.aec_enable == aec_en) {
		return BK_OK;
	}

	bool temp = aud_intf_info.voc_info.aec_enable;
	aud_intf_info.voc_info.aec_enable = aec_en;

	CHECK_AUD_INTF_BUSY_STA();
	ret = mailbox_media_aud_send_msg(EVENT_AUD_VOC_CTRL_AEC_REQ, (void *)aud_intf_info.voc_info.aec_enable);
	if (ret != BK_OK)
		aud_intf_info.voc_info.aec_enable = temp;

	return ret;
}

bk_err_t bk_aud_intf_drv_init(aud_intf_drv_setup_t *setup)
{
	bk_err_t ret = BK_OK;
	bk_err_t err = BK_ERR_AUD_INTF_FAIL;

	bk_pm_module_vote_boot_cp1_ctrl(PM_BOOT_CP1_MODULE_NAME_AUDP_AUDIO, PM_POWER_MODULE_STATE_ON);
//	rtos_delay_milliseconds(100);

	if (aud_intf_info.drv_status != AUD_INTF_DRV_STA_NULL) {
		LOGI("%s, %d, aud_intf driver already init \n", __func__, __LINE__);
		return BK_ERR_AUD_INTF_OK;
	}

	/* save drv_info */
	aud_intf_info.drv_info.setup = *setup;
//	aud_intf_info.drv_info.aud_tras_drv_com_event_cb = aud_tras_drv_com_event_cb_handle;

	CHECK_AUD_INTF_BUSY_STA();

	/* init audio interface driver */
	LOGD("%s, %d, init aud_intf driver in CPU1 mode \n", __func__, __LINE__);
	ret = mailbox_media_aud_send_msg(EVENT_AUD_INIT_REQ, &aud_intf_info.drv_info);
	if (ret != BK_OK) {
		LOGE("%s, %d, init aud_intf driver fail \n", __func__, __LINE__);
		goto aud_intf_drv_init_exit;
	} else {
		aud_intf_info.drv_status = AUD_INTF_DRV_STA_IDLE;
	}

	return BK_ERR_AUD_INTF_OK;

aud_intf_drv_init_exit:
	LOGE("%s, %d, init aud_intf driver fail \n", __func__, __LINE__);

	return err;
}

bk_err_t bk_aud_intf_drv_deinit(void)
{
	bk_err_t ret = BK_OK;

	if (aud_intf_info.drv_status == AUD_INTF_DRV_STA_NULL) {
		LOGI("%s, %d, aud_intf already deinit \n", __func__, __LINE__);
		return BK_OK;
	}

	/* reset uac_auto_connect */
	aud_intf_info.uac_auto_connect = true;

	CHECK_AUD_INTF_BUSY_STA();

	ret = mailbox_media_aud_send_msg(EVENT_AUD_DEINIT_REQ, NULL);
	if (ret != BK_OK) {
		LOGE("%s, %d, deinit audio transfer fail \n", __func__, __LINE__);
		return BK_ERR_AUD_INTF_FAIL;
	}

	/* deinit semaphore used to  */

	aud_intf_info.drv_status = AUD_INTF_DRV_STA_NULL;

	bk_pm_module_vote_boot_cp1_ctrl(PM_BOOT_CP1_MODULE_NAME_AUDP_AUDIO, PM_POWER_MODULE_STATE_OFF);

	return BK_ERR_AUD_INTF_OK;
}

static bk_err_t aud_intf_voc_write_dec_data(uint8_t *dac_buff, uint32_t size)
{
    uint32_t write_size = 0;


    /* check aud_intf status */
    if (aud_intf_info.voc_status == AUD_INTF_VOC_STA_NULL)
        return BK_ERR_AUD_INTF_STA;

#if (CONFIG_CACHE_ENABLE)
	flush_all_dcache();
#endif

    if (ring_buffer_get_free_size(aud_intf_info.voc_info.rx_info.decoder_rb) >= size) {
        write_size = ring_buffer_write(aud_intf_info.voc_info.rx_info.decoder_rb, dac_buff, size);
        if (write_size != size) {
            LOGE("%s, %d, write decoder_ring_buff fail, size:%d \n", __func__, __LINE__, size);
            return BK_FAIL;
        }
        aud_intf_info.voc_info.rx_info.rx_buff_seq_tail += size/(aud_intf_info.voc_info.rx_info.frame_size);
    }
    else
    {
        //LOGE("write fail, decoder_ring_buff is full!\r\n");
        return BK_FAIL;
    }

    return BK_OK;
}

static bk_err_t aud_intf_voc_write_dec_data_opus(uint8_t *dac_buff, uint32_t size)
{
    #if CONFIG_AUD_INTF_SUPPORT_OPUS
    uint32_t write_size[2] = {0};
    uint16_t pkt_len = size;

    /* check aud_intf status */
    if (aud_intf_info.voc_status == AUD_INTF_VOC_STA_NULL)
        return BK_ERR_AUD_INTF_STA;

#if (CONFIG_CACHE_ENABLE)
    flush_all_dcache();
#endif

    if (ring_buffer_get_free_size(aud_intf_info.voc_info.rx_info.decoder_len_rbc) >= (sizeof(uint16_t)))
    {
        if (ring_buffer_get_free_size(aud_intf_info.voc_info.rx_info.decoder_rb) >= size)
        {
            write_size[0] = ring_buffer_write(aud_intf_info.voc_info.rx_info.decoder_rb, dac_buff, size);
            write_size[1] = ring_buffer_write(aud_intf_info.voc_info.rx_info.decoder_len_rbc, (uint8_t *)&pkt_len, sizeof(uint16_t));
            if ((write_size[0] != size) || (write_size[1] != sizeof(uint16_t))) 
            {
                LOGE("write decoder_ring_buff fail, size:%d \r\n", size);
                return BK_FAIL;
            }
            aud_intf_info.voc_info.rx_info.rx_buff_seq_tail++;
        }
        else
        {
            LOGE("write fail, decoder_ring_buff is full!\r\n");
            return BK_FAIL;
        }
    }
    else
    {
        LOGE("write fail, decoder_len_ring_buff is full!\r\n");
        return BK_FAIL;
    }

    return BK_OK;
    #else
    LOGE("%s,%d,OPUS codec is disabled!\r\n",__func__, __LINE__);
    return BK_FAIL;
    #endif
}


/* write speaker data in voice work mode */
static bk_err_t aud_intf_voc_write_spk_data(uint8_t *dac_buff, uint32_t size)
{
    if(AUD_INTF_VOC_DATA_TYPE_OPUS == bk_aud_get_decoder_type())
    {
        return aud_intf_voc_write_dec_data_opus(dac_buff, size);
    }
    else
    {
        return aud_intf_voc_write_dec_data(dac_buff, size);
    }
}

/* write speaker data in general work mode */
static bk_err_t aud_intf_genl_write_spk_data(uint8_t *dac_buff, uint32_t size)
{
	uint32_t write_size = 0;

	//LOGI("%s \n", __func__);

	if (aud_intf_info.spk_status == AUD_INTF_SPK_STA_START || aud_intf_info.spk_status == AUD_INTF_SPK_STA_IDLE) {
#if (CONFIG_CACHE_ENABLE)
		flush_all_dcache();
#endif
		if (ring_buffer_get_free_size(aud_intf_info.spk_info.spk_rx_rb) >= size) {
			write_size = ring_buffer_write(aud_intf_info.spk_info.spk_rx_rb, dac_buff, size);
			if (write_size != size) {
				LOGE("%s, %d, write spk_rx_ring_buff fail, write_size:%d \n", __func__, __LINE__, write_size);
				return BK_FAIL;
			}
		}
	}

	return BK_OK;
}

bk_err_t bk_aud_intf_write_spk_data(uint8_t *dac_buff, uint32_t size)
{
	bk_err_t ret = BK_OK;

	//LOGI("%s \n", __func__);

	switch (aud_intf_info.drv_info.setup.work_mode) {
		case AUD_INTF_WORK_MODE_GENERAL:
			ret = aud_intf_genl_write_spk_data(dac_buff, size);
			break;

		case AUD_INTF_WORK_MODE_VOICE:
#ifdef CONFIG_AUD_RX_COUNT_DEBUG
			aud_rx_count.rx_size += size;
#endif
			ret = aud_intf_voc_write_spk_data(dac_buff, size);
			break;

		default:
			ret = BK_FAIL;
			break;
	}

	return ret;
}


bk_err_t bk_aud_intf_voc_tx_debug(aud_intf_dump_data_callback dump_callback)
{
	if (aud_intf_info.voc_status == AUD_INTF_VOC_STA_NULL)
		return BK_ERR_AUD_INTF_STA;

	CHECK_AUD_INTF_BUSY_STA();
	return mailbox_media_aud_send_msg(EVENT_AUD_VOC_TX_DEBUG_REQ, dump_callback);
}

bk_err_t bk_aud_intf_voc_rx_debug(aud_intf_dump_data_callback dump_callback)
{
	if (aud_intf_info.voc_status == AUD_INTF_VOC_STA_NULL)
		return BK_ERR_AUD_INTF_STA;

	CHECK_AUD_INTF_BUSY_STA();
	return mailbox_media_aud_send_msg(EVENT_AUD_VOC_RX_DEBUG_REQ, dump_callback);
}

bk_err_t bk_aud_intf_voc_aec_debug(aud_intf_dump_data_callback dump_callback)
{
	if (aud_intf_info.voc_status == AUD_INTF_VOC_STA_NULL)
		return BK_ERR_AUD_INTF_STA;

	CHECK_AUD_INTF_BUSY_STA();
	return mailbox_media_aud_send_msg(EVENT_AUD_VOC_AEC_DEBUG_REQ, dump_callback);
}

#if CONFIG_AUD_INTF_SUPPORT_PROMPT_TONE
bk_err_t bk_aud_intf_voc_play_prompt_tone(prompt_tone_url_info_t *prompt_tone)
{
	if (aud_intf_info.voc_status == AUD_INTF_VOC_STA_NULL)
		return BK_ERR_AUD_INTF_STA;

	CHECK_AUD_INTF_BUSY_STA();
	return mailbox_media_aud_send_msg(EVENT_AUD_VOC_PLAY_PROMPT_TONE_REQ, (void *)prompt_tone);
}

bk_err_t bk_aud_intf_voc_stop_prompt_tone(void)
{
	if (aud_intf_info.voc_status == AUD_INTF_VOC_STA_NULL)
		return BK_ERR_AUD_INTF_STA;

	CHECK_AUD_INTF_BUSY_STA();
	return mailbox_media_aud_send_msg(EVENT_AUD_VOC_STOP_PROMPT_TONE_REQ, NULL);
}
#endif

#if CONFIG_AUD_INTF_SUPPORT_SPK_PLAY_FINISH_NOTIFY
bk_err_t bk_aud_intf_voc_write_spk_data_ctrl(bool en)
{
	if (aud_intf_info.voc_status == AUD_INTF_VOC_STA_NULL)
		return BK_ERR_AUD_INTF_STA;

	CHECK_AUD_INTF_BUSY_STA();
	return mailbox_media_aud_send_msg(EVENT_AUD_VOC_SET_WRITE_SPK_DATA_STATE_REQ, (void *)en);
}
#endif

#if CONFIG_AUD_INTF_SUPPORT_MULTIPLE_SPK_SOURCE_TYPE
bk_err_t bk_aud_intf_voc_init_audio_act(void)
{
    return msg_send_req_to_media_app_mailbox_sync(EVENT_AUDIO_ACT_INIT_REQ, 0, NULL);
}

bk_err_t bk_aud_intf_voc_deinit_audio_act(void)
{
    return msg_send_req_to_media_app_mailbox_sync(EVENT_AUDIO_ACT_DEINIT_REQ, 0, NULL);
}

bk_err_t bk_aud_intf_voc_write_multiple_spk_data(audio_write_multiple_spk_data_req_t *write_req)
{
    uint32_t write_len = 0;
    bk_err_t ret = BK_OK;

    ret = msg_send_req_to_media_app_mailbox_sync(EVENT_AUD_VOC_WRITE_MULTIPLE_SPK_SOURCE_DATA_REQ, (uint32_t)write_req, &write_len);
    if (ret != 0)
    {
        write_len = -1;
    }

    return write_len;
}

bk_err_t bk_aud_intf_voc_set_spk_source_type(aud_spk_source_info_t *source_info)
{
    return msg_send_req_to_media_app_mailbox_sync(EVENT_AUD_VOC_SET_SPK_SOURCE_REQ, (uint32_t)source_info, NULL);
}

bk_err_t bk_aud_intf_voc_get_spk_source_type(spk_source_type_t *source_type)
{
    return msg_send_req_to_media_app_mailbox_sync(EVENT_AUD_VOC_GET_SPK_SOURCE_REQ, 0, (uint32_t *)source_type);
}
#endif

static void aec_vad_status_set(int val)
{
    if(val == 1)
    {
        LOGI("------------vad start----------\r\n");
        aud_intf_aec_vad_flag = 1;//FLAG_VAD_START;
    }
    else if(val == 2)
    {
        LOGI("------------vad end----------\r\n");
        aud_intf_aec_vad_flag = 2;//FLAG_VAD_END;
    }
    else if(val == 3)
    {
        LOGI("------------silence----------\r\n");
        aud_intf_aec_silence_flag = 1;
    }
}

uint8_t bk_aud_intf_get_aec_vad_flag(void)
{
	return aud_intf_aec_vad_flag;
}

void bk_aud_intf_clear_aec_vad_flag(void)
{
    aud_intf_aec_vad_flag = 0;
}

uint8_t bk_aud_intf_get_aec_slience_flag(void)
{
    return aud_intf_aec_silence_flag;
}

void bk_aud_intf_clear_aec_slience_flag(void)
{
    aud_intf_aec_silence_flag = 0;
}
bk_err_t bk_aud_intf_set_vad_para(aud_intf_voc_vad_para_t vad_para, uint32_t value)
{
	bk_err_t ret = BK_OK;
	aud_intf_voc_vad_ctl_t *vad_ctl = NULL;

	if (aud_intf_info.voc_status == AUD_INTF_VOC_STA_NULL)
		return BK_ERR_AUD_INTF_STA;

	CHECK_AUD_INTF_BUSY_STA();

	vad_ctl = audio_intf_malloc(sizeof(aud_intf_voc_vad_ctl_t));
	if (vad_ctl == NULL) {
		aud_intf_info.api_info.busy_status = false;
		return BK_ERR_AUD_INTF_MEMY;
	}
	vad_ctl->op = vad_para;
	vad_ctl->value = value;


	switch (vad_para) {
		case AUD_INTF_VOC_VAD_START_THRESHOLD:
			aud_intf_info.voc_info.vad_setup->vad_start_threshold = vad_para;
			break;

		case AUD_INTF_VOC_VAD_STOP_THRESHOLD:
			aud_intf_info.voc_info.vad_setup->vad_stop_threshold = vad_para;
			break;

		case AUD_INTF_VOC_VAD_SILENCE_THRESHOLD:
			aud_intf_info.voc_info.vad_setup->vad_silence_threshold = vad_para;
			break;
		case AUD_INTF_VOC_VAD_ENG_THRESHOLD:
			aud_intf_info.voc_info.vad_setup->vad_eng_threshold = vad_para;
			break;

		default:
			break;
		}
		ret = mailbox_media_aud_send_msg(EVENT_AUD_VOC_SET_VAD_PARA_REQ, vad_ctl);

		audio_intf_free(vad_ctl);
		return ret;
}

bk_err_t bk_aud_intf_set_vad_enable(bool val)
{
	if (aud_intf_info.voc_status == AUD_INTF_VOC_STA_NULL)
		return BK_ERR_AUD_INTF_STA;

	CHECK_AUD_INTF_BUSY_STA();
	app_aud_para_t * cust_aud_para = get_app_aud_cust_para();
	cust_aud_para->aec_config_voice.vad_enable = val;
	return mailbox_media_aud_send_msg(EVENT_AUD_VOC_SET_VAD_ENABLE, (void *)val);
}
bk_err_t bk_aud_intf_audio_para_set(app_aud_para_t *aud_para_ptr)
{
	bk_err_t ret = BK_OK;
	bk_aud_debug_get_audpara(aud_para_ptr);
	ret = mailbox_media_aud_send_msg(EVENT_AUD_SET_AUD_PARA_REQ, aud_para_ptr);
	return ret;
}
bk_err_t bk_aud_intf_update_sys_config_para(app_aud_sys_config_t *aud_sys_config_ptr)
{
	bk_err_t ret = BK_OK;
	ret = mailbox_media_aud_send_msg(EVENT_AUD_UPDATE_SYS_CONFIG_PARA_REQ, aud_sys_config_ptr);
	return ret;
}

bk_err_t bk_aud_intf_update_dl_eq_para(app_eq_t *dl_eq_para_ptr)
{
	bk_err_t ret = BK_OK;
	ret = mailbox_media_aud_send_msg(EVENT_AUD_UPDATE_DL_EQ_PARA_REQ, dl_eq_para_ptr);
	return ret;
}

bk_err_t bk_aud_intf_update_aec_para(app_aud_aec_config_t *aec_para_ptr)
{
	bk_err_t ret = BK_OK;
	ret = mailbox_media_aud_send_msg(EVENT_AUD_UPDATE_AEC_PARA_REQ, aec_para_ptr);
	return ret;
}


void audio_intf_debug_init()
{
#if CONFIG_SYS_CPU0
	bk_aud_debug_register_update_dl_eq_para_cb(bk_aud_intf_update_dl_eq_para);
	bk_aud_debug_register_update_aec_config_cb(bk_aud_intf_update_aec_para);
	bk_aud_debug_register_update_sys_config_cb(bk_aud_intf_update_sys_config_para);
#endif
}

uint32_t bk_aud_intf_get_dec_rb_free_size(void)
{
    return ring_buffer_get_free_size(aud_intf_info.voc_info.rx_info.decoder_rb);
}

