#### 非芯片适配场景，AI2T_LingXinEngine 下所有代码不可修改

### 目录结构

.
├── inc/ # 外部头文件（供开发者调用）
│ ├── func/ # 可直接调用的功能模块头文件
│ │ ├── asr.h # asr 功能
│ │ ├── chat_api.h # 对话模式 API
│ │ ├── lingxin_log.h # 日志 API
│ │ ├── llm_generate.h # LLM 功能（生文、生图）
│ │ └── tts.h # 文本转语音功能
│ │
│ └── system_adapter/ # 功能适配层头文件(（需要开发者适配实现）)
│ ├── audio_buffer_play.h # 音频播放适配
│ ├── audio_recorder.h # 录音功能适配
│ ├── chat_state_machine_event.h # 适配所需要的状态机相关功能
│ ├── lingxin_http.h # HTTP 适配
│ ├── lingxin_semaphore.h # 信号量适配
│ ├── lingxin_system_time.h # 系统时间适配
│ ├── lingxin_timer.h # 定时器适配
│ ├── lingxin_websocket.h # WebSocket 适配
│ └── local_audio_play.h # 本地音频播放适配
│
├── src/ # SDK 源码实现
│ ├── asr/ # ASR 实现
│ │ └── asr.c
│ │
│ ├── auth/ # 鉴权配置模块
│ │ └── auth_config.c
│ │
│ ├── cjson/ # JSON 解析模块
│ │ └── cJSON.c
│ │
│ ├── inc/ # 内部头文件
│ │ ├── chat_state_machine.h
│ │ ├── cJSON.h
│ │ ├── lingxin_auth_config.h
│ │ ├── lingxin_common.h
│ │ ├── lingxin_event_queue.h
│ │ ├── lingxin_json_util.h
│ │ ├── lingxin_tls_utils.h
│ │ ├── lingxin_version.h
│ │ ├── lingxin_voice_queue.h
│ │ ├── schedule_first_ws.h
│ │ ├── schedule_timer_manager.h
│ │ ├── schedule_ws_manager.h
│ │ ├── voice_chat_manager.h
│ │ └── voice_chat.h  
│ │
│ ├── llm/ # LLM 功能实现
│ │ ├── image_generate.c # 图像生成
│ │ └── text_generate.c # 文字生成
│ │
│ ├── net/ # 网络通信模块
│ │ ├── http/
│ │ │ └── lingxin_http_lws.c # HTTP 默认适配
│ │ └── websocket/
│ │ └── lingxin_websocket_lws.c # WebSocket 默认适配
│ │
│ ├── schedule/ # 定时任务功能实现
│ │ ├── schedule_protocol.c # 服务端交互协议处理
│ │ ├── schedule_timer_manager.c # 定时任务定时器处理
│ │ └── schedule_ws_manager.c # 定时任务初次处理
│ │
│ ├── tts/ # TTS 实现
│ │ └── tts.c
│ │
│ ├── utils/ # 工具类方法
│ │ ├── common.c
│ │ ├── event_queue.c # 消息队列
│ │ ├── json_util.c # JSON 解析工具
│ │ ├── lingxin_log.c # 日志工具
│ │ ├── tls_utils.c # TLS 相关工具
│ │ └── voice_queue.c # 语音队列管理
│ │
│ └── voice_chat/ # chat 功能实现
│ ├── chat_api.c # 对话 API 处理入口
│ ├── chat_protocol.c # 服务端交互协议处理
│ ├── chat_state_machine.c # chat 状态机逻辑
│ └── voice_chat_manager.c # chat 方法调用管理
├── cmake/ # CMake 编译相关
│ ├── cmake_cross_config/ # 交叉编译配置文件
│ │ ├── android_arm64_toolchain.cmake
│ │ ├── android_armeabiv7a_toolchain.cmake
│ │ ├── android_x86_toolchain.cmake
│ │ ├── linux_arm_toolchain.cmake
│ │ ├── linux_x86_64_toolchain.cmake
│ │ └── windows_x86_64_toolchain.cmake
│ └── cmake_build.sh # CMake 编译脚本
│
├── libs/ # 第三方静态库及头文件（可由宏控制是否依赖）
│ ├── libwebsockets_4.3.0/  
│ └── mbedtls_2.26.0/
│
├── third_library/ # 第三方开源库源码（用于构建 libs 中的静态库）
│ ├── libwebsockets-4.3.0/
│ └── mbedtls-2.26.0/
│
└── README.md # 项目说明文档
