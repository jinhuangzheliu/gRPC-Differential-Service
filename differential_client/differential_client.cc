#include <google/protobuf/compiler/importer.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/message.h>
#include <grpcpp/grpcpp.h>

#include <fstream>
#include <iostream>
#include <memory>
#include <string>

#include "differential_client_lib/differential_service.grpc.pb.h"
#include "differential_client_lib/differential_client.grpc.pb.h"
#include "differential_client_lib/ServiceClient.h"

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


int main(int argc, char* argv[]) {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  // Establish the gRPC channel with port number.
  std::string target_str;
  target_str = "0.0.0.0:50053";

  // Initial the service client instance.
  ServiceClient serviceClient(
      grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials())
      );

  /*
   *  Check the Service Connection
   */
  std::string user(" Established");
  std::string reply = serviceClient.GetConnect(user);
  std::cout << "Status received: " << reply << std::endl;

  /*
   *  Implement the Differential Service
   */
  employee message_first;
  employee message_second;
  differentialservice::log log_message;

  message_first.set_employ_id(01);
  message_first.set_fullname("Jin Huang");
  message_first.set_age(32);
  message_first.mutable_employer()->set_name("Google Inc.");
  message_first.mutable_employer()->set_occupation("Software Engineer");
  message_first.mutable_employer()->set_address("CA, US");
  message_first.add_areas("Google Ads.");
  message_first.add_areas("YouTube Ads.");
  message_first.add_areas("Click Ads.");
  message_first.add_areas("Search Ads.");
  differential_client::dependent_info* dependentInfo_ptr_1 = message_first.mutable_dependents();
  dependentInfo_ptr_1->add_name("Jeremy");
  dependentInfo_ptr_1->add_name("Zhe");
  dependentInfo_ptr_1->add_name("Jin");
  dependentInfo_ptr_1->add_name("June");
  dependentInfo_ptr_1->add_age(29);
  dependentInfo_ptr_1->add_age(30);
  dependentInfo_ptr_1->add_age(31);
  dependentInfo_ptr_1->add_age(26);
  education_info* edu_info_1 = message_first.add_education();
  edu_info_1->set_name("Wright State University");
  edu_info_1->set_degree("PhD");
  edu_info_1->set_major("Computer Science");
  edu_info_1->set_address("OH, US");

  message_first.set_floatpoint(100.0);


  message_second.set_employ_id(02);
  message_second.set_fullname("Zhe Liu");
  message_second.set_age(29);
  message_second.mutable_employer()->set_name("Alphabet Inc.");
  message_second.mutable_employer()->set_occupation("Research Scientist");
  message_second.mutable_employer()->set_address("CA, US");
  message_second.add_areas("Google Ads.");
  message_second.add_areas("YouTube Ads.");
  message_second.add_areas("Search Ads.");
  message_second.add_areas("Click Ads.");
  differential_client::dependent_info* dependentInfo_ptr_2 = message_second.mutable_dependents();
  dependentInfo_ptr_2->add_name("Zhe");
  dependentInfo_ptr_2->add_name("Jeremy");
  dependentInfo_ptr_2->add_name("June");
  dependentInfo_ptr_2->add_name("Jin");
  dependentInfo_ptr_2->add_age(29);
  dependentInfo_ptr_2->add_age(30);
  dependentInfo_ptr_2->add_age(26);
  dependentInfo_ptr_2->add_age(31);
  education_info* edu_info_2 = message_second.add_education();
  edu_info_2->set_name("Wright State University");
  edu_info_2->set_degree("Master of Science");
  edu_info_2->set_major("Computer Science and Engineering");
  edu_info_2->set_address("OH, US");

  message_second.set_floatpoint(109.9);


  // Write two input messages into service log.
  ServiceClient::messageWriter(&message_first, &message_second, &log_message);

  // compare two messages by default.
  std::string repeatedRes = serviceClient.DefaultDifferentialService(
      message_first, message_second, log_message);
  std::cout << "Message differencer result (Default): \n" << repeatedRes << std::endl;



  // Add the ignore field list.
  std::vector<std::string> ignore_list;
  ignore_list.push_back("differential_client.employee.employ_id");
  ignore_list.push_back("differential_client.employee.age");
  ignore_list.push_back("differential_client.dependent_info.age");
  ServiceClient::blackListCriteria(log_message, ignore_list);


  // Add the compare field list/
//  std::vector<std::string> compare_list;
//  ServiceClient::whiteListCriteria(log_message, compare_list);

  // set the repeated field comparison
  std::string field_1 = "areas";
  ServiceClient::treat_repeated_field_list_or_set(log_message, 1, field_1);
  std::string field_2 = "dependents.name";
  ServiceClient::treat_repeated_field_list_or_set(log_message, 1, field_2);
  std::string field_3 = "dependents.age";
  ServiceClient::treat_repeated_field_list_or_set(log_message, 0, field_3);


  // Set the map value for repeated filed compare.
  std::string map_field_name = "education";
  std::vector<std::string> sub_field_list;
  sub_field_list.push_back("name");
//  sub_field_list.push_back("degree");
  ServiceClient::treat_repeated_field_map(log_message, map_field_name, sub_field_list);

  // Set the fraction and margin for float number comparison
  ServiceClient::setFractionAndMargin(log_message, 0.01, 0.0);



  // compare two messages by default.
  std::string repeatedRes1 = serviceClient.DifferentialService(
      message_first, message_second, log_message);
  std::cout << "Message differencer result: \n" << repeatedRes1 << std::endl;



  google::protobuf::ShutdownProtobufLibrary();
  return 0;
}