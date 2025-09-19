#!/bin/bash

echo "------------------libwebsockets BUILD START------------------"
# 初始化标志变量
CLEAN_OPTION=false
#windows、linux-arm、linux-x86_64
ENV_OPTION=windows
# 解析命令行参数
while [[ $# -gt 0 ]]; do
  key="$1"
  case $key in
  --clean)
    CLEAN_OPTION=true
    shift # 解析-c选项
    ;;
  --env=*)
    ENV_OPTION="${key#*=}"
    shift # 解析-env选项
    ;;
  esac
done

# 判断是否提供了-C选项
if [ "$CLEAN_OPTION" = true ]; then
  echo "Cleaning build directory..."
  rm -rf build
fi

# Check if the build directory exists, if not, create it
if [ ! -d "build" ]; then
  mkdir build
fi
cd build

if [ "$ENV_OPTION" = linux-x86_64 ]; then
  TOOLCHAIN_PATH=linux_x86_64_toolchain.cmake
elif [ "$ENV_OPTION" = android-arm64-v8a ]; then
  TOOLCHAIN_PATH=android_arm64_toolchain.cmake
elif [ "$ENV_OPTION" = android-armeabi-v7a ]; then
  TOOLCHAIN_PATH=android_armeabiv7a_toolchain.cmake
elif [ "$ENV_OPTION" = android-x86 ]; then
  TOOLCHAIN_PATH=android_x86_toolchain.cmake
elif [ "$ENV_OPTION" = windows ]; then
  TOOLCHAIN_PATH=windows_x86_64_toolchain.cmake
elif [ "$ENV_OPTION" = linux-arm ]; then
  TOOLCHAIN_PATH=linux_arm_toolchain.cmake
fi

cmake .. -G "Ninja" -DCMAKE_TOOLCHAIN_FILE=../cmake_cross_config/${TOOLCHAIN_PATH}
cmake --build . --config Release

echo "------------------libwebsockets BUILD END------------------"
