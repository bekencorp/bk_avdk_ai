
set(CMAKE_SYSTEM_VERSION 21) # 你可以根据需要设置 API 级别

set(NDK $ENV{ANDROID_NDK_HOME})

set(ANDROID_NATIVE_API_LEVEL 21)

#根据实际修改
set(GCC_VER 9.4.0)
set(COMPILE_TOOL_PATH "${NDK}/toolchains/llvm/prebuilt/linux-x86_64/bin")

set(ANDROID_ABI armeabi-v7a)
set(CMAKE_SYSTEM_PROCESSOR armv7a)
set(CROSS_SYSROOT "${NDK}/platforms/android-${ANDROID_NATIVE_API_LEVEL}/arch-arm")
set(CMAKE_C_COMPILER "${COMPILE_TOOL_PATH}/armv7a-linux-androideabi${ANDROID_NATIVE_API_LEVEL}-clang")
set(CMAKE_CXX_COMPILER "${COMPILE_TOOL_PATH}/armv7a-linux-androideabi${ANDROID_NATIVE_API_LEVEL}-clang++")
set(CMAKE_AR "${COMPILE_TOOL_PATH}/arm-linux-androideabi-ar")
set(CMAKE_LINKER "${COMPILE_TOOL_PATH}/arm-linux-androideabi-ld")
set(CMAKE_STAGING_PREFIX "${CROSS_SYSROOT}")


set(CMAKE_FIND_ROOT_PATH "${CROSS_SYSROOT}")

# Adjust the default behavior of the FIND_XXX() commands:
# search programs in the host environment only.
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# Search headers and libraries in the target environment only.
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -Wall -Wno-unused-label")


include(${CMAKE_CURRENT_LIST_DIR}/common_options.cmake)
# 兼容Android4.4
set(LWS_AVOID_SIGPIPE_IGN ON CACHE BOOL "")
set(LWS_HAVE_GETGRGID_R OFF CACHE BOOL "")
set(LWS_HAVE_GETGRNAM_R OFF CACHE BOOL "")
set(LWS_HAVE_GETIFADDRS OFF CACHE BOOL "")
set(LWS_BUILTIN_GETIFADDRS ON CACHE BOOL "")
