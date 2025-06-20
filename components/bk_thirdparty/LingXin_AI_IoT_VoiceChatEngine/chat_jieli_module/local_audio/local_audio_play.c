#include "../../chat_include/chat_module_config.h"
#include "server/audio_server.h"
#include "../../chat_include/local_audio_play.h"
#include "fs/fs.h"
#include "app_config.h"

#include "../../voice_chat_machine.h"

#ifdef CONFIG_PLAY_DEFAULT_ENABLE

typedef struct
{
    // bool playing;
    FILE *file;
    struct server *decServer;
    PlayFinishCallback playCallback;
} local_audio_play_hdl;

// static struct local_audio_play_hdl local_audio_handler;

// #define __this (&local_audio_handler)

// 停止播放
static int local_audio_play_dec_stop(local_audio_play_hdl *localAudioPlay)
{
    int err = 0;
    union audio_req req = {0};

    printf("local_audio_play_dec_stop begin\n");

    if (!localAudioPlay->file || !localAudioPlay->decServer)
    {
        return 0;
    }

    printf("local_audio_play_dec_stop real\n");

    req.dec.cmd = AUDIO_DEC_STOP;
    server_request(localAudioPlay->decServer, AUDIO_REQ_DEC, &req);

    if (localAudioPlay->file)
    {
        fclose(localAudioPlay->file);
    }
    if (!localAudioPlay->decServer)
    {
        return 0;
    }

    int argv[2];
    argv[0] = AUDIO_SERVER_EVENT_END;
    argv[1] = (int)localAudioPlay->file;
    server_event_handler_del(localAudioPlay->decServer, 2, argv);
    server_close(localAudioPlay->decServer);
    return 0;
}

// bool isLocalAudioPlaying()
// {
//     return __this->playing;
// }
// void closeLocalAudioPlay()
// {

// }

static void flashAudioDecServerEventHandler(void *priv, int argc, int *argv)
{
    local_audio_play_hdl *localAudioPlay = (local_audio_play_hdl *)priv;

    union audio_req r = {0};

    switch (argv[0])
    {
    case AUDIO_SERVER_EVENT_ERR:
        printf("flash_music_dec_server_event_handler: AUDIO_SERVER_EVENT_ERR\n");
    case AUDIO_SERVER_EVENT_END:
        printf("flash_music_dec_server_event_handler: AUDIO_SERVER_EVENT_END\n");
        local_audio_play_dec_stop(localAudioPlay);
        if (localAudioPlay->playCallback)
        {
            printf("local_audio_play_dec_stop has callback\n");

            localAudioPlay->playCallback();
        }
        free(localAudioPlay);

        break;
    default:
        break;
    }
}

int playFlashAudio(const char *path, PlayFinishCallback callback)
{
    // __this->playing = false;
    local_audio_play_hdl *localAudioPlay = (local_audio_play_hdl *)calloc(1, sizeof(local_audio_play_hdl));

    int err = 0;
    union audio_req req = {0};
    printf("playFlashAudio : %s\n", path);

    localAudioPlay->playCallback = callback;

    localAudioPlay->file = fopen(path, "r");
    if (!localAudioPlay->file)
    {
        free(localAudioPlay);
        return -ENOENT;
    }
    // if (!__this->dec_server)
    // {
    printf("playFlashAudio dec_server  null\n");

    localAudioPlay->decServer = server_open("audio_server", "dec");
    if (!localAudioPlay->decServer)
    {
        goto __err;
    }
    server_register_event_handler_to_task(localAudioPlay->decServer, localAudioPlay, flashAudioDecServerEventHandler, "app_core");
    // }
    req.dec.output_buf_len = 6 * 1024; // DEC_BUF_LEN;
    req.dec.sample_source = "dac";

    req.dec.cmd = AUDIO_DEC_OPEN;
    req.dec.volume = 80;
    req.dec.speedV = 130;
    // req.dec.sample_rate = 32000;
    // req.dec.channel = 1;

    req.dec.file = localAudioPlay->file;

    err = server_request(localAudioPlay->decServer, AUDIO_REQ_DEC, &req);
    if (err)
    {
        goto __err;
    }
    req.dec.cmd = AUDIO_DEC_START;
    // __this->playing = true;

    err = server_request(localAudioPlay->decServer, AUDIO_REQ_DEC, &req);
    if (err)
    {
        goto __err;
    }

    printf("playFlashAudio: suss\n");
    return 0;
__err:
    // __this->playing = false;

    if (localAudioPlay->decServer)
    {
        server_close(localAudioPlay->decServer);
    }
    if (localAudioPlay->file)
    {
        fclose(localAudioPlay->file);
    }

    printf("playFlashAudio: fail : %d\n", err);

    return err;
}

static void playDingDongAudioCallback()
{
    printf("%s 播放成功 State_Event_Play_DingDong",__func__);
    state_machine_run_event(State_Event_Play_DingDong);
}

int playDingDongAudio() 
{       
    playFlashAudio(CONFIG_VOICE_PROMPT_FILE_PATH "sdkRecStart.mp3", playDingDongAudioCallback);
}

#endif