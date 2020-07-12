cmake_minimum_required(VERSION 3.16)

project(Differential_service)

include_directories(${CMAKE_CURRENT_BINARY_DIR})

AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_BINARY_DIR} DIR_SRCS)

set(FILE_DESCRIPTOR ${DIR_SRCS})

add_executable(differential_client_cpp differential_client.cc)