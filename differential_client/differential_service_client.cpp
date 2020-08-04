//
// Created by jinhuangzheliu on 7/13/20.
//
#include "differential_service_client.h"

DifferentialServiceClient::DifferentialServiceClient() {}

Status DifferentialServiceClient::InitializeConnection(const std::string& connection_address) {
  // Create the channel by target address.
  const std::shared_ptr<Channel>& channel =
      grpc::CreateChannel(connection_address, grpc::InsecureChannelCredentials());

  // Create a new Server stub by the channel.
  stub_ = ServerDifferential::NewStub(channel);

  // Check the connection with Server.
  // 1) Create a request.
  MsgRequest request;

  // 2) create container for the data we expect from the server.
  MsgReply reply;

  // 3) Context for the client.
  ClientContext context;

  // 4) The actual RPC.
  Status status = stub_->GetConnect(&context, request, &reply);

  // 5) Act upon its status.
  return status;
}

StatusCode DifferentialServiceClient::CompareInputMessages(const DiffRequest& diff_request,
                                                           DiffResponse* diff_response,
                                                           const std::string& target_address) {
  // Initial the gRPC connection and check the status of the connection.
  Status initial_status = this->InitializeConnection(target_address);
  if (!initial_status.ok()){
    return initial_status.error_code();
  }

  // Set size limitation of the proto message
  size_t proto_max_size = 4194304;
  // Get the actual size of the proto message
  size_t proto_size = diff_request.ByteSizeLong();
  // Check the size of input message if exceed the size limitation return StatusCode.
  if (proto_size > proto_max_size) {
    return StatusCode::INVALID_ARGUMENT;
  }

  // ClientContext define by gRPC.
  ClientContext context;

  // implement the DifferentialService in differential service.
  Status status = stub_->CompareInputMessages(&context, diff_request, diff_response);

  // If the gRPC status isn't OK, write the error message to the diff response
  // and return the error code of the status.
  if (!status.ok()) {
    diff_response->set_error(status.error_message());
    return status.error_code();
  }

  return StatusCode::OK;
}
