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

#include "client_util.h"
#include "differential_client_lib/differential_service.grpc.pb.h"
#include "differential_client_lib/differential_test.grpc.pb.h"
#include "differential_service_client.h"

// using namespace form the differential test .proto file.
using google::protobuf::Message;
using google::protobuf::MessageFactory;
using google::protobuf::Reflection;
using google::protobuf::Descriptor;
using google::protobuf::DescriptorPool;
using google::protobuf::DescriptorProto;
using google::protobuf::FieldDescriptor;
using google::protobuf::FileDescriptor;
using google::protobuf::FileDescriptorProto;

using google::protobuf::Message;
using google::protobuf::MessageFactory;
using google::protobuf::Reflection;


using DifferentialTest::Company;
using DifferentialTest::EducationInfo;
using DifferentialTest::DependentInfo;
using DifferentialTest::ExamScore;
using DifferentialTest::TestEmployee;



int main(int argc, char* argv[]) {
  GOOGLE_PROTOBUF_VERIFY_VERSION;


/*
  std::cout << "Hello World!" << std::endl;

  TestEmployee message_first;

  message_first.set_employ_id(01);
  message_first.set_fullname("Jin Huang");
  message_first.set_age(32);

  std::cout << message_first.DebugString() << std::endl;

  const Descriptor* descriptor = message_first.GetDescriptor();

  const FieldDescriptor* field_descriptor = descriptor->FindFieldByName("fullname");

  const Reflection* refl = message_first.GetReflection();

  refl->ClearField(&message_first, field_descriptor);

  std::cout << message_first.DebugString() << std::endl;

  const Message& msg = refl->GetMessage(message_first, field_descriptor);

  std::cout << msg.DebugString() << std::endl;
*/

  // Establish the gRPC channel with port number.
  std::string target_str;
  target_str = "0.0.0.0:50053";

  // Initial the service client instance.
  DifferentialServiceClient service_client(target_str);

  bool connection_status = service_client.InitializeConnection();

  if (!connection_status){
    std::cerr << "ERROR: Connection Failure!" << std::endl;
    exit(1);
  }

  /*
   *  Write two message for testing.
   *  The .proto file of testing message is located in ../protos/differential_test.proto
   */
  TestEmployee message_first;

  ExamScore* exam_score_1 = message_first.add_exam_score();
  exam_score_1->set_exam1("Mid");
  exam_score_1->set_score1(98);
  exam_score_1->set_exam2("Final");
  exam_score_1->set_score2(86);

  /*
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
  DependentInfo* dependentInfo_ptr_1 = message_first.mutable_dependents();
  dependentInfo_ptr_1->add_name("Jeremy");
  dependentInfo_ptr_1->add_name("Zhe");
  dependentInfo_ptr_1->add_name("Jin");
  dependentInfo_ptr_1->add_name("June");
  dependentInfo_ptr_1->add_age(29);
  dependentInfo_ptr_1->add_age(30);
  dependentInfo_ptr_1->add_age(31);
  dependentInfo_ptr_1->add_age(26);
  EducationInfo* edu_info_1 = message_first.add_education();
  edu_info_1->set_name("Wright State University");
  edu_info_1->set_degree("PhD");
  edu_info_1->set_major("Computer Science");
  edu_info_1->set_address("OH, US");
  message_first.set_floatpoint(100.0);
*/

  TestEmployee message_second;

  ExamScore* exam_score_2 = message_second.add_exam_score();
  exam_score_2->set_exam1("Final");
  exam_score_2->set_exam2("Mid");
  exam_score_2->set_score1(86);
  exam_score_2->set_score2(98);

  /*
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
  DependentInfo* dependentInfo_ptr_2 = message_second.mutable_dependents();
  dependentInfo_ptr_2->add_name("Zhe");
  dependentInfo_ptr_2->add_name("Jeremy");
  dependentInfo_ptr_2->add_name("June");
  dependentInfo_ptr_2->add_name("Jin");
  dependentInfo_ptr_2->add_age(29);
  dependentInfo_ptr_2->add_age(30);
  dependentInfo_ptr_2->add_age(26);
  dependentInfo_ptr_2->add_age(31);
  EducationInfo* edu_info_2 = message_second.add_education();
  edu_info_2->set_name("Wright State University");
  edu_info_2->set_degree("Master of Science");
  edu_info_2->set_major("Computer Science and Engineering");
  edu_info_2->set_address("OH, US");
  message_second.set_floatpoint(109.9);
   */


  // Generate the Differential Request
  DiffRequest diff_request = ClientUtil::WriteMsgToDiffRequest(message_first, message_second);

  // Call Default Differential Service that compare two messages by default.
  DiffResponse diff_response_default =
      service_client.DefaultDifferentialService(diff_request);
  std::cout << "Message differential result (Default): \n"
            << diff_response_default.result() << std::endl;


  /*
   *  Try to Customize the differential service.
   */
  /*
  // Add the ignore field list.
  std::vector<std::string> ignore_fields_list;
  ignore_fields_list.push_back("DifferentialTest.employee.employ_id");
  ignore_fields_list.push_back("DifferentialTest.employee.age");
  ignore_fields_list.push_back("DifferentialTest.DependentInfo.age");
  ClientUtil::IgnoreFields(&diff_request, ignore_fields_list);


  // set the repeated field TestEmployee.areas as Set-Based Comparison
  std::string treat_field_as_list = "areas";
  ClientUtil::TreatRepeatedFieldAsListOrSet(&diff_request, 1,
                                            treat_field_as_list);

  // set the repeated field dependents.name as Set-Based Comparison
  std::string treat_field_as_set_dp_name = "dependents.name";
  ClientUtil::TreatRepeatedFieldAsListOrSet(&diff_request, 1,
                                            treat_field_as_set_dp_name);

  // set the repeated field dependents.age as List-Based Comparison
  std::string treat_field_as_set_2 = "dependents.age";
  ClientUtil::TreatRepeatedFieldAsListOrSet(&diff_request, 0,
                                            treat_field_as_set_2);


  // Set the map value comparison for repeated filed TestEmployee.education.
  std::string repeated_field_name = "education";
  std::vector<std::string> map_fields_list;
  // Set the sub-field education.name and education.degree as Map field.
  map_fields_list.push_back("name");
  map_fields_list.push_back("degree");
  ClientUtil::TreatRepeatedFieldAsMap(&diff_request, repeated_field_name,
                                      map_fields_list);

  // Set the fraction and margin for float number comparison
  ClientUtil::SetFractionAndMargin(&diff_request, 0.01, 0.0);
*/

  DifferentialService::MapCompareNotSameIndex* tmp = diff_request.mutable_map_compare_not_same_index();
  tmp->set_repeated_field("exam_score");
  tmp->set_first_key_field("exam_score.exam1");
  tmp->set_second_key_field("exam_score.exam2");



  // Call Differential Service that compare two messages.
  DiffResponse diff_response = service_client.DifferentialService(diff_request);
  std::cout << "Message differential result: \n"
            << diff_response.result() << std::endl;






  google::protobuf::ShutdownProtobufLibrary();
  return 0;
}