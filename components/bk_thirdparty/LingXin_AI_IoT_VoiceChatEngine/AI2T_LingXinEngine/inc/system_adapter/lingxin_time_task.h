#include <stdbool.h>

// 定时器：后台只存在一个定时器，如果已经有，则不允许重复创建
/** 
 * 创建定时器
 * 如果已经存在定时器，则返回 false
 * 如果不存在定时器，则创建一个10s定时器，返回 true
*/
bool init_lingxin_chat_timer(void *priv, void (*func)(void *priv));

/** 
 * 删除定时器
 * 如果定时器存在，则删除定时器，返回 true
 * 如果定时器不存在，则返回 false
*/
bool delete_lingxin_chat_timer();

/** 
 * 重置定时器时间
 * 如果定时器存在，则重置定时器时间，返回 true
 * 如果定时器不存在，则返回 false
*/
bool reset_lingxin_chat_timer_run();
