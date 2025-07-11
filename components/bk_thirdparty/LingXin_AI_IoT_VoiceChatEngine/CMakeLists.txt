set(incs)
set(srcs)

if (CONFIG_LINGXIN_AI_EN) 
list(APPEND incs
            .
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
)
endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_common lwip_intf_v2_1 bk_vfs mbedtls psa_mbedtls audio_engine multimedia avdk_utils)

if (CONFIG_LINGXIN_AI_EN)
get_filename_component(lib_name ${CMAKE_CURRENT_SOURCE_DIR}/AI2T_LingXinEngine/libAI2T_LingXinEngine.a NAME)
add_prebuilt_library(${lib_name} "${CMAKE_CURRENT_SOURCE_DIR}/AI2T_LingXinEngine/libAI2T_LingXinEngine.a")
target_link_libraries(${COMPONENT_LIB} INTERFACE ${lib_name})
endif()

