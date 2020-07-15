//
// Created by jinhuangzheliu on 7/13/20.
//
#include "ServiceClient.h"

ServiceClient::ServiceClient(std::string& target_address) {
  // Initial the connection to the server and get the status.
  bool initial_status = ServiceClient::InitializeConnection(target_address);

  // If the initial_status is false means the connection is failure. STOP!
  if (!initial_status){
    std::cerr << "Connection Initialization Failure!" << std::endl;
    exit(1);
  }
}

bool ServiceClient::InitializeConnection(std::string& address) {
  // Create the channel by target address.
  const std::shared_ptr<Channel>& channel =
      grpc::CreateChannel(address, grpc::InsecureChannelCredentials());

  // Create a new Server stub by the channel.
  stub_ = DifferentialServer::NewStub(channel);

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
  if (status.ok() && reply.reply() == "12345678"){
    return true;
  } else {
    return false;
  }
}

DiffMsgReply ServiceClient::DefaultDifferentialService(DiffMsgRequest& diff_request)
  {
    // Initial the differential result.
    differentialservice::DiffMsgReply resultOfDifferService;

    // ClientContext define by gRPC.
    ClientContext context;

    // implement the DefaultDifferentialService in differential service.
    Status status = stub_->DefaultDifferentialService(
        &context, diff_request, &resultOfDifferService);


    // If the status is ok, return the message DiffMsgReply.
    // If not, set the error field by error message and clear the result field
    // then return the message DiffMsgReply.
    if (status.ok()) {
      return resultOfDifferService;
    } else {
      resultOfDifferService.set_error(status.error_message());
      resultOfDifferService.clear_result();
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return resultOfDifferService;
    }
  }

DiffMsgReply ServiceClient::DifferentialService(DiffMsgRequest& diff_request)
{
  // Initial the differential result.
  differentialservice::DiffMsgReply resultOfDifferService;

  // ClientContext define by gRPC.
  ClientContext context;

  // implement the DifferentialService in differential service.
  Status status = stub_->DifferentialService(
      &context, diff_request, &resultOfDifferService);

  // If the status is ok, return the message DiffMsgReply.
  // If not, set the error field by error message and clear the result field
  // then return the message DiffMsgReply.
  if (status.ok()) {
    return resultOfDifferService;
  } else {
    resultOfDifferService.set_error(status.error_message());
    resultOfDifferService.clear_result();
    std::cout << status.error_code() << ": " << status.error_message()
              << std::endl;
    return resultOfDifferService;
  }
}






