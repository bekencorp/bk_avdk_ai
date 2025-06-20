# Changelog

## [1.9.5.6] - 20250523

### Added
- 无

### Changed
- 优化音频重传机制，增大jitter buffer最大抗丢包GAP时长，单下行丢包对抗>50%（开启jitter buffer能力情况下）
- SSDK内部timer轮询从60ms改为20ms，非弱网对话延迟优化50ms

### Removed
- 无

### Fixed
- 无

-----------------------------------------------------------------------------------------------------

## [1.9.5.5] - 20250402

### Added
- jitter buffer支持不同音频帧长度，通过jitter_buffer_per_pcm_frame_ms设置接收的音频帧长度，支持20、40、60ms三种配置项

### Changed
- 无

### Removed
- 无

### Fixed
- 无

-----------------------------------------------------------------------------------------------------

## [1.9.5.4] - 20250401

### Added
- stream message功能，请关注on_stream_message回调函数，接收Agent服务发送的消息
- 自定义内容加密功能，on_packet_input_hook和on_packet_output_hook接口，目前项目用不到，可以暂不关心
- audio收流 jitter buffer，请设置enable_audio_jitter_buffer为true，此时设置audio_codec_type不能设置为AUDIO_CODEC_DISABLED

### Changed
- 修改底层timer定时器频次，加速下行收流重传包频次
- 构建参数设置为-o3，提升音频编码器运行效率，降低CPU占用率
- 线程创建接口换成rtos_create_psram_thread，线程栈空间使用psram内存

### Removed
- 无

### Fixed
- 无

-----------------------------------------------------------------------------------------------------