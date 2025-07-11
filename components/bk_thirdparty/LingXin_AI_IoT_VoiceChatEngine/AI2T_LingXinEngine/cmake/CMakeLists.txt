cmake_minimum_required(VERSION 3.10)

# 设置项目名称、版本
project(AI2T_LINGXINEngine VERSION 1.2)

# 添加你的源码文件
file(GLOB_RECURSE SDK_SOURCES
    "${CMAKE_SOURCE_DIR}/../src/*.c"
)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0")

# 创建动态库
add_library(${PROJECT_NAME} SHARED ${SDK_SOURCES})

target_compile_options(${PROJECT_NAME} PRIVATE -g)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/../inc/func)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/../inc/system_adapter)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_SOURCE_DIR}/../src/inc)

if(DEFINED EXTRA_LIBS_INCLUDE)
    target_include_directories(${PROJECT_NAME} PRIVATE ${EXTRA_LIBS_INCLUDE})
endif()

if(DEFINED EXTRA_LIBS)
    target_link_libraries(${PROJECT_NAME} PRIVATE ${EXTRA_LIBS})
endif()

# 设置SDK中的宏控制

# 使用灵芯SDK日常环境
if(ENV_DAILY)
    target_compile_definitions(${PROJECT_NAME} PRIVATE ENV_DAILY)
endif()

# 使用灵芯SDK自带网络库
if(USE_LINGXIN_NET_LIB)
    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_LINGXIN_NET_LIB)
endif()
# 使用灵芯SDK自带JSON库
if(USE_LINGXIN_JSON)
    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_LINGXIN_JSON)
endif()

# 设置sdk名称、版本
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "${PROJECT_NAME}-${ARCH}-${PROJECT_VERSION}")
