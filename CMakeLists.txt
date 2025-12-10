cmake_minimum_required(VERSION 3.10)
project(bplus_sql)

set(CMAKE_CXX_STANDARD 23)

# Set global include directories for all targets
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Add subdirectories
add_subdirectory(src)
add_subdirectory(tests)