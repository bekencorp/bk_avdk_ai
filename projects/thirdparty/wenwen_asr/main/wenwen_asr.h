// Copyright 2023-2024 Beken
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

#pragma once

#include <os/os.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	WENWEN_ASR_IDLE = 0,
	WENWEN_ASR_START,
	WENWEN_ASR_EXIT
} wenwen_asr_op_t;

typedef struct {
	wenwen_asr_op_t op;
	void *param;
} wenwen_asr_msg_t;


bk_err_t wenwen_asr_init(void);

bk_err_t wenwen_asr_deinit(void);

bk_err_t wenwen_asr_start(void);

bk_err_t wenwen_asr_stop(void);

#ifdef __cplusplus
}
#endif

