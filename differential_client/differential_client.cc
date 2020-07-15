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
#include "differential_client_lib/differential_test.grpc.pb.h"
#include "ServiceClient.h"
#include "Client_util.h"

// using namespace form the differential test .proto file.
using differential_test::company;
using differential_test::education_info;
using differential_test::dependent_info;
using differential_test::field_set;
using differential_test::employee;


int main(int argc, char* argv[]) {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  // Establish the gRPC channel with port number.
  std::string target_str;
  target_str = "0.0.0.0:50053";

  // Initial the service client instance.
  ServiceClient serviceClient(target_str);


  /*
   *  Write two message for testing.
   *  The .proto file of testing message is located in ../protos/differential_test.proto
   */
  employee message_first;

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
  dependent_info* dependentInfo_ptr_1 = message_first.mutable_dependents();
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


  employee message_second;

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
  dependent_info* dependentInfo_ptr_2 = message_second.mutable_dependents();
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


  // Generate the Differential Request
  DiffMsgRequest diff_request = Client_util::WriteMsgToDiffRequest(message_first,message_second);

  // Call Default Differential Service that compare two messages by default.
  DiffMsgReply repeatedRes = serviceClient.DefaultDifferentialService(diff_request);
  std::cout << "Message differential result (Default): \n" << repeatedRes.result() << std::endl;


  /*
   *  Try to Customize the differential service.
   */
  // Add the ignore field list.
  std::vector<std::string> ignore_list;
  ignore_list.push_back("differential_client.employee.employ_id");
  ignore_list.push_back("differential_client.employee.age");
  ignore_list.push_back("differential_client.dependent_info.age");
  Client_util::IgnoreFields(diff_request, ignore_list);


  // set the repeated field comparison
  std::string field_1 = "areas";
  Client_util::TreatRepeatedFieldAsListOrSet(diff_request, 1, field_1);
  std::string field_2 = "dependents.name";
  Client_util::TreatRepeatedFieldAsListOrSet(diff_request, 1, field_2);
  std::string field_3 = "dependents.age";
  Client_util::TreatRepeatedFieldAsListOrSet(diff_request, 0, field_3);


  // Set the map value for repeated filed compare.
  std::string map_field_name = "education";
  std::vector<std::string> sub_field_list;
  sub_field_list.push_back("name");
  sub_field_list.push_back("degree");
  Client_util::TreatRepeatedFieldAsMap(diff_request, map_field_name, sub_field_list);

  // Set the fraction and margin for float number comparison
  Client_util::SetFractionAndMargin(diff_request, 0.01, 0.0);

  // Call Differential Service that compare two messages by default.
  DiffMsgReply repeatedRes1 = serviceClient.DifferentialService(diff_request);
  std::cout << "Message differential result: \n" << repeatedRes1.result() << std::endl;
  
  google::protobuf::ShutdownProtobufLibrary();
  return 0;
}