
set(NDK $ENV{ANDROID_NDK_HOME})
set(CMAKE_ANDROID_NDK $ENV{ANDROID_NDK_HOME}) # 替换为你的 NDK 路径

set(ANDROID_NATIVE_API_LEVEL 19)

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

set(LIBWEBSOCKETS "${CMAKE_SOURCE_DIR}/../libs/libwebsockets_4.3.0/android/${ANDROID_ABI}/libwebsockets.a")
set(LIBWEBSOCKETS_INCLUDE "${CMAKE_SOURCE_DIR}/../libs/libwebsockets_4.3.0/android/${ANDROID_ABI}/include")

set(MBEDTLS "${CMAKE_SOURCE_DIR}/../libs/mbedtls_2.26.0/android/${ANDROID_ABI}/libmbedcrypto.a")
set(MBEDTLS_INCLUDE "${CMAKE_SOURCE_DIR}/../libs/mbedtls_2.26.0/include")

set(EXTRA_LIBS ${LIBWEBSOCKETS} ${MBEDTLS} log)
set(EXTRA_LIBS_INCLUDE ${LIBWEBSOCKETS_INCLUDE} ${MBEDTLS_INCLUDE})

set(USE_LINGXIN_NET_LIB ON)
set(USE_LINGXIN_JSON ON)
