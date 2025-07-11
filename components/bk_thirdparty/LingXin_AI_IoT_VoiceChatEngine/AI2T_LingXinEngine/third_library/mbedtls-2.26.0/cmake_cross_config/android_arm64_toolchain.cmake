
set(NDK $ENV{ANDROID_NDK_HOME})
set(CMAKE_ANDROID_NDK $ENV{ANDROID_NDK_HOME}) # 替换为你的 NDK 路径

set(ANDROID_NATIVE_API_LEVEL 21)

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

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -O2")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -O2")

set(CMAKE_FIND_ROOT_PATH "${CROSS_SYSROOT}")

# Adjust the default behavior of the FIND_XXX() commands:
# search programs in the host environment only.
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# Search headers and libraries in the target environment only.
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(ARCH ${ANDROID_ABI})

