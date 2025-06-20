set(incs)
set(srcs)

if (CONFIG_LINGXIN_AI_EN) 
list(APPEND incs
            .
            chat_base_sdk/include
            chat_include
)

list(APPEND srcs
            chat_custom_module/audio_buffer/audio_buffer_play_custom.c
			chat_custom_module/base_sdk_custom/websocket_custom.c
			chat_custom_module/record/audio_record_custom.c
			#chat_jieli_module/audio_buffer/audio_buffer_play.c
			#chat_jieli_module/local_audio/local_audio_play.c
			#chat_jieli_module/record/audio_recorder.c
			#chat_jieli_module/sdk_module/voice_chat_engine.c
			voice_chat_machine.c
)
endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_common lwip_intf_v2_1 bk_vfs mbedtls psa_mbedtls)

if (CONFIG_LINGXIN_AI_EN)
get_filename_component(lib_name ${CMAKE_CURRENT_SOURCE_DIR}/chat_base_sdk/lib/libAI_IOT_SDK.a NAME)
add_prebuilt_library(${lib_name} "${CMAKE_CURRENT_SOURCE_DIR}/chat_base_sdk/lib/libAI_IOT_SDK.a")
target_link_libraries(${COMPONENT_LIB} INTERFACE ${lib_name})
endif()

