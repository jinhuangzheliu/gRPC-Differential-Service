project(Differential_server_lib)

set(HEADER_FILES
        differential_server.grpc.pb.h
        differential_server.pb.h
        differential_service.grpc.pb.h
        differential_service.pb.h
        )

set(SOURCE_FILES
        differential_server.grpc.pb.cc
        differential_server.pb.cc
        differential_service.grpc.pb.cc
        differential_service.pb.cc
        )

add_library(Differential_server_lib STATIC ${SOURCE_FILES} ${HEADER_FILES})