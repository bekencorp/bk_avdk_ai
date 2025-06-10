set(incs include)
set(srcs src/media_service.c
)
set(priv_req bk_common)

list(APPEND priv_req multimedia)

list(APPEND incs bk_private)
list(APPEND srcs
	osi_wrapper/video_osi_wrapper.c
	osi_wrapper/audio_osi_wrapper.c
)

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}" REQUIRES avdk_libs media_utils bk_peripheral media_unit_test bk_vfs avdk)
