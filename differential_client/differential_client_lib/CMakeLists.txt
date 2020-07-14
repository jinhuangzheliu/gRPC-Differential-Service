project(Differential_client_lib)

set(HEADER_FILES
        differential_client.grpc.pb.h
        differential_client.pb.h
        differential_service.grpc.pb.h
        differential_service.pb.h
        ServiceClient.h
        )

set(SOURCE_FILES
        differential_client.grpc.pb.cc
        differential_client.pb.cc
        differential_service.grpc.pb.cc
        differential_service.pb.cc
        ServiceClient.cpp
        )

add_library(Differential_client_lib STATIC ${SOURCE_FILES} ${HEADER_FILES})