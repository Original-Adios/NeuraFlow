cmake_minimum_required(VERSION 3.10)
project(NeuraFlow VERSION 1.0)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找依赖库 (ZeroMQ 和 PThread)
find_package(PkgConfig REQUIRED)
pkg_check_modules(ZMQ REQUIRED libzmq)
find_package(Threads REQUIRED)

# 包含头文件目录
include_directories(${CMAKE_SOURCE_DIR})

# ==========================================
# 1. 编译 Broker (系统大脑)
# ==========================================
add_executable(broker broker/broker.cpp)
target_link_libraries(broker ${ZMQ_LIBRARIES} Threads::Threads)

# ==========================================
# 2. 编译 Worker (LLM 业务单元)
# ==========================================
add_executable(worker_llm worker/worker_llm.cpp)
target_link_libraries(worker_llm ${ZMQ_LIBRARIES} Threads::Threads)

# ==========================================
# 3. 编译 Client (模拟用户)
# ==========================================
add_executable(client_simulate client/client_simulate.cpp)
target_link_libraries(client_simulate ${ZMQ_LIBRARIES} Threads::Threads)