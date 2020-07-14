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

#include "differential_service.grpc.pb.h"
#include "differential_client.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using google::protobuf::Descriptor;
using google::protobuf::DescriptorPool;
using google::protobuf::DescriptorProto;
using google::protobuf::FieldDescriptor;
using google::protobuf::FieldDescriptorProto;
using google::protobuf::FileDescriptor;
using google::protobuf::FileDescriptorProto;
using google::protobuf::Map;
using google::protobuf::Message;
using google::protobuf::MessageFactory;

// using namespace from the differential service .proto file
using differentialservice::DifferentialServer;
using differentialservice::MsgRequest;
using differentialservice::MsgReply;

// using namespace form the differential client .proto file.
using differential_client::company;
using differential_client::education_info;
using differential_client::dependent_info;
using differential_client::field_set;
using differential_client::employee;

class ServiceClient {
 public:
  // Constructor for Service client derived from the stub of DifferentialServer
  explicit ServiceClient(const std::shared_ptr<Channel>& channel);

  std::string GetConnect(const std::string& msg);

  static void messageWriter(employee* msg_1, employee* msg_2,
                            differentialservice::log* log_msg);

  static void blackListCriteria(differentialservice::log& log_message,
                                std::vector<std::string>& field_list);

  static void whiteListCriteria(differentialservice::log& log_message,
                                std::vector<std::string>& field_list);

  static void RegexCriteria(differentialservice::log* log_message,
                            std::string& regex);

  static void treat_repeated_field_list_or_set(differentialservice::log& log_message,
                                               int flag, std::string& field_name);

  static void treat_repeated_field_map(differentialservice::log& log_message,
                                       std::string& field_name,
                                       std::vector<std::string>& sub_field_name);

  static void setFractionAndMargin(differentialservice::log& log_message,
                                   double fraction,
                                   double margin);

  std::string DefaultDifferentialService( employee& message_1, employee& message_2,
                                          differentialservice::log& log_message);

  std::string DifferentialService( employee& message_1, employee& message_2,
                                   differentialservice::log& log_message);

 private:
  std::unique_ptr<DifferentialServer::Stub> stub_;
};

#endif  // DIFFERENTIAL_CLIENT_SERVICECLIENT_H
