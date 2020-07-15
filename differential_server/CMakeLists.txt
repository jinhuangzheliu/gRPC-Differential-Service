cmake_minimum_required(VERSION 3.16)
project(Differential_service)

set(CMAKE_CXX_STANDARD 14)

set(SOURCE_FILES differential_server.cc)
add_executable(differential_server_cpp ${SOURCE_FILES})

# Add the libraries
include_directories(differential_server_lib)
add_subdirectory(differential_server_lib)
target_link_libraries(differential_server_cpp Differential_server_lib)
