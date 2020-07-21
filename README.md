# gRPC-Differential-Service

Creating an open source gRPC based differential service to enable users to differentiate the differences between paired protocol buffer messages. By comparing the user inputs to find the abnormalities that may lead to unexpected behavior of a software system, this service has great potential use cases in debugging, testing, or monitoring software systems. The service is based on gRPC framework and MessageDifferencer libraries. 

## Prerequisites
Our differential service is established on the gRPC framework by C++ language. To implement our service,  you have to install the compiler for C++(e.g. gcc), the process building tool(e.g. cmake), and Protocol Buffer in the system.

You can follow the gRPC offical [reference](https://github.com/grpc/grpc "reference") to install all these prerequisites.

## Client-Side Process
After installing all prerequisites you need to download the folder
> differential_client
to you local grpc directory.


