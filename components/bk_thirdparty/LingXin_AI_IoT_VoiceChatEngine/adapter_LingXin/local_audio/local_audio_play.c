#include "chat_state_machine.h"


int playDingDongAudio()
{
    // LOGI("%s State_Event_Play_DingDong\r\n", __func__);
    state_machine_run_event(State_Event_Play_DingDong);
    return 1;
}