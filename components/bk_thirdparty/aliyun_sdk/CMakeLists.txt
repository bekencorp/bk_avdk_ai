set(incs)
set(srcs)

if (CONFIG_ALIYUN_MMI_EN) 
list(APPEND incs
            .
            include
            include/c_utils
)

list(APPEND srcs
            hal_util_time.c
            hal_util_random.c
            hal_util_storage.c
            hal_util_mem.c
            hal_util_mutex.c
)
endif()
armino_component_register(SRCS "${srcs}" INCLUDE_DIRS "${incs}" PRIV_REQUIRES bk_common lwip_intf_v2_1 bk_vfs mbedtls psa_mbedtls multimedia avdk_utils)

if (CONFIG_ALIYUN_MMI_EN)
	file(GLOB aliyun_libs ${CMAKE_CURRENT_SOURCE_DIR}/*.a)
	foreach(lib ${aliyun_libs})
		get_filename_component(lib_name ${lib} NAME)
		message(STATUS "aliyun iot sdk:${lib_name}")
		add_prebuilt_library(${lib_name} "${CMAKE_CURRENT_SOURCE_DIR}/${lib_name}")
		target_link_libraries(${COMPONENT_LIB} INTERFACE ${lib_name})
	endforeach()
endif()