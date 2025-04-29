set(incs .)
set(srcs)

list(APPEND srcs
	app_main.c
)

if (CONFIG_OVERRIDE_FLASH_PARTITION)
    list(APPEND srcs
        vendor_flash.c
    )
endif()

armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_init multimedia avdk_utils mbedtls)
