# gRPC-Differential-Service

Creating an open source gRPC based differential service to enable users to differentiate the differences between paired protocol buffer messages. By comparing the user inputs to find the abnormalities that may lead to unexpected behavior of a software system, this service has great potential use cases in debugging, testing, or monitoring software systems. The service is based on gRPC framework and MessageDifferencer libraries. 

## Prerequisites
Our differential service is established on the gRPC framework by C++ language. To implement our service,  you have to install the compiler for C++(e.g. gcc), the process building tool(e.g. cmake), and Protocol Buffer in the system.

You can follow the gRPC offical [reference](https://github.com/grpc/grpc "reference") to install all these prerequisites.

## Proto file for differential service
The .proto file of the differential service saved in the 'protos' folder at the root of this repo. Download the 'differential_service.proto' to your local grpc directory.
>(YOUR_DIR/grpc/exmaples/proto/).

We support a testing protobuf message defined in "differential_test.proto" for testing our service. Download the 'differential_service.proto' to your local grpc directory.
>(YOUR_DIR/grpc/exmaples/proto/).


## Client-Side Process
Download the folder 'differential_client' to you local directory:

>(YOUR_DIR/grpc/example/cpp)

and execute the following commands under the differential_client directory.

> $ `protoc -I=../../protos --grpc_out=./differential_client_lib/ --plugin=protoc-gen-grpc=\`which grpc_cpp_plugin\` ../../protos/differential_test.proto ../../protos/differential_service.proto`

> $ protoc -I=../../protos --cpp_out=./differential_client_lib/ ../../protos/differential_test.proto ../../protos/differential_service.proto

## Server-Side Process
Download the folder 'differential_server' to you local directory:

>(YOUR_DIR/grpc/example/cpp)

and execute the following commands under the differential_server directory.

> $ protoc -I=../../protos --grpc_out=./differential_server_lib/ --plugin=protoc-gen-grpc='which grpc_cpp_plugin' ../../protos/differential_service.proto

> $ protoc -I=../../protos --cpp_out=./differential_server_lib/ ../../protos/differential_service.proto

## Test the Differential Service
Under the differential_server directory make the server-side process as follows:

> $ make

> $ ./differential_server_cpp

If the process running successfully the follow output will preseted in your terminal window:

> Server Listening on 0.0.0.0:50053

We support a default test program for testing the connection between the client and server in your system. From another termial, make the differential_client_cpp under the differenital_client directory.

> make

> $ ./differential_client_cpp

### Unit test for Differential Service
We perform a total 50 unit-tests for the differential service. You can leverage a IDE to implement these unit test. The unit test file locate at:

>(YOUR_DIR/grpc/example/cpp/differential_client/Google_tests/unit_test_diff.cpp)

![Unit_test](https://github.com/jinhuangzheliu/gRPC-Differential-Service/blob/master/unit_test_screenshot.png)



