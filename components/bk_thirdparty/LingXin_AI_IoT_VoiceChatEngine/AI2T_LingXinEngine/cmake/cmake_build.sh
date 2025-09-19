echo "------------------SDK BUILD START------------------"

# 初始化标志变量
CLEAN_OPTION=false
ENV_OPTION=linux-x86_64
ENABLE_DAILY=false

# 解析命令行参数
while [ $# -gt 0 ]; do
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
  --daily)
    ENABLE_DAILY=true
    shift # 解析-daily选项
    ;;
  *)
    echo "Unknown option: $key"
    exit 1
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
else
  echo "Unknown option: $ENV_OPTION"
  exit 1
fi

if [ "$ENABLE_DAILY" = true ]; then
  cmake .. -G "Ninja" -DCMAKE_TOOLCHAIN_FILE=./cmake_cross_config/${TOOLCHAIN_PATH} -DENV_DAILY=ON
else
  cmake .. -G "Ninja" -DCMAKE_TOOLCHAIN_FILE=./cmake_cross_config/${TOOLCHAIN_PATH}
fi

cmake --build . --config Release

echo "------------------SDK BUILD END------------------"
