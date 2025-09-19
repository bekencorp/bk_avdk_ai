
set(NDK $ENV{ANDROID_NDK_HOME})
set(CMAKE_ANDROID_NDK $ENV{ANDROID_NDK_HOME}) # 替换为你的 NDK 路径

set(ANDROID_NATIVE_API_LEVEL 21)

#根据实际修改
set(GCC_VER 9.4.0)
set(COMPILE_TOOL_PATH "${NDK}/toolchains/llvm/prebuilt/linux-x86_64/bin")

set(ANDROID_ABI x86)
set(CMAKE_SYSTEM_PROCESSOR i686)

set(CMAKE_C_COMPILER "${COMPILE_TOOL_PATH}/${CMAKE_SYSTEM_PROCESSOR}-linux-android${ANDROID_NATIVE_API_LEVEL}-clang")
set(CMAKE_CXX_COMPILER "${COMPILE_TOOL_PATH}/${CMAKE_SYSTEM_PROCESSOR}-linux-android${ANDROID_NATIVE_API_LEVEL}-clang++")
set(CMAKE_AR "${COMPILE_TOOL_PATH}/llvm-ar")
set(CMAKE_LINKER "${COMPILE_TOOL_PATH}/ld.lld")
set(CMAKE_RANLIB "${COMPILE_TOOL_PATH}/llvm-ranlib")
set(CMAKE_STRIP "${COMPILE_TOOL_PATH}/llvm-strip")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -O2")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -O2")


# Adjust the default behavior of the FIND_XXX() commands:
# search programs in the host environment only.
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# Search headers and libraries in the target environment only.
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(ARCH ${ANDROID_ABI})

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -Wall -Wno-unused-label")

include(${CMAKE_CURRENT_LIST_DIR}/common_options.cmake)
# 兼容Android4.4
set(LWS_AVOID_SIGPIPE_IGN ON)
set(LWS_HAVE_GETGRGID_R OFF)
set(LWS_HAVE_GETGRNAM OFF)
set(LWS_HAVE_GETIFADDRS OFF)
set(LWS_BUILTIN_GETIFADDRS ON)
