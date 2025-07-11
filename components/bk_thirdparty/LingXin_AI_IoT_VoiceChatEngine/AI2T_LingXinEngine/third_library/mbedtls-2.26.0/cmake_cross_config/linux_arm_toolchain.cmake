set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(COMPILER_ROOT $ENV{ARM_LINUX_ROOT}/bin)

# 指定交叉编译器
set(CMAKE_C_COMPILER ${COMPILER_ROOT}/arm-linux-gnueabihf-gcc)
set(CMAKE_CXX_COMPILER ${COMPILER_ROOT}/arm-linux-gnueabihf-g++)
set(CMAKE_LINKER ${COMPILER_ROOT}/arm-linux-gnueabihf-ld)
set(CMAKE_STRIP ${COMPILER_ROOT}/arm-linux-gnueabihf-strip)
set(CMAKE_OBJCOPY ${COMPILER_ROOT}/arm-linux-gnueabihf-objcopy)


# 搜索库和头文件的策略
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# 追加新的编译标志
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -lpthread")

