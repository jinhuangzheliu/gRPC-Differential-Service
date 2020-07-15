//
// Created by jinhuangzheliu on 7/13/20.
//

#ifndef DIFFERENTIAL_CLIENT_SERVICECLIENT_H
#define DIFFERENTIAL_CLIENT_SERVICECLIENT_H

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
using differentialservice::DifferentialServer;
using differentialservice::MsgRequest;
using differentialservice::MsgReply;
using differentialservice::DiffMsgRequest;
using differentialservice::DiffMsgReply;


class ServiceClient {
 public:
  // Constructor for client derived from the stub of DifferentialServer
  explicit ServiceClient(std::string& target_address);

  /*
   * Default message differential service.
   * [Input Args]: the object of differential request message.
   * [Return]: the object of differential reply message.
   */
  DiffMsgReply DefaultDifferentialService(DiffMsgRequest& diff_request);

  /*
   * Differential service. User can set their criteria to
   *                       1) Ignore specific fields,
   *                       2) Treat repeated field as list, set, or map.
   *                       3) set fraction or margin for double number.
   * [Input Args]: the object of differential request message.
   * [Return]: the object of differential reply message.
   */
  DiffMsgReply DifferentialService(DiffMsgRequest& diff_request);

 private:
  std::unique_ptr<DifferentialServer::Stub> stub_;

  // Initial the connection to the server
  bool InitializeConnection(std::string& target_address);
};

#endif  // DIFFERENTIAL_CLIENT_SERVICECLIENT_H
