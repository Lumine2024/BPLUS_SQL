cmake_minimum_required(3.5)
project(bplus_sql)

add_executable(main src/main.cpp)

target_include_directories(main PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)