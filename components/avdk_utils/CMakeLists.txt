set(incs include)
set(srcs
	src/uart_util.c
	src/count_util.c
	src/ring_buffer.c
)

if (CONFIG_FATFS)
	list(APPEND srcs
		src/tfcard_util.c
	)
endif()

if (CONFIG_DEBUG_DUMP)
	list(APPEND srcs
		src/debug_dump.c
	)
endif()

set(priv_req )


armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES "${priv_req}")
