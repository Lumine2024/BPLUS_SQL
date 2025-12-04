cmake_minimum_required(VERSION 3.10)
project(bplus_sql)

set(CMAKE_CXX_STANDARD 23)

add_executable(main src/main.cpp src/lru.cpp src/bplus_tree.cpp)

target_include_directories(main PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)