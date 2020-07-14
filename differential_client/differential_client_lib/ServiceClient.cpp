//
// Created by jinhuangzheliu on 7/13/20.
//
#include "ServiceClient.h"

ServiceClient::ServiceClient(const std::shared_ptr<Channel>& channel)
    : stub_(DifferentialServer::NewStub(channel)) {}

std::string ServiceClient::GetConnect(const std::string& msg) {
  // message sending to the server.
  MsgRequest request;
  request.set_request(msg);

  // Container for the data we expect from the server.
  MsgReply reply;

  // Context for the client.
  ClientContext context;

  // The actual RPC.
  Status status = stub_->GetConnect(&context, request, &reply);

  // Act upon its status.
  if (status.ok()) {
    return reply.reply();
  } else {
    std::cout << status.error_code() << ": " << status.error_message()
              << std::endl;
    return "RPC failed";
  }
}

void ServiceClient::messageWriter(employee* msg_1, employee* msg_2,
                                  differentialservice::log* log_msg)
{
  // two message serialized values.
  std::string firstMessageStr;
  std::string secondMessageStr;

  // Serialized to string
  msg_1->SerializeToString(&firstMessageStr);
  // set the messages.
  log_msg->set_message_1(firstMessageStr);

  msg_2->SerializeToString(&secondMessageStr);
  log_msg->set_message_2(secondMessageStr);

  // 3) Get the message Descriptor.
  const Descriptor* dsp = msg_1->GetDescriptor();
  log_msg->set_name_of_message_descriptor(dsp->name());

  // 4) Get the file descriptor proto.
  const FileDescriptor* fdsp = dsp->file();
  FileDescriptorProto fdspProto;
  fdsp->CopyTo(&fdspProto);

  // Serialized to string
  std::string fdspProtoStr;
  fdspProto.SerializeToString(&fdspProtoStr);

  // set the file descriptor proto and its dependency
  log_msg->add_file_descriptor_proto(fdspProtoStr);

  // Add the dependency of file descriptor
  for (int i = 0; i < fdsp->dependency_count(); i++) {
    FileDescriptorProto fdspProto_dep;
    std::string fdspProtoStr_dep;

    // Get the file descriptor of dependency
    const FileDescriptor* fdsp_dep = fdsp->dependency(i);

    // Get the file descriptor proto of dependency
    fdsp_dep->CopyTo(&fdspProto_dep);
    fdspProto_dep.SerializeToString(&fdspProtoStr_dep);

    // add dependency to log
    log_msg->add_file_descriptor_proto(fdspProtoStr_dep);
  }
}

void ServiceClient::blackListCriteria(differentialservice::log& log_message,
                                             std::vector<std::string>& field_list)
{
  differentialservice::ignoreCriteria* ignoreCriteria_ptr =
      log_message.mutable_user_ignore();
  ignoreCriteria_ptr->set_flag(
      differentialservice::ignoreCriteria_ignoreFlag_FLAG_IGNORE);

  for (auto i = field_list.begin(); i != field_list.end() ; ++i) {
    ignoreCriteria_ptr->add_ignore_fields_list(*i);
  }
}

void ServiceClient::whiteListCriteria(differentialservice::log& log_message,
                              std::vector<std::string>& field_list) {
  differentialservice::ignoreCriteria* ignoreCriteria_ptr =
      log_message.mutable_user_ignore();
  ignoreCriteria_ptr->set_flag(
      differentialservice::ignoreCriteria_ignoreFlag_FLAG_COMPARE);

  for (auto i = field_list.begin(); i != field_list.end() ; ++i) {
    ignoreCriteria_ptr->add_ignore_fields_list(*i);
  }
}

void ServiceClient::RegexCriteria(differentialservice::log* log_message,
                   std::string& regex) {
//    log_message->mutable_user_ignore()->set_regex("^mes.*ress$");
  log_message->mutable_user_ignore()->set_regex(regex);
}

void ServiceClient::treat_repeated_field_list_or_set(differentialservice::log& log_message,
                                      int flag, std::string& field_name){
  differentialservice::repeatedFieldTuple* tuple_ptr = log_message.add_repeated_field();
  // Set the treat as flag.
  if (flag == 0){
    tuple_ptr->set_flag(differentialservice::repeatedFieldTuple_treatAsFlag_FLAG_LIST);
  }
  else if (flag == 1){
    tuple_ptr->set_flag(differentialservice::repeatedFieldTuple_treatAsFlag_FLAG_SET);
  }
  // Set the field name.
  tuple_ptr->set_field_name(field_name);
}

void ServiceClient::treat_repeated_field_map(differentialservice::log& log_message,
                              std::string& field_name,
                              std::vector<std::string>& sub_field_name)
{
  differentialservice::mapvalueTuple* mapvalue_ptr =
      log_message.add_mapvaluecompare();
  mapvalue_ptr->set_name_of_repeated_field(field_name);

  for (auto i = sub_field_name.begin(); i != sub_field_name.end(); ++i) {
    mapvalue_ptr->add_name_of_sub_field(*i);
  }
}

void ServiceClient::setFractionAndMargin(differentialservice::log& log_message,
                          double fraction,
                          double margin)
{
  differentialservice::float_comparison* fracAndMar_ptr =
      log_message.mutable_fraction_margin();
  fracAndMar_ptr->set_fraction(fraction);
  fracAndMar_ptr->set_margin(margin);
}

std::string ServiceClient::DefaultDifferentialService( employee& message_1, employee& message_2,
                                          differentialservice::log& log_message)
  {
    // Initial the differential result instance.
    differentialservice::result resultOfDifferService;

    // ClientContext define by gRPC.
    ClientContext context;

    // implement the DefaultDifferentialService in differential service.
    Status status = stub_->DefaultDifferentialService(
        &context, log_message, &resultOfDifferService);

    // return the output
    if (status.ok()) {
      return resultOfDifferService.res();
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return "Differ test failed";
    }
  }

std::string ServiceClient::DifferentialService( employee& message_1, employee& message_2,
                                 differentialservice::log& log_message)
{
  differentialservice::result resultOfDifferService;

  ClientContext context;

  Status status = stub_->DifferentialService(
      &context, log_message, &resultOfDifferService);

  if (status.ok()) {
    return resultOfDifferService.res();
  } else {
    std::cout << status.error_code() << ": " << status.error_message()
              << std::endl;
    return "Differ test failed";
  }
}






