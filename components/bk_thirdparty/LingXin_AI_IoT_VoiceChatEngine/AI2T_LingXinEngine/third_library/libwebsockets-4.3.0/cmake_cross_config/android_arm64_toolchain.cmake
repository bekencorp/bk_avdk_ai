#
# CMake Toolchain file for crosscompiling Android / aarch64
#
# This can be used when running cmake in the following way:
#  cd build/
#  cmake .. -DCMAKE_TOOLCHAIN_FILE=contrib/cross-aarch64-android.cmake
#



set(ANDROID_API_VER 24)
set(CMAKE_SYSTEM_VERSION 24) # 你可以根据需要设置 API 级别

set(NDK $ENV{ANDROID_NDK_HOME})
set(CMAKE_ANDROID_NDK $ENV{ANDROID_NDK_HOME}) # 替换为你的 NDK 路径

# set(NDK /opt/android/ndk/21.1.6352462/)
# set(PLATFORM android)
# set(CMAKE_SYSTEM_NAME Android)
# set(CMAKE_SYSTEM_NAME Linux)
set(ANDROID_NATIVE_API_LEVEL 24)

#根据实际修改
set(GCC_VER 9.4.0)
set(COMPILE_TOOL_PATH "${NDK}/toolchains/llvm/prebuilt/linux-x86_64/bin")

set(ANDROID_ABI arm64-v8a)
set(CMAKE_SYSTEM_PROCESSOR aarch64)
set(CROSS_SYSROOT "${NDK}/platforms/android-${ANDROID_API_VER}/arch-arm64")
set(CMAKE_C_COMPILER "${COMPILE_TOOL_PATH}/aarch64-linux-android${ANDROID_NATIVE_API_LEVEL}-clang")
set(CMAKE_CXX_COMPILER "${COMPILE_TOOL_PATH}/aarch64-linux-android${ANDROID_NATIVE_API_LEVEL}-clang++")
set(CMAKE_AR "${COMPILE_TOOL_PATH}/aarch64-linux-android-ar")
set(CMAKE_LINKER "${COMPILE_TOOL_PATH}/aarch64-linux-android-ld")
set(CMAKE_STAGING_PREFIX "${CROSS_SYSROOT}")
set(CMAKE_C_FLAGS "-DGCC_VER=${GCC_VER} -DARM64=1 -D__LP64__=1 -Os -g3 -fpie -mstrict-align -fPIC -ffunction-sections -fdata-sections -D__ANDROID_API__=${ANDROID_API_VER} -Wno-pointer-sign -Wall -Wno-unused-label" CACHE STRING "" FORCE)



set(CMAKE_FIND_ROOT_PATH "${CROSS_SYSROOT}")

# Adjust the default behavior of the FIND_XXX() commands:
# search programs in the host environment only.
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# Search headers and libraries in the target environment only.
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)


include(${CMAKE_CURRENT_LIST_DIR}/common_options.cmake)
# 兼容Android4.4
set(LWS_AVOID_SIGPIPE_IGN ON CACHE BOOL "")
set(LWS_HAVE_GETGRGID_R OFF CACHE BOOL "")
set(LWS_HAVE_GETGRNAM_R OFF CACHE BOOL "")
set(LWS_HAVE_GETIFADDRS OFF CACHE BOOL "")
set(LWS_BUILTIN_GETIFADDRS ON CACHE BOOL "")

