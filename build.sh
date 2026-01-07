#!/bin/bash
set -e  # 遇到错误立即退出

echo ">>> [Build] Starting build process..."

# 创建构建目录
mkdir -p build
cd build

# 运行 CMake 和 Make
cmake ..
make -j4  # 使用 4 线程并行编译

echo ">>> [Build] Success! Executables are in 'build/' directory."