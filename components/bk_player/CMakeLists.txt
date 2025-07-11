set(incs . include)
set(srcs "")
set(sources "")
set(codecs "codecs/frame_info_list.c")
set(sinks "")


if (CONFIG_PLAYER_ENABLE_SOURCE_NET)
list(APPEND sources
	"sources/source_net.c"
	)
endif()

if (CONFIG_PLAYER_ENABLE_CODEC_MP3)
list(APPEND codecs
	"codecs/codec_mp3.c"
	)
endif()

if (CONFIG_PLAYER_ENABLE_SINK_ONBOARD_SPEAKER)
list(APPEND sinks
	"sinks/sink_onboard_speaker.c"
	"sinks/sink_uart.c"
	)
endif()

if (CONFIG_PLAYER_ENABLE_SINK_VOICE_SPEAKER)
list(APPEND sinks
	"sinks/sink_voice_speaker.c"
	)
endif()

if(CONFIG_PLAYER)
	list(APPEND incs .)
	list(APPEND srcs
		"core/bk_player.c"
		"core/player_osal.c"
		"core/source.c"
		"core/codec.c"
		"core/sink.c"
		"core/player_mem.c"
		${sources}
		${codecs}
		${sinks})
endif()

if (CONFIG_PLAYER_TEST)
list(APPEND srcs
	"cli/player_cli.c"
	)
endif()

armino_component_register(SRCS "${srcs}"
		INCLUDE_DIRS "${incs}"
		PRIV_REQUIRES bk_common bk_cli avdk_libs bk_vfs webclient lwip_intf_v2_1 psa_mbedtls multimedia avdk_utils
		)
