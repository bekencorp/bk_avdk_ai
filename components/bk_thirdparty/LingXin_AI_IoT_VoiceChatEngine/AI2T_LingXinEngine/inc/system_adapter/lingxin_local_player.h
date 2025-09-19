#ifndef __LINGXIN_LOCAL_PLAYER_H__
#define __LINGXIN_LOCAL_PLAYER_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 本地播放器句柄
 */
typedef void *lingxin_local_player_t;

/**
 * 本地播放器回调函数
 * @param result 播放结果。0为播放成功；-1为播放失败
 */
typedef void (*lingxin_local_player_callback_t)(int result);

/**
 * 创建本地播放器
 * @return 本地播放器句柄
 */
lingxin_local_player_t lingxin_local_player_create();

/**
 * 定义播放参数的结构体
 */
typedef struct {
  // 音频路径
  char *audio_path;
  // 初始音量
  int initial_volume;
} lingxin_local_player_play_param_t;

/**
 * 播放本地音频
 * @param player 本地播放器句柄
 * @param param 播放参数结构体的指针
 * @param callback 播放回调函数
 */
void lingxin_local_player_play(lingxin_local_player_t player, lingxin_local_player_play_param_t *param, lingxin_local_player_callback_t callback);

/**
 * 设置播放的音量
 * @param player 本地播放器句柄
 * @param volume 音量
 */
void lingxin_local_player_set_volume(lingxin_local_player_t player, int volume);

/**
 * 销毁本地播放器
 * @param player 本地播放器句柄
 */
void lingxin_local_player_destory(lingxin_local_player_t player);

#ifdef __cplusplus
}
#endif

#endif // __LINGXIN_LOCAL_PLAYER_H__