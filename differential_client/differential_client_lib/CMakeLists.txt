project(Differential_client_lib)

set(HEADER_FILES
        differential_test.grpc.pb.h
        differential_test.pb.h
        differential_service.grpc.pb.h
        differential_service.pb.h
        )

set(SOURCE_FILES
        differential_test.grpc.pb.cc
        differential_test.pb.cc
        differential_service.grpc.pb.cc
        differential_service.pb.cc
        )

add_library(Differential_client_lib STATIC ${SOURCE_FILES} ${HEADER_FILES})