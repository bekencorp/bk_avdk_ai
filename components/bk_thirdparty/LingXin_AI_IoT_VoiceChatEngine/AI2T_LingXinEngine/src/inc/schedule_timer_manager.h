#ifndef SCHEDULE_TIMER_MANAGER_H
#define SCHEDULE_TIMER_MANAGER_H
#ifdef __cplusplus
extern "C"
{
#endif
#include <stdint.h>
#include "chat_state_machine.h"
    //typedef uint16_t u16;
    //typedef uint32_t u32;
    #include <common/bk_typedef.h>
    typedef struct
    {
        char *taskId;  // 任务唯一ID
        u16 timerId;   // 定时器ID（由系统分配）
        u32 countdown; // 倒计时时间（单位：毫秒或秒）
        u32 startTime; // 当前任务设置时间
    } TimerTask;

    typedef struct
    {
        int countdown;
        char *taskId;
    } TaskItem;
    typedef struct
    {
        TaskItem *tasks;
        int taskCount;
        int advanceConnectTime;
    } ScheduleTaskList;

    typedef void (*ScheduleEmitEventListener)();
    void recieve_schedule_task_error();
    void initTimerTaskList(char *scheduleStr);
    void module_schedule_init();
    void setNoVoiceListner(void *listener);
    void novoice_user_custom_listener(StateEvent event);
	int parseScheduleTaskList(const char *jsonStr, ScheduleTaskList *outList);
#ifdef __cplusplus
}
#endif
#endif