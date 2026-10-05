cmake_minimum_required(VERSION 3.10)
project(kvstore CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# NtyCo 协程库头文件和动态库
include_directories(${CMAKE_SOURCE_DIR}/NtyCo/core)
link_directories(${CMAKE_SOURCE_DIR}/NtyCo)
set(CMAKE_BUILD_RPATH "${CMAKE_SOURCE_DIR}/NtyCo")

# 主程序
add_executable(kvstore
    kvstore.cpp
    kvstore_mp.cpp
    kvstore_array.cpp
    kvstore_hash.cpp
    kvstore_rbtree.cpp
    kvstore_skiplist.cpp
    ntyco_entry.cpp
)

target_link_libraries(kvstore ntyco)

# 压测客户端
add_executable(testcase testcase.cpp)
