//
// Created by jinhuangzheliu on 7/13/20.
//
#include "differential_service_client.h"

DifferentialServiceClient::DifferentialServiceClient() {
}

bool DifferentialServiceClient::InitializeConnection() {
  const std::string& target_address = "0.0.0.0:50053";
  // Create the channel by target address.
  const std::shared_ptr<Channel>& channel =
      grpc::CreateChannel(target_address, grpc::InsecureChannelCredentials());

  // Create a new Server stub by the channel.
  stub_ = ServerDifferential::NewStub(channel);

  // Check the connection with Server.
  // 1) Create a request.
  MsgRequest request;
  request.set_request("5678");

  // 2) create container for the data we expect from the server.
  MsgReply reply;

  // 3) Context for the client.
  ClientContext context;

  // 4) The actual RPC.
  Status status = stub_->GetConnect(&context, request, &reply);

  // 5) Act upon its status.
  return status.ok() && reply.reply() == "12345678";
}

DiffResponse DifferentialServiceClient::CompareInputMessages(const DiffRequest& diff_request) {
  // Initial the differential result.
  DifferentialService::DiffResponse diff_response;

  size_t proto_max_size = 4194304;

  size_t proto_size = diff_request.ByteSizeLong();

  if (proto_size > proto_max_size) {
    diff_response.set_error("The size of request message larger than max(4194304).");
    return diff_response;
  }

  // ClientContext define by gRPC.
  ClientContext context;

  // implement the DifferentialService in differential service.
  Status status = stub_->CompareInputMessages(&context, diff_request, &diff_response);

  // If the status is ok, return the message DiffMsgReply.
  // If not, set the error field by error message and clear the result field
  // then return the message DiffMsgReply.
  if (status.ok()) {
    return diff_response;
  } else {
    diff_response.set_error(status.error_message());
    diff_response.clear_result();
    std::cout << status.error_code() << ": " << status.error_message()
              << std::endl;
    return diff_response;
  }
}
