project(Google_tests)
add_subdirectory(lib)
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

add_subdirectory(../../../.. ${CMAKE_CURRENT_BINARY_DIR}/grpc EXCLUDE_FROM_ALL)

# After using add_subdirectory, we can now use the grpc targets directly from
# this build.
set(_PROTOBUF_LIBPROTOBUF libprotobuf)
set(_REFLECTION grpc++_reflection)
#    if(CMAKE_CROSSCOMPILING)
#        find_program(_PROTOBUF_PROTOC protoc)
#    else()
#        set(_PROTOBUF_PROTOC $<TARGET_FILE:protobuf::protoc>)
#    endif()
    set(_GRPC_GRPCPP grpc++)
#    if(CMAKE_CROSSCOMPILING)
#        find_program(_GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)
#    else()
#        set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:grpc_cpp_plugin>)
#    endif()


# adding the Google_Tests_run target
add_executable(Google_Tests_run unit_test_diff.cpp)

# linking Google_Tests_run with DateConverter_lib which will be tested
target_link_libraries(Google_Tests_run differential_service_client_lib)
target_link_libraries(Google_Tests_run client_util_lib)
target_link_libraries(Google_Tests_run Differential_client_lib)


target_link_libraries(Google_Tests_run ${_REFLECTION})
target_link_libraries(Google_Tests_run ${_GRPC_GRPCPP})
target_link_libraries(Google_Tests_run ${_PROTOBUF_LIBPROTOBUF})

target_link_libraries(Google_Tests_run gtest gtest_main)