project(Differential_server_lib)

set(HEADER_FILES
        differential_service.grpc.pb.h
        differential_service.pb.h
        )

set(SOURCE_FILES
        differential_service.grpc.pb.cc
        differential_service.pb.cc
        )

add_library(Differential_server_lib STATIC ${SOURCE_FILES} ${HEADER_FILES})