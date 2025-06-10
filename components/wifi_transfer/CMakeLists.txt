set(incs include)
set(srcs)

set(priv_req bk_common)

if (CONFIG_WIFI_TRANSFER AND CONFIG_SYS_CPU0)
	list(APPEND srcs
		src/wifi_transfer.c
		src/trans_list.c
		)
endif()

list(APPEND priv_req multimedia)


armino_component_register(SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	PRIV_REQUIRES "${priv_req}"
	)
 