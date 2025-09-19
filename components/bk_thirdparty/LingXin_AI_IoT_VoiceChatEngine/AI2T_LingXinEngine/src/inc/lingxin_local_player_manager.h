#ifndef __LINGXIN_LOCAL_PLAYER_MANAGER_H__
#define __LINGXIN_LOCAL_PLAYER_MANAGER_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 设置音量
 * @param volume 音量
 */
void module_local_play_set_volume(int volume);

/**
 * 设置首次唤醒后播放的音频
 * @param audio_path 音频路径
 */
void module_local_play_set_welcome_audio_path(char *audio_path);
/**
 * 设置打断时播放的音频
 * @param audio_path 音频路径
 */
void module_local_play_set_terminate_audio_path(char *audio_path);
/**
 * 设置连续对话进入下一轮对话前播放的音频
 * @param audio_path 音频路径
 */
void module_local_play_set_continue_audio_path(char *audio_path);

/**
 * 播放首次唤醒后播放的音频
 */
void module_local_play_welcome_audio();
/**
 * 播放打断时播放的音频
 */
void module_local_play_terminate_audio();
/**
 * 播放连续对话进入下一轮对话前播放的音频
 */
void module_local_play_continue_audio();

#ifdef __cplusplus
}
#endif

#endif /* __LINGXIN_LOCAL_PLAYER_MANAGER_H__ */