//
// Created by jinhuangzheliu on 7/13/20.
//

#ifndef DIFFERENTIAL_CLIENT_DIFFERENTIAL_SERVICE_CLIENT_H
#define DIFFERENTIAL_CLIENT_DIFFERENTIAL_SERVICE_CLIENT_H

#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/message.h>
#include <grpcpp/grpcpp.h>

#include <iostream>
#include <memory>
#include <string>

#include "differential_client_lib/differential_service.grpc.pb.h"
#include "differential_client_lib/differential_test.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using grpc::StatusCode;

// using namespace from the differential service .proto file
using DifferentialService::ServerDifferential;
using DifferentialService::MsgRequest;
using DifferentialService::MsgReply;
using DifferentialService::DiffRequest;
using DifferentialService::DiffResponse;


class DifferentialServiceClient {
 public:
  // Constructor for client derived from the stub of ServerDifferential
  explicit DifferentialServiceClient();

  /*
   * Differential service. User can set their criteria to
   *                       1) Ignore specific fields,
   *                       2) Treat repeated field as list, set, or map.
   *                       3) set fraction or margin for double number.
   * [Input Args]:
   * diff_request: the object of differential request message.
   * diff_response: the pointer of the differential response message.
   * target_address: the address of gRPC service.
   * [Return]: StatusCode of the gRPC response.
   */
  StatusCode CompareInputMessages(const DiffRequest& diff_request,
                                  DiffResponse* diff_response,
                                  const std::string& target_address);

 private:
  std::unique_ptr<ServerDifferential::Stub> stub_;

  // Initial the connection to the server
  Status InitializeConnection(const std::string& target_address);
};

#endif  // DIFFERENTIAL_CLIENT_DIFFERENTIAL_SERVICE_CLIENT_H
