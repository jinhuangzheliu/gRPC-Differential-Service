# gRPC-Differential-Service
This is a gRPC based Differential Service

Client Side:
protoc -I=../../protos --grpc_out=./differential_client_lib/ --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` ../../protos/differential_test.proto ../../protos/differential_service.proto

protoc -I ../../protos --cpp_out=./differential_client_lib/ ../../protos/differential_test.proto ../../protos/differential_service.proto

Server Side:
protoc -I=../../protos --grpc_out=./differential_server_lib/ --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` ../../protos/differential_service.proto

protoc -I ../../protos --cpp_out=./differential_server_lib/ ../../protos/differential_service.proto
