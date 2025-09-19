#ifndef SCHEDULE_CHAT_ENGINE_H
#define SCHEDULE_CHAT_ENGINE_H

#include <stdbool.h>

#define current_demo_version "0.0.6"

typedef void (*ErrorCallback)(const char *data);

void initScheduleChat();


#endif // SCHEDULE_CHAT_ENGINE_H