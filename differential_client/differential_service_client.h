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

  // Initial the connection to the server
  bool InitializeConnection();

  /*
   * Default message differential service.
   * [Input Args]: the object of differential request message.
   * [Return]: the object of differential reply message.
   */
  DiffResponse DefaultDifferentialService(const DiffRequest& diff_request);

  /*
   * Differential service. User can set their criteria to
   *                       1) Ignore specific fields,
   *                       2) Treat repeated field as list, set, or map.
   *                       3) set fraction or margin for double number.
   * [Input Args]: the object of differential request message.
   * [Return]: the object of differential reply message.
   */
  DiffResponse CompareInputMessages(const DiffRequest& diff_request);

 private:
  std::unique_ptr<ServerDifferential::Stub> stub_;
//  std::string target_address;
};

#endif  // DIFFERENTIAL_CLIENT_DIFFERENTIAL_SERVICE_CLIENT_H
