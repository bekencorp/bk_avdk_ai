set(incs)
set(srcs)

if (CONFIG_LINGXIN_AI_EN) 
list(APPEND incs
            .
            adapter_LingXin/auth
            AI2T_LingXinEngine/inc/func
            AI2T_LingXinEngine/inc/system_adapter
            AI2T_LingXinEngine/src/inc
)

list(APPEND srcs
            adapter_LingXin/audio_buffer/audio_buffer_play_custom.c
            adapter_LingXin/auth/auth_config.c
            adapter_LingXin/local_audio/local_audio_play.c
            adapter_LingXin/net/websocket_custom.c
            adapter_LingXin/record/audio_record_custom.c
            adapter_LingXin/semaphore/lingxin_semaphore.c
            AI2T_LingXinEngine/src/cjson/cJSON.c
            AI2T_LingXinEngine/src/asr/asr.c
            AI2T_LingXinEngine/src/tts/tts.c
            AI2T_LingXinEngine/src/llm/image_generate.c
            AI2T_LingXinEngine/src/llm/text_generate.c
            AI2T_LingXinEngine/src/voice_chat/chat_protocol.c
            AI2T_LingXinEngine/src/voice_chat/chat_state_machine.c
            AI2T_LingXinEngine/src/voice_chat/voice_chat_manager.c
            AI2T_LingXinEngine/src/utils/common.c
            AI2T_LingXinEngine/src/utils/event_queue.c
            AI2T_LingXinEngine/src/utils/json_util.c
            AI2T_LingXinEngine/src/utils/tls_utils.c
            AI2T_LingXinEngine/src/utils/voice_queue.c
)
endif()
armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_common lwip_intf_v2_1 bk_vfs mbedtls psa_mbedtls audio_engine multimedia avdk_utils)

#if (CONFIG_LINGXIN_AI_EN)
#get_filename_component(lib_name ${CMAKE_CURRENT_SOURCE_DIR}/chat_base_sdk/lib/libAI_IOT_SDK.a NAME)
#add_prebuilt_library(${lib_name} "${CMAKE_CURRENT_SOURCE_DIR}/chat_base_sdk/lib/libAI_IOT_SDK.a")
#target_link_libraries(${COMPONENT_LIB} INTERFACE ${lib_name})
#endif()

