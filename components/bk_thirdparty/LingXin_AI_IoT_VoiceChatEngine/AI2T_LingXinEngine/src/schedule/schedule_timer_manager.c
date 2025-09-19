#include <stdlib.h>
#include <string.h>
#include "chat_state_machine.h"
#include "lingxin_timer.h"
#include "schedule_timer_manager.h"
#include "schedule_ws_manager.h"
#include "lingxin_log.h"

#define MAX_TIMER_TASKS 20

TimerTask timerTasks[MAX_TIMER_TASKS]; // 存储所有定时任务
int taskCount = 0;                     // 当前任务数量

static u32 timer_start_time = 0;
// static TimerTask *timer_task_info = NULL;  // 注释掉未使用的变量
// static u16 timer_task_id = NULL;  // 注释掉未使用的变量
static bool is_task_emit_error = false;  // 定时任务是否触发失败,是否接收到error消息
static int is_schedule_task_on = 0; // 定时任务是否开启
// 用户在开启NoVoice循环前注入的自定义action(关闭tts/asr等)
static ScheduleEmitEventListener novoice_user_command_listener = NULL;

void module_schedule_init()
{
    is_schedule_task_on = 1;
    initScheduleChat();
}

void setNoVoiceListner(void *listener)
{
    novoice_user_command_listener = listener;
}

void novoice_user_custom_listener(StateEvent event)
{
    switch (event)
    {
    case State_Event_NoVoice_TerminateEnd:
    {
        if (novoice_user_command_listener)
        {
            novoice_user_command_listener();
        }
        break;
    }
    default:
    {
        break;
    }
    }
}

void recieve_schedule_task_error()
{
    is_task_emit_error = true;
}

static void schedule_callback(char *taskId)
{
    lingxin_log_debug("用户自定义回调执行！,更新taskId：%s\n", taskId);
    // 每次触发都会更新最新触发的taskId
    initScheduleConfig(taskId);
    // TODO: 升级为usr_timeout_add的硬件定时器中只触发不执行
    state_machine_run_event(State_Event_NoVoice_Start);
}
static void inner_callback(TimerTask *timer_task_info)
{
    // 定时器触发后的处理逻辑
    u32 now = get_sys_time_sec();
    lingxin_log_debug("定时器触发！\n与设置时间相差: %d s\n", now - timer_task_info->startTime);
}

static void hw_timer_callback(void *priv)
{
    TimerTask *timer_task_info = (TimerTask *)priv;
    if (timer_task_info->taskId)
    {
        lingxin_log_debug("当前定时任务触发，删除当前定时任务，其中定时器id为%d，任务id为%s，倒计时为%d\n", timer_task_info->timerId, timer_task_info->taskId, timer_task_info->countdown);
    }
    else
    {
        lingxin_log_debug("当前定时任务id为空%d\n", timer_task_info->taskId);
    }
    if (timer_task_info->timerId == INVALID_TIMER_ID)
    {
        lingxin_log_debug("当前定时任务倒计时id为空\n");
    }
    else
    {
        delete_schedule_timer(timer_task_info->timerId); // 删除定时器
    }

    inner_callback(timer_task_info); // 先执行公共逻辑
    schedule_callback(timer_task_info->taskId);
    // if (timer_task_info->callback)
    // {
    //     logPrintf("用户自定义回调不为空%s", timer_task_info->callback);
    //     timer_task_info->callback(timer_task_info->taskId);
    // }
}

static bool setup_hw_timer(TimerTask *timerTask, TaskItem *task, int advance_connect_time)
{
    timerTask->timerId = create_schedule_timer(timerTask, hw_timer_callback, timerTask->countdown * 1000);
    if (timerTask->timerId == INVALID_TIMER_ID)
    {
        lingxin_log_debug("定时器任务添加失败！\n");
        // 如果 taskId 分配了内存，但注册失败，应立即释放
        if (timerTask->taskId)
        {
            free(timerTask->taskId);
            timerTask->taskId = NULL;
        }
        return false;
    }
    taskCount++;
    lingxin_log_debug("定时器任务添加成功！当前任务数量为%d，当前任务id为%s，当前定时器id为%d\n", taskCount, timerTask->taskId, timerTask->timerId);
    return true;
}

static void clearTimerTask()
{
    const int temp_task_count = taskCount;
    for (int i = 0; i < temp_task_count && i < MAX_TIMER_TASKS; i++)
    {
        if (timerTasks[i].timerId != 0)
        {
            delete_schedule_timer(timerTasks[i].timerId); // 删除已存在的定时器
        }
        // ✅ 释放 taskId 内存
        if (timerTasks[i].taskId)
        {
            free(timerTasks[i].taskId);
            timerTasks[i].taskId = NULL;
        }
        // 重置结构体内容
        timerTasks[i].timerId = 0;
        timerTasks[i].countdown = 0;
        taskCount--;
    }

    lingxin_log_debug("定时任务列表清空完成！当前任务数量为%d\n", taskCount);
}

// 函数调用前必须显式声明函数原型
static void updateTimerTaskList(TaskItem sync_task_list[], int task_num, int advance_connect_time)
{
    // 清空当前维护的计时器数组
    clearTimerTask();
    lingxin_log_debug("提前建联时间 %d\n", advance_connect_time);

    // 根据传入的任务列表重新设置定时器
    for (int i = 0; i < task_num && i < MAX_TIMER_TASKS; i++)
    {
        lingxin_log_debug("定时任务列表添加任务 %d,任务id为%s，倒计时为%d\n", i, sync_task_list[i].taskId, sync_task_list[i].countdown);
        TaskItem *task = &sync_task_list[i];
        TimerTask *timer = &timerTasks[i];
        timer->taskId = task->taskId ? strdup(task->taskId) : NULL;
        int countdownSec = task->countdown - advance_connect_time;
        timer->countdown = (countdownSec > 0) ? (u32)countdownSec : 0;

        // timerTask->callback = task->callback;
        timer->startTime = get_sys_time_sec();
        timer_start_time = timer->startTime;
    }
    // 基本属性设置完后再设置定时器
    for (int i = 0; i < task_num && i < MAX_TIMER_TASKS; i++)
    {
        TaskItem *task = &sync_task_list[i];
        TimerTask *timer = &timerTasks[i];
        setup_hw_timer(timer, task, advance_connect_time); // 设置硬件定时器
    }
}

void initTimerTaskList(char *scheduleStr)
{
    if (!is_schedule_task_on)
    {
        clearTimerTask();
        return;
    }
    // 控制在接收到error后的system_event之后才向状态机发送事件拉起voice循环
    if (is_task_emit_error)
    {
        state_machine_run_event(State_Event_NoVoice_Error);
        is_task_emit_error = false;
    }
    // 从scheduleStr中解析出定时任务列表
    ScheduleTaskList *taskList = (ScheduleTaskList *)calloc(1, sizeof(ScheduleTaskList));
    if (!taskList)
    {
        lingxin_log_error("malloc taskList failed");
        return;
    }
    if (parseScheduleTaskList(scheduleStr, taskList) == 0)
    {
        lingxin_log_debug("当前解析出定时任务个数为%d\n", taskList->taskCount);
        for (int i = 0; i < taskList->taskCount; i++)
        {
            lingxin_log_debug("当前任务【%d】，解析出的任务Id为%s，倒计时时间为: %d\n", i, taskList->tasks[i].taskId, taskList->tasks[i].countdown);
        }

        updateTimerTaskList(taskList->tasks, taskList->taskCount, taskList->advanceConnectTime);
        // 释放分配的内存
        for (int i = 0; i < taskList->taskCount; i++)
        {
            if (taskList->tasks[i].taskId)
            {
                free(taskList->tasks[i].taskId); // ✅ 释放每个 taskId
                taskList->tasks[i].taskId = NULL;
            }
        }
        free(taskList->tasks); // ✅ 释放数组本身
        taskList->tasks = NULL;
    }
    else
    {
        lingxin_log_debug("system_event消息解析失败");
    }
}
