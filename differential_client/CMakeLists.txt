cmake_minimum_required(VERSION 3.16)
project(Differential_client)

set(CMAKE_CXX_STANDARD 14)

set(SOURCE_FILES differential_client.cc)
add_executable(differential_client_cpp ${SOURCE_FILES})

add_library(
        ServiceClient_lib STATIC
        ServiceClient.h
        ServiceClient.cpp
)
target_link_libraries(differential_client_cpp ServiceClient_lib)

add_library(
        Client_util_lib STATIC
        Client_util.h
        Client_util.cpp
)
target_link_libraries(differential_client_cpp Client_util_lib)

# Add the libraries from differential_client_lib
include_directories(differential_client_lib)
add_subdirectory(differential_client_lib)
target_link_libraries(differential_client_cpp Differential_client_lib)

# Add the Google Test for unit testing
add_subdirectory(Google_tests)