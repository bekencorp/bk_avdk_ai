
set(incs .
		model
		include
	)

set(srcs)

if (CONFIG_WANSON_FOREIGN_ASR)

#armino_build_get_property(armino_soc ARMINO_SOC)

if ("${ARMINO_SOC}" STREQUAL "bk7256" OR "${ARMINO_SOC}" STREQUAL "bk7256_cp1")
	string(FIND ${CONFIG_TOOLCHAIN_PATH} "nds32le-elf-mculib-v5f" compiler_type)

	if (${compiler_type} STREQUAL "-1")
		set(WANSON_FOREIGN_ASR_LIB_DIR_NAME "nds32le-elf-mculib-v5")
	else()
		set(WANSON_FOREIGN_ASR_LIB_DIR_NAME "nds32le-elf-mculib-v5f")
	endif()

	list(APPEND incs
				include
				bk7256/${WANSON_FOREIGN_ASR_LIB_DIR_NAME}
	)
endif()

# if ("${ARMINO_SOC}" STREQUAL "bk7236" OR "${ARMINO_SOC}" STREQUAL "bk7258" OR "${ARMINO_SOC}" STREQUAL "bk7258_cp1")
if ("${ARMINO_SOC}" STREQUAL "bk7236" OR "${ARMINO_SOC}" STREQUAL "bk7258" OR "${ARMINO_SOC}" STREQUAL "bk7258_cp1" OR "${ARMINO_SOC}" STREQUAL "bk7258_cp2")
    list(APPEND incs
				include
                bk7258  
    )
endif()

endif()
 

armino_component_register(INCLUDE_DIRS "${incs}")

if ("${ARMINO_SOC}" STREQUAL "bk7236" OR "${ARMINO_SOC}" STREQUAL "bk7258" OR "${ARMINO_SOC}" STREQUAL "bk7258_cp1" OR "${ARMINO_SOC}" STREQUAL "bk7258_cp2")
if (CONFIG_WANSON_FOREIGN_ASR)
	get_filename_component(lib_name ${CMAKE_CURRENT_SOURCE_DIR}/bk7258/libasrengine.a NAME)
	add_prebuilt_library(${lib_name} "${CMAKE_CURRENT_SOURCE_DIR}/bk7258/libasrengine.a")
	target_link_libraries(${COMPONENT_LIB} INTERFACE ${lib_name})
	
	get_filename_component(lib_name ${CMAKE_CURRENT_SOURCE_DIR}/bk7258/libwanson_foreign.a NAME)
	add_prebuilt_library(${lib_name} "${CMAKE_CURRENT_SOURCE_DIR}/bk7258/libwanson_foreign.a")
	target_link_libraries(${COMPONENT_LIB} INTERFACE ${lib_name})
endif()
endif()
