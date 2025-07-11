// 此处用来申明当前条件编译 默认实现和自定义实现只使用一个即可

/**
 * 流式播放条件编译宏
 */
// 默认实现
#define  CONFIG_AUDIO_BUFFER_DEFAULT_ENABLE 1

// 自定义实现
// #define  CONFIG_AUDIO_BUFFER_CUSTOM_ENABLE 1


/**
 * 录音模块条件编译宏  默认实现和自定义实现只使用一个即可
*/
// 默认实现
#define  CONFIG_RECORD_DEFAULT_ENABLE 1

// 录音模块自定义
// #define  CONFIG_RECORD_CUSTOM_ENABLE 1



/**
 * 播放模块条件编译宏
*/
// 默认实现
#define  CONFIG_PLAY_DEFAULT_ENABLE 1

// #define  CONFIG_PLAY_CUSTOM_ENABLE


/**
 * websocket 信号量控制
*/
// 默认实现
#define  CONFIG_SEMAPHORE_DEFAULT_ENABLE 1
