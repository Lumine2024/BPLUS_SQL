cmake_minimum_required(VERSION 3.10)
project(bplus_sql)

set(CMAKE_CXX_STANDARD 23)

add_executable(main src/main.cpp)
target_include_directories(main PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(pressure_test tests/pressure_test.cpp)
target_include_directories(pressure_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(always_insert tests/always_insert.cpp)
target_include_directories(always_insert PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(test_existing_tree tests/test_existing_tree.cpp)
target_include_directories(test_existing_tree PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(parse_commands tests/parse_commands.cpp)
target_include_directories(parse_commands PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)