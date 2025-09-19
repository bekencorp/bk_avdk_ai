#ifndef AI_IOT_SDK_SCHEDULE_CHAT_H
#define AI_IOT_SDK_SCHEDULE_CHAT_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stddef.h>

    typedef struct ScheduleChatHandler ScheduleChatHandler;

    typedef struct
    {
        char *taskId;
        char *requestId; // 请求ID
        char *instanceId;
    } VoiceChatExtraInfo;

    typedef void (*SystemEventListener)(ScheduleChatHandler *globalHandler, const char *data);

    typedef struct
    {
        bool showLog;
        const char *serverPath;
        const char *payload;
        const char *taskId;
        const char *appKey;
        const char *appId;
        const char *sn;
    } ScheduleChatConfig;

    char *startFirstScheduleConnect( ScheduleChatConfig *config, SystemEventListener listener);

    void scheduleWsDestroy(ScheduleChatHandler *handler);

#ifdef __cplusplus
}
#endif
#endif // AI_IOT_SDK_SCHEDULE_CHAT_H