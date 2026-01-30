set(incs)
set(srcs)


list(APPEND incs
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/osal
)

if (CONFIG_UNISOUND)
list(APPEND srcs
    unisound_if.c

)
endif()



armino_component_register(
    SRCS "${srcs}"
	INCLUDE_DIRS "${incs}"
	PRIV_REQUIRES include avdk_utils  
)

if (CONFIG_UNISOUND)

	get_filename_component(lib_name ${CMAKE_CURRENT_SOURCE_DIR}/libosal.a NAME)
	add_prebuilt_library(${lib_name} "${CMAKE_CURRENT_SOURCE_DIR}/libosal.a")
	target_link_libraries(${COMPONENT_LIB} INTERFACE ${lib_name})

	get_filename_component(lib_name ${CMAKE_CURRENT_SOURCE_DIR}/libais-lite-Ual_kws.a.a NAME)
	add_prebuilt_library(${lib_name} "${CMAKE_CURRENT_SOURCE_DIR}/libais-lite-Ual_kws.a")
	target_link_libraries(${COMPONENT_LIB} INTERFACE ${lib_name})

endif()

#if("${ARMINO_SOC}" STREQUAL "bk7236" OR "${ARMINO_SOC}" STREQUAL "bk7258" OR "${ARMINO_SOC}" STREQUAL "bk7258_cp0")
    if(CONFIG_UNISOUND_LICENSE)
        get_filename_component(lib_name ${CMAKE_CURRENT_SOURCE_DIR}/libais-lite-Ual_auth.a NAME)
        add_prebuilt_library(${lib_name} "${CMAKE_CURRENT_SOURCE_DIR}/libais-lite-Ual_auth.a")
        target_link_libraries(${COMPONENT_LIB} INTERFACE ${lib_name}) 
    endif()
#endif()