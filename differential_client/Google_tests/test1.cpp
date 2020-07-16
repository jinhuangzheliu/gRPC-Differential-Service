//
// Created by jinhuangzheliu on 7/12/20.
//
//
#include <google/protobuf/compiler/importer.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/message.h>
#include <grpcpp/grpcpp.h>

#include <iostream>
#include <memory>
#include <string>


#include "gtest/gtest.h"
#include "differential_service.grpc.pb.h"
#include "differential_test.grpc.pb.h"
#include "../ServiceClient.h"
#include "../Client_util.h"


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

using differentialservice::DifferentialServer;
using differentialservice::MsgRequest;
using differentialservice::MsgReply;
using differentialservice::DiffMsgRequest;
using differentialservice::DiffMsgReply;

using differential_test::company;
using differential_test::education_info;
using differential_test::dependent_info;
using differential_test::field_set;
using differential_test::employee;

namespace {
ServiceClient* TestStub_;
}

// Create the client stub of the differential service.
class DifferentialTestEnvironment :public testing::Environment {
 public:
  explicit DifferentialTestEnvironment(){
    std::string target_str = "0.0.0.0:50053";

    TestStub_ = new ServiceClient(target_str);
  }
};

namespace google {

/*
 * In this Unit Test, The .proto field differential_test.proto was used to generate
 * the input message. (Link: https://github.com/jinhuangzheliu/gRPC-Differential-Service/blob/master/protos/differential_test.proto)
 */

// Test 1: Test the default differential service.
TEST(Differential_unit, default_diff_1) {
  // Generate two message by message type employee(defined in differential_test.proto)
  employee message_first;
  employee message_second;
  // write two messages
  message_first.set_fullname("Jin Huang");
  message_first.set_age(32);

  message_second.set_fullname("Zhe Liu");
  message_second.set_age(32);

  // Generate the differential message request
  DiffMsgRequest diff_request = Client_util::WriteMsgToDiffRequest(message_first,
                                                                   message_second);
  // Receive the differential service reply.
  DiffMsgReply diff_reply = TestStub_->DefaultDifferentialService(diff_request);

  // Get the test result.
  const std::string& test_res = diff_reply.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // Only fullname is different
  const char* except_res = "modified: fullname: \"Jin Huang\" -> \"Zhe Liu\"\n";

  EXPECT_STREQ(c_test_res, except_res);
}



// Test 2: Test the nested field
TEST(Differential_unit, default_diff_2) {
  // Generate two message by message type employee(defined in differential_test.proto)
  employee message_first;
  employee message_second;

  // write two messages
  company* company_ptr_1 = message_first.mutable_employer();
  company_ptr_1->set_name("Google Inc.");
  company_ptr_1->set_occupation("Intern");
  company_ptr_1->set_address("CA, U.S.");

  company* company_ptr_2 = message_second.mutable_employer();
  company_ptr_2->set_name("Alphabet Inc.");
  company_ptr_2->set_occupation("Software Engineer");
  company_ptr_2->set_address("CA, U.S.");

  // Generate the differential message request
  DiffMsgRequest diff_request = Client_util::WriteMsgToDiffRequest(message_first,
                                                                   message_second);
  // Receive the differential service reply.
  DiffMsgReply diff_reply = TestStub_->DefaultDifferentialService(diff_request);

  // Get the test result.
  const std::string& test_res = diff_reply.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // Only name and occupation are different
  const char* except_res = "modified: employer.name: \"Google Inc.\" -> \"Alphabet Inc.\"\n"
      "modified: employer.occupation: \"Intern\" -> \"Software Engineer\"\n";

  EXPECT_STREQ(c_test_res, except_res);
}



// Test 3: Test the single field ignore.
TEST(Differential_unit, ignore_1) {
  // Generate two message by message type employee(defined in differential_test.proto)
  employee message_first;
  employee message_second;
  // write two messages
    message_first.set_fullname("Jin Huang");
  message_first.set_age(32);

  message_second.set_fullname("Zhe Liu");
  message_second.set_age(32);

  // Generate the differential message request
  DiffMsgRequest diff_request = Client_util::WriteMsgToDiffRequest(message_first,
                                                                   message_second);

  std::vector<std::string> fields;
  std::string field_1("differential_test.employee.fullname");
  fields.push_back(field_1);
  Client_util::IgnoreFields(diff_request, fields);

  // Receive the differential service reply.
  DiffMsgReply diff_reply = TestStub_->DifferentialService(diff_request);

  // Get the test result.
  const std::string& test_res = diff_reply.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // the fullname field was ignored so return is same.
  const char* except_res = "SAME";

  EXPECT_STREQ(c_test_res, except_res);
}



// Test 4: Test the multiple fields ignore
TEST(Differential_unit, ignore_2) {
  // Generate two message by message type employee(defined in differential_test.proto)
  employee message_first;
  employee message_second;
  // write two messages
  message_first.set_employ_id(001);
  message_first.set_fullname("Jin Huang");
  message_first.set_age(39);

  message_second.set_employ_id(002);
  message_second.set_fullname("Zhe Liu");
  message_second.set_age(32);

  // Generate the differential message request
  DiffMsgRequest diff_request = Client_util::WriteMsgToDiffRequest(message_first,
                                                                   message_second);

  std::vector<std::string> fields;
  std::string field_1("differential_test.employee.fullname");
  std::string field_2("differential_test.employee.employ_id");

  fields.push_back(field_1);
  fields.push_back(field_2);

  // Add the ignore field to the diff request
  Client_util::IgnoreFields(diff_request, fields);


  // Receive the differential service reply.
  DiffMsgReply diff_reply = TestStub_->DifferentialService(diff_request);

  // Get the test result.
  const std::string& test_res = diff_reply.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // fullname and employ_id were ignored but age is different
  const char* except_res = "modified: age: 39 -> 32\n";

  EXPECT_STREQ(c_test_res, except_res);
}



// Test 5: Test the multiple fields ignore include the nested field.
TEST(Differential_unit, ignore_3) {
  // Generate two message by message type employee(defined in differential_test.proto)
  employee message_first;
  employee message_second;

  // ******************** write two messages **********************


  // Set the message 1 by following stuff.
  message_first.set_employ_id(001);
  message_first.set_fullname("Jin Huang");
  message_first.set_age(39);

  message_first.mutable_employer()->set_name("Google Inc.");
  message_first.mutable_employer()->set_occupation("Software Engineer");
  message_first.mutable_employer()->set_address("CA, US");

  // Set the message 2 by the following stuff.
  message_second.set_employ_id(002);
  message_second.set_fullname("Zhe Liu");
  message_second.set_age(32);

  message_second.mutable_employer()->set_name("Alphabet Inc.");
  message_second.mutable_employer()->set_occupation("Research Scientist");
  message_second.mutable_employer()->set_address("CA, US");



  // ***************** Generate the differential message request ***************
  DiffMsgRequest diff_request = Client_util::WriteMsgToDiffRequest(message_first,
                                                                   message_second);

  //******************** Test Content ****************************************


  // Add the ignore fields. we add two more field nested under the message employee.
  std::vector<std::string> ignore_fields;
  std::string field_1("differential_test.employee.fullname");
  std::string field_2("differential_test.employee.employ_id");

  std::string nested_field_1("differential_test.company.name");
  std::string nested_field_2("differential_test.company.occupation");

  // push the fields
  ignore_fields.push_back(field_1);
  ignore_fields.push_back(field_2);

  // push the nested fields.
  ignore_fields.push_back(nested_field_1);
  ignore_fields.push_back(nested_field_2);

  // Write the ignore to differential request
  Client_util::IgnoreFields(diff_request, ignore_fields);



  // ********************** Differential Service **********************
  DiffMsgReply diff_reply = TestStub_->DifferentialService(diff_request);

  // Get the test result.
  const std::string& test_res = diff_reply.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // ******************** Prospective result **********************
  // Test the nested field ignore Criteria. The different between the nested fields
  // are ignored so only the different in age should be presented.
  const char* except_res = "modified: age: 39 -> 32\n";

  EXPECT_STREQ(c_test_res, except_res);
}



// Test 6: Test ignore nothing
TEST(Differential_unit, ignore_4) {
  // Generate two message by message type employee(defined in differential_test.proto)
  employee message_first;
  employee message_second;

  // ******************** write two messages **********************

  message_first.set_employ_id(001);
  message_first.set_fullname("Jin Huang");
  message_first.set_age(39);

  message_first.mutable_employer()->set_name("Google Inc.");
  message_first.mutable_employer()->set_occupation("Software Engineer");
  message_first.mutable_employer()->set_address("CA, US");

  // Set the message 2 by the following stuff.
  message_second.set_employ_id(002);
  message_second.set_fullname("Zhe Liu");
  message_second.set_age(32);

  message_second.mutable_employer()->set_name("Alphabet Inc.");
  message_second.mutable_employer()->set_occupation("Research Scientist");
  message_second.mutable_employer()->set_address("CA, US");



  // ***************** Generate the differential message request ***************
  DiffMsgRequest diff_request = Client_util::WriteMsgToDiffRequest(message_first,
                                                                   message_second);

  //******************** Test Content ****************************************


  // Add the ignore fields. we add two more field nested under the message employee.
  std::vector<std::string> ignore_fields;

  // Leave the ignore_fields as empty.

  // Write the ignore to differential request
  Client_util::IgnoreFields(diff_request, ignore_fields);



  // ********************** Differential Service **********************
  DiffMsgReply diff_reply = TestStub_->DifferentialService(diff_request);

  // Get the test result.
  const std::string& test_res = diff_reply.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // ******************** Prospective result **********************
  // Test if the ignore is empty.

  const char* except_res = "modified: employ_id: 1 -> 2\n"
                           "modified: fullname: \"Jin Huang\" -> \"Zhe Liu\"\n"
                           "modified: age: 39 -> 32\n"
                           "modified: employer.name: \"Google Inc.\" -> \"Alphabet Inc.\"\n"
                           "modified: employer.occupation: \"Software Engineer\" -> \"Research Scientist\"\n";

  EXPECT_STREQ(c_test_res, except_res);
}



// Test 7: Test ignore all fields in message.
TEST(Differential_unit, ignore_5) {
  // Generate two message by message type employee(defined in differential_test.proto)
  employee message_first;
  employee message_second;

  // ******************** write two messages **********************

  // Set the message 1 by following stuff.
  message_first.set_employ_id(001);
  message_first.set_fullname("Jin Huang");
  message_first.set_age(39);

  message_first.mutable_employer()->set_name("Google Inc.");
  message_first.mutable_employer()->set_occupation("Software Engineer");
  message_first.mutable_employer()->set_address("CA, US");

  // Set the message 2 by the following stuff.
  message_second.set_employ_id(002);
  message_second.set_fullname("Zhe Liu");
  message_second.set_age(32);

  message_second.mutable_employer()->set_name("Alphabet Inc.");
  message_second.mutable_employer()->set_occupation("Research Scientist");
  message_second.mutable_employer()->set_address("CA, US");


  // ***************** Generate the differential message request ***************
  DiffMsgRequest diff_request = Client_util::WriteMsgToDiffRequest(message_first,
                                                                   message_second);

  //******************** Test Content ****************************************

  // Add the ignore fields. we add two more field nested under the message employee.
  std::vector<std::string> ignore_fields;

  std::string field_1("differential_test.employee.fullname");
  std::string field_2("differential_test.employee.employ_id");
  std::string field_3("differential_test.employee.age");

  std::string nested_field_1("differential_test.company.name");
  std::string nested_field_2("differential_test.company.occupation");
  std::string nested_field_3("differential_test.company.address");

  // push the fields
  ignore_fields.push_back(field_1);
  ignore_fields.push_back(field_2);
  ignore_fields.push_back(field_3);

  // push the nested fields.
  ignore_fields.push_back(nested_field_1);
  ignore_fields.push_back(nested_field_2);
  ignore_fields.push_back(nested_field_3);

  // Black list criteria will do the ignore
  Client_util::IgnoreFields(diff_request, ignore_fields);


  // ********************** Differential Service **********************
  DiffMsgReply diff_reply = TestStub_->DifferentialService(diff_request);

  // Get the test result.
  const std::string& test_res = diff_reply.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // ******************** Prospective result **********************
  // Test all fields are ignored so result should be same.
  const char* except_res = "SAME";

  EXPECT_STREQ(c_test_res, except_res);
}



// Test 8: Test compare a single field.
TEST(Differential_unit, compare_1){
  // Generate two message by message type employee(defined in differential_test.proto)
  employee message_first;
  employee message_second;

  // ******************** write two messages **********************
  message_first.set_employ_id(001);
  message_first.set_fullname("Jin Huang");
  message_first.set_age(39);

  message_first.mutable_employer()->set_name("Google Inc.");
  message_first.mutable_employer()->set_occupation("Software Engineer");
  message_first.mutable_employer()->set_address("CA, US");

  // Set the message 2 by the following stuff.
  message_second.set_employ_id(002);
  message_second.set_fullname("Zhe Liu");
  message_second.set_age(32);

  message_second.mutable_employer()->set_name("Alphabet Inc.");
  message_second.mutable_employer()->set_occupation("Research Scientist");
  message_second.mutable_employer()->set_address("CA, US");


  // ***************** Generate the differential message request ***************
  DiffMsgRequest diff_request = Client_util::WriteMsgToDiffRequest(message_first,
                                                                   message_second);

  //******************** Test Content ****************************************

  // Add the ignore fields. we add two more field nested under the message employee.
  std::vector<std::string> compare_fields;

  std::string field_1("differential_test.employee.fullname");

  // push the fields
  compare_fields.push_back(field_1);

  // White list criteria will do the ignore
  Client_util::CompareFields(diff_request, compare_fields);


  // ********************** Differential Service **********************
  DiffMsgReply diff_reply = TestStub_->DifferentialService(diff_request);

  // Get the test result.
  const std::string& test_res = diff_reply.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // ******************** Prospective result **********************
  // Test only compare one field during the differential service
  const char* except_res = "modified: fullname: \"Jin Huang\" -> \"Zhe Liu\"\n";

  EXPECT_STREQ(c_test_res, except_res);
}



// Test 9: Test compare multiple fields.
TEST(Differential_unit, compare_2){
  // Generate two message by message type employee(defined in differential_test.proto)
  employee message_first;
  employee message_second;

  // ******************** write two messages **********************
  message_first.set_employ_id(001);
  message_first.set_fullname("Jin Huang");
  message_first.set_age(39);

  message_first.mutable_employer()->set_name("Google Inc.");
  message_first.mutable_employer()->set_occupation("Software Engineer");
  message_first.mutable_employer()->set_address("CA, US");

  // Set the message 2 by the following stuff.
  message_second.set_employ_id(002);
  message_second.set_fullname("Zhe Liu");
  message_second.set_age(32);

  message_second.mutable_employer()->set_name("Alphabet Inc.");
  message_second.mutable_employer()->set_occupation("Research Scientist");
  message_second.mutable_employer()->set_address("CA, US");


  // ***************** Generate the differential message request ***************
  DiffMsgRequest diff_request = Client_util::WriteMsgToDiffRequest(message_first,
                                                                   message_second);

  //******************** Test Content ****************************************
  // Add the ignore fields. we add two more field nested under the message employee.
  std::vector<std::string> compare_fields;

  std::string field_1("differential_test.employee.fullname");
  std::string field_2("differential_test.employee.employ_id");
  std::string field_3("differential_test.employee.age");

  // push the fields
  compare_fields.push_back(field_1);
  compare_fields.push_back(field_2);
  compare_fields.push_back(field_3);

  // White list criteria will do the ignore
  Client_util::CompareFields(diff_request, compare_fields);


  // ********************** Differential Service **********************
  DiffMsgReply diff_reply = TestStub_->DifferentialService(diff_request);

  // Get the test result.
  const std::string& test_res = diff_reply.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // ******************** Prospective result **********************
  // In this test we only compare these three fields
  const char* except_res = "modified: employ_id: 1 -> 2\n"
                           "modified: fullname: \"Jin Huang\" -> \"Zhe Liu\"\n"
                           "modified: age: 39 -> 32\n";

  EXPECT_STREQ(c_test_res, except_res);
}



// Test 10: Test compare a nested field in message.
TEST(Differential_unit, compare_3){
  // Generate two message by message type employee(defined in differential_test.proto)
  employee message_first;
  employee message_second;

  // ******************** write two messages **********************
  message_first.set_employ_id(001);
  message_first.set_fullname("Jin Huang");
  message_first.set_age(39);

  message_first.mutable_employer()->set_name("Google Inc.");
  message_first.mutable_employer()->set_occupation("Software Engineer");
  message_first.mutable_employer()->set_address("CA, US");

  // Set the message 2 by the following stuff.
  message_second.set_employ_id(002);
  message_second.set_fullname("Zhe Liu");
  message_second.set_age(32);

  message_second.mutable_employer()->set_name("Alphabet Inc.");
  message_second.mutable_employer()->set_occupation("Research Scientist");
  message_second.mutable_employer()->set_address("CA, US");


  // ***************** Generate the differential message request ***************
  DiffMsgRequest diff_request = Client_util::WriteMsgToDiffRequest(message_first,
                                                                   message_second);

  //******************** Test Content ****************************************
  // Add the ignore fields. we add two more field nested under the message employee.
  std::vector<std::string> compare_fields;

  // because we comapre a nested field so we have to add its parent field first.
  std::string parent_field_1("differential_test.employee.employer");
  std::string nested_field_1("differential_test.company.name");

  // push the nested fields.
  compare_fields.push_back(parent_field_1);
  compare_fields.push_back(nested_field_1);

  // White list criteria will do the ignore
  Client_util::CompareFields(diff_request, compare_fields);


  // ********************** Differential Service **********************
  DiffMsgReply diff_reply = TestStub_->DifferentialService(diff_request);

  // Get the test result.
  const std::string& test_res = diff_reply.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // ******************** Prospective result **********************
  // In this test case, we want to test the nested fields comparison
  const char* except_res = "modified: employer.name: \"Google Inc.\" -> \"Alphabet Inc.\"\n";

  EXPECT_STREQ(c_test_res, except_res);
}



// Test 11: Test compare multiple nested fields in message.
TEST(Differential_unit, compare_4){
  // Generate two message by message type employee(defined in differential_test.proto)
  employee message_first;
  employee message_second;

  // ******************** write two messages **********************
  message_first.set_employ_id(001);
  message_first.set_fullname("Jin Huang");
  message_first.set_age(39);

  message_first.mutable_employer()->set_name("Google Inc.");
  message_first.mutable_employer()->set_occupation("Software Engineer");
  message_first.mutable_employer()->set_address("CA, US");

  // Set the message 2 by the following stuff.
  message_second.set_employ_id(002);
  message_second.set_fullname("Zhe Liu");
  message_second.set_age(32);

  message_second.mutable_employer()->set_name("Alphabet Inc.");
  message_second.mutable_employer()->set_occupation("Research Scientist");
  message_second.mutable_employer()->set_address("CA, US");


  // ***************** Generate the differential message request ***************
  DiffMsgRequest diff_request = Client_util::WriteMsgToDiffRequest(message_first,
                                                                   message_second);

  //******************** Test Content ****************************************
  // Add the ignore fields. we add two more field nested under the message employee.
  std::vector<std::string> compare_fields;

  // because we comapre a nested field so we have to add its parent field first.
  std::string parent_field_1("differential_test.employee.employer");
  std::string nested_field_1("differential_test.company.name");
  std::string nested_field_2("differential_test.company.occupation");
  std::string nested_field_3("differential_test.company.address");

  // push the nested fields.
  compare_fields.push_back(parent_field_1);
  compare_fields.push_back(nested_field_1);
  compare_fields.push_back(nested_field_2);
  compare_fields.push_back(nested_field_3);

  // White list criteria will do the ignore
  Client_util::CompareFields(diff_request, compare_fields);


  // ********************** Differential Service **********************
  DiffMsgReply diff_reply = TestStub_->DifferentialService(diff_request);

  // Get the test result.
  const std::string& test_res = diff_reply.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // ******************** Prospective result **********************
  // Test multiple fields again.
  const char* except_res = "modified: employer.name: \"Google Inc.\" -> \"Alphabet Inc.\"\n"
                           "modified: employer.occupation: \"Software Engineer\" -> \"Research Scientist\"\n";

  EXPECT_STREQ(c_test_res, except_res);
}


// Test 12: Test compare no field in message. If the user want to compare nothing
// (leave the compare fields blank), our service will compare all fields in the message.
TEST(Differential_unit, compare_5){
  // Generate two message by message type employee(defined in differential_test.proto)
  employee message_first;
  employee message_second;

  // ******************** write two messages **********************
  message_first.set_employ_id(001);
  message_first.set_fullname("Jin Huang");
  message_first.set_age(39);

  message_first.mutable_employer()->set_name("Google Inc.");
  message_first.mutable_employer()->set_occupation("Software Engineer");
  message_first.mutable_employer()->set_address("CA, US");

  // Set the message 2 by the following stuff.
  message_second.set_employ_id(002);
  message_second.set_fullname("Zhe Liu");
  message_second.set_age(32);

  message_second.mutable_employer()->set_name("Alphabet Inc.");
  message_second.mutable_employer()->set_occupation("Research Scientist");
  message_second.mutable_employer()->set_address("CA, US");


  // ***************** Generate the differential message request ***************
  DiffMsgRequest diff_request = Client_util::WriteMsgToDiffRequest(message_first,
                                                                   message_second);

  //******************** Test Content ****************************************
  // Add the ignore fields. we add two more field nested under the message employee.
  std::vector<std::string> compare_fields;

  // Leave the compare fields empty.

  // White list criteria will do the ignore
  Client_util::CompareFields(diff_request, compare_fields);


  // ********************** Differential Service **********************
  DiffMsgReply diff_reply = TestStub_->DifferentialService(diff_request);

  // Get the test result.
  const std::string& test_res = diff_reply.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // ******************** Prospective result **********************
  // Compare all fields that user write
  const char* except_res = "modified: employ_id: 1 -> 2\n"
                           "modified: fullname: \"Jin Huang\" -> \"Zhe Liu\"\n"
                           "modified: age: 39 -> 32\n"
                           "modified: employer.name: \"Google Inc.\" -> \"Alphabet Inc.\"\n"
                           "modified: employer.occupation: \"Software Engineer\" -> \"Research Scientist\"\n";

  EXPECT_STREQ(c_test_res, except_res);
}


// Test 13:: Test the regular expression for ignoring the fields of message.
TEST(Differential_unit, regex_1){
  // Generate two message by message type employee(defined in differential_test.proto)
  employee message_first;
  employee message_second;

  // ******************** write two messages **********************

  message_first.mutable_employer()->set_name("Google Inc.");
  message_first.mutable_employer()->set_occupation("Software Engineer");
  message_first.mutable_employer()->set_address("CA, US");

  education_info* edu_info_1 = message_first.add_education();
  edu_info_1->set_name("University of Florida");
  edu_info_1->set_degree("PhD");
  edu_info_1->set_major("Computer Science");
  edu_info_1->set_address("FL, US");

  // Set the message 2 by the following stuff.
  message_second.mutable_employer()->set_name("Google Inc.");
  message_second.mutable_employer()->set_occupation("Software Engineer");
  message_second.mutable_employer()->set_address("NY, US");

  education_info* edu_info_2 = message_second.add_education();
  edu_info_2->set_name("Wright State University");
  edu_info_2->set_degree("PhD");
  edu_info_2->set_major("Computer Science");
  edu_info_2->set_address("OH, US");

  // ***************** Generate the differential message request ***************
  DiffMsgRequest diff_request = Client_util::WriteMsgToDiffRequest(message_first,
                                                                   message_second);

  //******************** Test Content ****************************************
  std::string regex(".*ress$");
  Client_util::RegexCriteria(diff_request, regex);

  // ********************** Differential Service **********************
  DiffMsgReply diff_reply = TestStub_->DifferentialService(diff_request);

  // Get the test result.
  const std::string& test_res = diff_reply.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // ******************** Prospective result **********************
  // Any different in suffix "ress" will be ignore.
  const char* except_res = "modified: education[0].name: \"University of Florida\" -> \"Wright State University\"\n";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 14: Test the regular expression for ignoring the fields of message.
TEST(Differential_unit, regex_2){
  // Generate two message by message type employee(defined in differential_test.proto)
  employee message_first;
  employee message_second;

  // ******************** write two messages **********************

  message_first.mutable_employer()->set_name("Google Inc.");
  message_first.mutable_employer()->set_occupation("Software Engineer");
  message_first.mutable_employer()->set_address("CA, US");

  education_info* edu_info_1 = message_first.add_education();
  edu_info_1->set_name("University of Florida");
  edu_info_1->set_degree("PhD");
  edu_info_1->set_major("Computer Science");
  edu_info_1->set_address("FL, US");

  // Set the message 2 by the following stuff.
  message_second.mutable_employer()->set_name("Google Inc.");
  message_second.mutable_employer()->set_occupation("Software Engineer");
  message_second.mutable_employer()->set_address("NY, US");

  education_info* edu_info_2 = message_second.add_education();
  edu_info_2->set_name("Wright State University");
  edu_info_2->set_degree("PhD");
  edu_info_2->set_major("Computer Science");
  edu_info_2->set_address("OH, US");

  // ***************** Generate the differential message request ***************
  DiffMsgRequest diff_request = Client_util::WriteMsgToDiffRequest(message_first,
                                                                   message_second);

  //******************** Test Content ****************************************
  std::string regex(".*ame$");
  Client_util::RegexCriteria(diff_request, regex);

  // ********************** Differential Service **********************
  DiffMsgReply diff_reply = TestStub_->DifferentialService(diff_request);

  // Get the test result.
  const std::string& test_res = diff_reply.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // ******************** Prospective result **********************
  // Any different in suffix "ame" will be ignore so the different in address should be presented.
  const char* except_res = "modified: employer.address: \"CA, US\" -> \"NY, US\"\n"
      "modified: education[0].address: \"FL, US\" -> \"OH, US\"\n";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 15: Test the regular expression for ignoring the fields of message.
TEST(Differential_unit, regex_3){
  // Generate two message by message type employee(defined in differential_test.proto)
  employee message_first;
  employee message_second;

  // ******************** write two messages **********************
  message_first.set_fullname("Jin Huang");
  message_first.set_age(32);

  message_first.mutable_employer()->set_name("Google Inc.");
  message_first.mutable_employer()->set_occupation("Software Engineer");
  message_first.mutable_employer()->set_address("CA, US");

  education_info* edu_info_1 = message_first.add_education();
  edu_info_1->set_name("University of Florida");
  edu_info_1->set_degree("PhD");
  edu_info_1->set_major("Computer Science");
  edu_info_1->set_address("FL, US");

  // Set the message 2 by the following stuff.
  message_second.set_fullname("Zhe Liu");
  message_second.set_age(29);

  message_second.mutable_employer()->set_name("Alphabet Inc.");
  message_second.mutable_employer()->set_occupation("Software Engineer");
  message_second.mutable_employer()->set_address("NY, US");

  education_info* edu_info_2 = message_second.add_education();
  edu_info_2->set_name("Wright State University");
  edu_info_2->set_degree("PhD");
  edu_info_2->set_major("Computer Science");
  edu_info_2->set_address("OH, US");

  // ***************** Generate the differential message request ***************
  DiffMsgRequest diff_request = Client_util::WriteMsgToDiffRequest(message_first,
                                                                   message_second);

  //******************** Test Content ****************************************
  std::string regex(".*ame$");
  Client_util::RegexCriteria(diff_request, regex);

  // ********************** Differential Service **********************
  DiffMsgReply diff_reply = TestStub_->DifferentialService(diff_request);

  // Get the test result.
  const std::string& test_res = diff_reply.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // ******************** Prospective result **********************
  // Any different in suffix "ame" will be ignore.
  // So the different between age, employer address, and education address should be presented.
  const char* except_res = "modified: age: 32 -> 29\n"
      "modified: employer.address: \"CA, US\" -> \"NY, US\"\n"
      "modified: education[0].address: \"FL, US\" -> \"OH, US\"\n";

  EXPECT_STREQ(c_test_res, except_res);
}


// Test 16: Test the repeated field treat as LIST.
TEST(Differential_unit, repeated_list_1){
  // Generate two message by message type employee(defined in differential_test.proto)
  employee message_first;
  employee message_second;

  // ******************** write two messages **********************
  // Write two message with same value and same order.
  message_first.add_areas("Google Ads.");
  message_first.add_areas("YouTube Ads.");
  message_first.add_areas("Search Ads.");
  message_first.add_areas("Click Ads.");

  message_second.add_areas("Google Ads.");
  message_second.add_areas("YouTube Ads.");
  message_second.add_areas("Search Ads.");
  message_second.add_areas("Click Ads.");


  // ***************** Generate the differential message request ***************
  DiffMsgRequest diff_request = Client_util::WriteMsgToDiffRequest(message_first,
                                                                   message_second);

  //******************** Test Content ****************************************
  // Set the field areas as list
  std::string field_1 = "areas";
  Client_util::TreatRepeatedFieldAsListOrSet(diff_request, 0, field_1);


  // ********************** Differential Service **********************
  DiffMsgReply diff_reply = TestStub_->DifferentialService(diff_request);

  // Get the test result.
  const std::string& test_res = diff_reply.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // ******************** Prospective result **********************
  // Compare area as List so it's should same.
  const char* except_res = "SAME";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 17: Test the repeated field treat as LIST.
TEST(Differential_unit, repeated_list_2){
  // Generate two message by message type employee(defined in differential_test.proto)
  employee message_first;
  employee message_second;

  // ******************** write two messages **********************

// Write two message with same value and same order.
  message_first.add_areas("Google Ads.");
  message_first.add_areas("YouTube Ads.");
  message_first.add_areas("Search Ads.");
  message_first.add_areas("Click Ads.");

  message_second.add_areas("Google Ads.");
  message_second.add_areas("YouTube Ads.");
  message_second.add_areas("Click Ads."); // different
  message_second.add_areas("Search Ads.");

  // ***************** Generate the differential message request ***************
  DiffMsgRequest diff_request = Client_util::WriteMsgToDiffRequest(message_first,
                                                                   message_second);

  //******************** Test Content ****************************************
  // Set the field areas as list
  std::string field_1 = "areas";
  Client_util::TreatRepeatedFieldAsListOrSet(diff_request, 0, field_1);


  // ********************** Differential Service **********************
  DiffMsgReply diff_reply = TestStub_->DifferentialService(diff_request);

  // Get the test result.
  const std::string& test_res = diff_reply.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // ******************** Prospective result **********************
  // compare as list so present the differential
  const char* except_res = "modified: areas[2]: \"Search Ads.\" -> \"Click Ads.\"\n"
      "modified: areas[3]: \"Click Ads.\" -> \"Search Ads.\"\n";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 18: Test a nested repeated field treat as LIST.
TEST(Differential_unit, repeated_list_3){
  // Generate two message by message type employee(defined in differential_test.proto)
  employee message_first;
  employee message_second;

  // ******************** write two messages **********************
  // Write two message with same value and same order.
  message_first.add_areas("Google Ads.");
  message_first.add_areas("YouTube Ads.");
  message_first.add_areas("Search Ads.");
  message_first.add_areas("Click Ads.");

  message_second.add_areas("Google Ads.");
  message_second.add_areas("YouTube Ads.");
  message_second.add_areas("Search Ads.");
  message_second.add_areas("Click Ads.");

  // Write the nested field for two messages.
  differential_test::dependent_info* dependentInfo_ptr_1 =
      message_first.mutable_dependents();
  dependentInfo_ptr_1->add_name("Jeremy");
  dependentInfo_ptr_1->add_name("Zhe");
  dependentInfo_ptr_1->add_name("Jin");
  dependentInfo_ptr_1->add_name("June");

  differential_test::dependent_info* dependentInfo_ptr_2 =
      message_second.mutable_dependents();

  dependentInfo_ptr_2->add_name("Jeremy");
  dependentInfo_ptr_2->add_name("Zhe");
  dependentInfo_ptr_2->add_name("Jin");
  dependentInfo_ptr_2->add_name("June");



  // ***************** Generate the differential message request ***************
  DiffMsgRequest diff_request = Client_util::WriteMsgToDiffRequest(message_first,
                                                                   message_second);

  //******************** Test Content ****************************************
  // Set the field areas as list
  std::string field_1 = "areas";
  Client_util::TreatRepeatedFieldAsListOrSet(diff_request, 0, field_1);
  std::string field_2 = "dependents.name";
  Client_util::TreatRepeatedFieldAsListOrSet(diff_request, 0, field_2);


  // ********************** Differential Service **********************
  DiffMsgReply diff_reply = TestStub_->DifferentialService(diff_request);

  // Get the test result.
  const std::string& test_res = diff_reply.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // ******************** Prospective result **********************
  // should same.
  const char* except_res = "SAME";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 18: Test a nested repeated field treat as LIST.
TEST(Differential_unit, repeated_list_4){
  // Generate two message by message type employee(defined in differential_test.proto)
  employee message_first;
  employee message_second;

  // ******************** write two messages **********************
  // Write two message with same value and same order.
  message_first.add_areas("Google Ads.");
  message_first.add_areas("YouTube Ads.");
  message_first.add_areas("Search Ads.");
  message_first.add_areas("Click Ads.");

  message_second.add_areas("Google Ads.");
  message_second.add_areas("YouTube Ads.");
  message_second.add_areas("Search Ads.");
  message_second.add_areas("Click Ads.");

  // Write the nested field for two messages.
  differential_test::dependent_info* dependentInfo_ptr_1 =
      message_first.mutable_dependents();
  dependentInfo_ptr_1->add_name("Jeremy");
  dependentInfo_ptr_1->add_name("Zhe");
  dependentInfo_ptr_1->add_name("Jin");
  dependentInfo_ptr_1->add_name("June");

  differential_test::dependent_info* dependentInfo_ptr_2 =
      message_second.mutable_dependents();

  dependentInfo_ptr_2->add_name("Zhe");
  dependentInfo_ptr_2->add_name("Jeremy");
  dependentInfo_ptr_2->add_name("June");
  dependentInfo_ptr_2->add_name("Jin");


  // ***************** Generate the differential message request ***************
  DiffMsgRequest diff_request = Client_util::WriteMsgToDiffRequest(message_first,
                                                                   message_second);

  //******************** Test Content ****************************************
  // Set the field areas as list
  std::string field_1 = "areas";
  Client_util::TreatRepeatedFieldAsListOrSet(diff_request, 0, field_1);
  std::string field_2 = "dependents.name";
  Client_util::TreatRepeatedFieldAsListOrSet(diff_request, 0, field_2);


  // ********************** Differential Service **********************
  DiffMsgReply diff_reply = TestStub_->DifferentialService(diff_request);

  // Get the test result.
  const std::string& test_res = diff_reply.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // ******************** Prospective result **********************
  // should same.
  const char* except_res = "modified: dependents.name[0]: \"Jeremy\" -> \"Zhe\"\n"
                           "modified: dependents.name[1]: \"Zhe\" -> \"Jeremy\"\n"
                           "modified: dependents.name[2]: \"Jin\" -> \"June\"\n"
                           "modified: dependents.name[3]: \"June\" -> \"Jin\"\n";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 16: Test multiple nested repeated fields treat as LIST.
TEST(Differential_unit, repeated_list_5){
  // Generate two message by message type employee(defined in differential_test.proto)
  employee message_first;
  employee message_second;

  // ******************** write two messages **********************

  // Write two message with same value and same order.
  message_first.add_areas("Google Ads.");
  message_first.add_areas("YouTube Ads.");
  message_first.add_areas("Search Ads.");
  message_first.add_areas("Click Ads.");

  message_second.add_areas("Google Ads.");
  message_second.add_areas("YouTube Ads.");
  message_second.add_areas("Search Ads.");
  message_second.add_areas("Click Ads.");

  // Write the nested field for two messages.
  differential_test::dependent_info* dependentInfo_ptr_1 =
      message_first.mutable_dependents();
  dependentInfo_ptr_1->add_name("Jeremy");
  dependentInfo_ptr_1->add_name("Zhe");
  dependentInfo_ptr_1->add_name("Jin");
  dependentInfo_ptr_1->add_name("June");

  dependentInfo_ptr_1->add_age(29);
  dependentInfo_ptr_1->add_age(30);
  dependentInfo_ptr_1->add_age(32);
  dependentInfo_ptr_1->add_age(26);

  differential_test::dependent_info* dependentInfo_ptr_2 =
      message_second.mutable_dependents();

  dependentInfo_ptr_2->add_name("Zhe");
  dependentInfo_ptr_2->add_name("Jeremy");
  dependentInfo_ptr_2->add_name("June");
  dependentInfo_ptr_2->add_name("Jin");

  dependentInfo_ptr_2->add_age(29);
  dependentInfo_ptr_2->add_age(32);
  dependentInfo_ptr_2->add_age(30);
  dependentInfo_ptr_2->add_age(26);

  // ***************** Generate the differential message request ***************
  DiffMsgRequest diff_request = Client_util::WriteMsgToDiffRequest(message_first,
                                                                   message_second);

  //******************** Test Content ****************************************
  // Set the field areas as list
  std::string field_1 = "areas";
  Client_util::TreatRepeatedFieldAsListOrSet(diff_request, 0, field_1);
  std::string field_2 = "dependents.name";
  Client_util::TreatRepeatedFieldAsListOrSet(diff_request, 0, field_2);
  std::string field_3 = "dependents.age";
  Client_util::TreatRepeatedFieldAsListOrSet(diff_request, 0, field_3);


  // ********************** Differential Service **********************
  DiffMsgReply diff_reply = TestStub_->DifferentialService(diff_request);

  // Get the test result.
  const std::string& test_res = diff_reply.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // ******************** Prospective result **********************
  const char* except_res = "modified: dependents.name[0]: \"Jeremy\" -> \"Zhe\"\n"
                           "modified: dependents.name[1]: \"Zhe\" -> \"Jeremy\"\n"
                           "modified: dependents.name[2]: \"Jin\" -> \"June\"\n"
                           "modified: dependents.name[3]: \"June\" -> \"Jin\"\n"
                           "modified: dependents.age[1]: 30 -> 32\n"
                           "modified: dependents.age[2]: 32 -> 30\n";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 17: Test multiple nested repeated fields treat as LIST.
TEST(Differential_unit, repeated_list_6){
  // Generate two message by message type employee(defined in differential_test.proto)
  employee message_first;
  employee message_second;

  // ******************** write two messages **********************
  // Write two message with same value and same order.
  message_first.add_areas("Google Ads.");
  message_first.add_areas("YouTube Ads.");
  message_first.add_areas("Search Ads.");
  message_first.add_areas("Click Ads.");

  message_second.add_areas("Google Ads.");
  message_second.add_areas("YouTube Ads.");
  message_second.add_areas("Search Ads.");
  message_second.add_areas("Click Ads.");

  // Write the nested field for two messages.
  differential_test::dependent_info* dependentInfo_ptr_1 =
      message_first.mutable_dependents();
  dependentInfo_ptr_1->add_name("Jeremy");
  dependentInfo_ptr_1->add_name("Zhe");
  dependentInfo_ptr_1->add_name("Jin");
  dependentInfo_ptr_1->add_name("June");

  dependentInfo_ptr_1->add_age(29);
  dependentInfo_ptr_1->add_age(30);
  dependentInfo_ptr_1->add_age(32);
  dependentInfo_ptr_1->add_age(26);

  differential_test::dependent_info* dependentInfo_ptr_2 =
      message_second.mutable_dependents();

  dependentInfo_ptr_2->add_name("Jeremy");
  dependentInfo_ptr_2->add_name("Zhe");
  dependentInfo_ptr_2->add_name("Jin");
  dependentInfo_ptr_2->add_name("June");

  dependentInfo_ptr_2->add_age(29);
  dependentInfo_ptr_2->add_age(30);
  dependentInfo_ptr_2->add_age(32);
  dependentInfo_ptr_2->add_age(26);


  // ***************** Generate the differential message request ***************
  DiffMsgRequest diff_request = Client_util::WriteMsgToDiffRequest(message_first,
                                                                   message_second);

  //******************** Test Content ****************************************
  // Set the field areas as list
  std::string field_1 = "areas";
  Client_util::TreatRepeatedFieldAsListOrSet(diff_request, 0, field_1);
  std::string field_2 = "dependents.name";
  Client_util::TreatRepeatedFieldAsListOrSet(diff_request, 0, field_2);
  std::string field_3 = "dependents.age";
  Client_util::TreatRepeatedFieldAsListOrSet(diff_request, 0, field_3);


  // ********************** Differential Service **********************
  DiffMsgReply diff_reply = TestStub_->DifferentialService(diff_request);

  // Get the test result.
  const std::string& test_res = diff_reply.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // ******************** Prospective result **********************
  const char* except_res = "SAME";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 18: Test the repeated field treat as LIST. deleted element.
TEST(Differential_unit, repeated_list_7){
  // Generate two message by message type employee(defined in differential_test.proto)
  employee message_first;
  employee message_second;

  // ******************** write two messages **********************
  // Write two message with same value and same order.
  message_first.add_areas("Google Ads.");
  message_first.add_areas("YouTube Ads.");
  message_first.add_areas("Search Ads.");
  message_first.add_areas("Click Ads.");

  message_second.add_areas("Google Ads.");
  message_second.add_areas("YouTube Ads.");
  message_second.add_areas("Search Ads.");


  // ***************** Generate the differential message request ***************
  DiffMsgRequest diff_request = Client_util::WriteMsgToDiffRequest(message_first,
                                                                   message_second);

  //******************** Test Content ****************************************
  // Set the field areas as list
  std::string field_1 = "areas";
  Client_util::TreatRepeatedFieldAsListOrSet(diff_request, 0, field_1);


  // ********************** Differential Service **********************
  DiffMsgReply diff_reply = TestStub_->DifferentialService(diff_request);

  // Get the test result.
  const std::string& test_res = diff_reply.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // ******************** Prospective result **********************
  const char* except_res = "deleted: areas[3]: \"Click Ads.\"\n";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 19: Test the repeated field treat as LIST. add element.
TEST(Differential_unit, repeated_list_8){
  employee message_first;
  employee message_second;
   

  // Write two message with same value and same order.
  message_first.add_areas("Google Ads.");
  message_first.add_areas("YouTube Ads.");
  message_first.add_areas("Search Ads.");


  message_second.add_areas("Google Ads.");
  message_second.add_areas("YouTube Ads.");
  message_second.add_areas("Search Ads.");
  message_second.add_areas("Click Ads.");


  // Write the message to log message.
  DiffMsgRequest diff_request = Client_util::WriteMsgToDiffRequest(message_first,
                                                                   message_second);

  // Set the field areas as list
  std::string field_1 = "areas";
  Client_util::TreatRepeatedFieldAsListOrSet(diff_request, 0, field_1);

  // Implements the differential service.
   DiffMsgReply diff_reply = TestStub_->DifferentialService(diff_request);

  // Get the test result.
  const std::string& test_res = diff_reply.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // we test the field areas have the same value with the same order.
  const char* except_res = "added: areas[3]: \"Click Ads.\"\n";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 20: Test the repeated field treat as SET.
TEST(Differential_unit, repeated_set_1){
  employee message_first;
  employee message_second;
   

  // Write two message with same value but different order.
  message_first.add_areas("Google Ads.");
  message_first.add_areas("YouTube Ads.");
  message_first.add_areas("Search Ads.");
  message_first.add_areas("Click Ads.");

  message_second.add_areas("Search Ads.");
  message_second.add_areas("Click Ads.");
  message_second.add_areas("Google Ads.");
  message_second.add_areas("YouTube Ads.");

  // Write the message to log message.
    DiffMsgRequest diff_request = Client_util::WriteMsgToDiffRequest(message_first,
                                                                   message_second);

  // Set the field areas as set
  std::string field_1 = "areas";
  Client_util::TreatRepeatedFieldAsListOrSet(diff_request, 1, field_1);

  // Implements the differential service.
   DiffMsgReply diff_reply = TestStub_->DifferentialService(diff_request);

  // Get the test result.
  const std::string& test_res = diff_reply.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // Because we treat the repeated field as set so even the element have different
  // order between the two messages the result is same.
  const char* except_res = "SAME";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 21: Test the repeated field treat as SET.
TEST(Differential_unit, repeated_set_2){

  employee message_first;
  employee message_second;
   

  // Write two message with different value
  message_first.add_areas("Google Ads.");
  message_first.add_areas("YouTube Ads."); // The different one.
  message_first.add_areas("Search Ads.");
  message_first.add_areas("Click Ads.");

  message_second.add_areas("Search Ads.");
  message_second.add_areas("Click Ads.");
  message_second.add_areas("Google Ads.");
  message_second.add_areas("Pop_up Ads."); // The different one.



  // Write the message to log message.
    DiffMsgRequest diff_request = Client_util::WriteMsgToDiffRequest(message_first,
                                                                   message_second);

  // Set the field areas as set
  std::string field_1 = "areas";
  Client_util::TreatRepeatedFieldAsListOrSet(diff_request, 1, field_1);

  // Implements the differential service.
   DiffMsgReply diff_reply = TestStub_->DifferentialService(diff_request);

  // Get the test result.
  const std::string& test_res = diff_reply.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // The first message needs deleting the areas[1] and add "Pop_up Ads.".
  const char* except_res = "added: areas[3]: \"Pop_up Ads.\"\ndeleted: areas[1]: \"YouTube Ads.\"\n";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 22: Test the repeated field treat as SET. delete element.
TEST(Differential_unit, repeated_set_3){

  employee message_first;
  employee message_second;
   

  // Write two message with different value
  message_first.add_areas("Google Ads.");
  message_first.add_areas("YouTube Ads."); // The different one.
  message_first.add_areas("Search Ads.");
  message_first.add_areas("Click Ads.");

  message_second.add_areas("Search Ads.");
  message_second.add_areas("Click Ads.");
  message_second.add_areas("Google Ads.");



  // Write the message to log message.
    DiffMsgRequest diff_request = Client_util::WriteMsgToDiffRequest(message_first,
                                                                   message_second);

  // Set the field areas as set
  std::string field_1 = "areas";
  Client_util::TreatRepeatedFieldAsListOrSet(diff_request, 1, field_1);

  // Implements the differential service.
   DiffMsgReply diff_reply = TestStub_->DifferentialService(diff_request);

  // Get the test result.
  const std::string& test_res = diff_reply.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // The first message needs deleting the areas[1]
  const char* except_res = "deleted: areas[1]: \"YouTube Ads.\"\n";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 23: Test the repeated field treat as SET. Add element
TEST(Differential_unit, repeated_set_4){

  employee message_first;
  employee message_second;
   

  // Write two message with different value
  message_first.add_areas("Google Ads.");
  message_first.add_areas("Search Ads.");
  message_first.add_areas("Click Ads.");

  message_second.add_areas("Search Ads.");
  message_second.add_areas("Click Ads.");
  message_second.add_areas("Google Ads.");
  message_second.add_areas("Pop_up Ads."); // The different one.

  // Write the message to log message.
    DiffMsgRequest diff_request = Client_util::WriteMsgToDiffRequest(message_first,
                                                                   message_second);

  // Set the field areas as set
  std::string field_1 = "areas";
  Client_util::TreatRepeatedFieldAsListOrSet(diff_request, 1, field_1);

  // Implements the differential service.
   DiffMsgReply diff_reply = TestStub_->DifferentialService(diff_request);

  // Get the test result.
  const std::string& test_res = diff_reply.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // The first message needs adding "Pop_up Ads.".
  const char* except_res = "added: areas[3]: \"Pop_up Ads.\"\n";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 24: Test the repeated field treat as SET. adding mulitpe elements
TEST(Differential_unit, repeated_set_5){

  employee message_first;
  employee message_second;
   

  // Write two message with different value
  message_first.add_areas("Google Ads.");

  message_second.add_areas("Search Ads.");
  message_second.add_areas("Click Ads.");
  message_second.add_areas("Google Ads.");
  message_second.add_areas("Pop_up Ads."); // The different one.

  // Write the message to log message.
    DiffMsgRequest diff_request = Client_util::WriteMsgToDiffRequest(message_first,
                                                                   message_second);

  // Set the field areas as set
  std::string field_1 = "areas";
  Client_util::TreatRepeatedFieldAsListOrSet(diff_request, 1, field_1);

  // Implements the differential service.
   DiffMsgReply diff_reply = TestStub_->DifferentialService(diff_request);

  // Get the test result.
  const std::string& test_res = diff_reply.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // The first message needs adding multiple elements than the seconde message.
  const char* except_res = "added: areas[0]: \"Search Ads.\"\n"
      "added: areas[1]: \"Click Ads.\"\n"
      "added: areas[3]: \"Pop_up Ads.\"\n";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 25: Test the nested repeated field treat as SET.
TEST(Differential_unit, repeated_set_6){

  employee message_first;
  employee message_second;
   

  // Write two message with same value and same order.
  message_first.add_areas("Google Ads.");
  message_first.add_areas("YouTube Ads.");
  message_first.add_areas("Search Ads.");
  message_first.add_areas("Click Ads.");

  message_second.add_areas("Google Ads.");
  message_second.add_areas("YouTube Ads.");
  message_second.add_areas("Click Ads.");
  message_second.add_areas("Search Ads.");

  // Write the nested field for two messages.
  differential_test::dependent_info* dependentInfo_ptr_1 =
      message_first.mutable_dependents();
  dependentInfo_ptr_1->add_name("Jeremy");
  dependentInfo_ptr_1->add_name("Zhe");
  dependentInfo_ptr_1->add_name("Jin");
  dependentInfo_ptr_1->add_name("June");

  dependentInfo_ptr_1->add_age(29);
  dependentInfo_ptr_1->add_age(30);
  dependentInfo_ptr_1->add_age(32);
  dependentInfo_ptr_1->add_age(26);

  differential_test::dependent_info* dependentInfo_ptr_2 =
      message_second.mutable_dependents();

  dependentInfo_ptr_2->add_name("Zhe");
  dependentInfo_ptr_2->add_name("Jeremy");
  dependentInfo_ptr_2->add_name("June");
  dependentInfo_ptr_2->add_name("Jin");

  dependentInfo_ptr_2->add_age(29);
  dependentInfo_ptr_2->add_age(30);
  dependentInfo_ptr_2->add_age(32);
  dependentInfo_ptr_2->add_age(26);

  // Write the message to log message.
    DiffMsgRequest diff_request = Client_util::WriteMsgToDiffRequest(message_first,
                                                                   message_second);

  // Set the field areas as set
  std::string field_1 = "areas";
  Client_util::TreatRepeatedFieldAsListOrSet(diff_request, 1, field_1);

  std::string field_2 = "dependents.name";
  Client_util::TreatRepeatedFieldAsListOrSet(diff_request, 1, field_2);

  std::string field_3 = "dependents.age";
  Client_util::TreatRepeatedFieldAsListOrSet(diff_request, 1, field_3);

  // Implements the differential service.
   DiffMsgReply diff_reply = TestStub_->DifferentialService(diff_request);

  // Get the test result.
  const std::string& test_res = diff_reply.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // The first message needs adding multiple elements than the seconde message.
  const char* except_res = "SAME";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 26: Test a nested repeated field treat as SET. Add element/s
TEST(Differential_unit, repeated_set_7){

  employee message_first;
  employee message_second;
   

  // Write two message with same value and same order.
  message_first.add_areas("Google Ads.");
  message_first.add_areas("YouTube Ads.");
  message_first.add_areas("Search Ads.");
  message_first.add_areas("Click Ads.");

  message_second.add_areas("Google Ads.");
  message_second.add_areas("YouTube Ads.");
  message_second.add_areas("Click Ads.");
  message_second.add_areas("Search Ads.");

  // Write the nested field for two messages.
  differential_test::dependent_info* dependentInfo_ptr_1 =
      message_first.mutable_dependents();
  dependentInfo_ptr_1->add_name("Jeremy");
  dependentInfo_ptr_1->add_name("Zhe");
  
  dependentInfo_ptr_1->add_age(29);
  dependentInfo_ptr_1->add_age(30);
  dependentInfo_ptr_1->add_age(32);


  differential_test::dependent_info* dependentInfo_ptr_2 =
      message_second.mutable_dependents();

  dependentInfo_ptr_2->add_name("Zhe");
  dependentInfo_ptr_2->add_name("Jeremy");
  dependentInfo_ptr_2->add_name("June");
  dependentInfo_ptr_2->add_name("Jin");

  dependentInfo_ptr_2->add_age(29);
  dependentInfo_ptr_2->add_age(30);
  dependentInfo_ptr_2->add_age(32);
  dependentInfo_ptr_2->add_age(26);

  // Write the message to log message.
    DiffMsgRequest diff_request = Client_util::WriteMsgToDiffRequest(message_first,
                                                                   message_second);

  // Set the field areas as set
  std::string field_1 = "areas";
  Client_util::TreatRepeatedFieldAsListOrSet(diff_request, 1, field_1);

  std::string field_2 = "dependents.name";
  Client_util::TreatRepeatedFieldAsListOrSet(diff_request, 1, field_2);

  std::string field_3 = "dependents.age";
  Client_util::TreatRepeatedFieldAsListOrSet(diff_request, 1, field_3);

  // Implements the differential service.
   DiffMsgReply diff_reply = TestStub_->DifferentialService(diff_request);

  // Get the test result.
  const std::string& test_res = diff_reply.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // The first message needs adding multiple elements than the seconde message.
  const char* except_res = "added: dependents.name[2]: \"June\"\n"
      "added: dependents.name[3]: \"Jin\"\n"
      "added: dependents.age[3]: 26\n";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 27: Test a nested repeated field treat as SET. delete element/s
TEST(Differential_unit, repeated_set_8){

  employee message_first;
  employee message_second;
   

  // Write two message with same value and same order.
  message_first.add_areas("Google Ads.");
  message_first.add_areas("YouTube Ads.");
  message_first.add_areas("Search Ads.");
  message_first.add_areas("Click Ads.");

  message_second.add_areas("Google Ads.");
  message_second.add_areas("YouTube Ads.");
  message_second.add_areas("Click Ads.");
  message_second.add_areas("Search Ads.");

  // Write the nested field for two messages.
  differential_test::dependent_info* dependentInfo_ptr_1 =
      message_first.mutable_dependents();
  dependentInfo_ptr_1->add_name("Jeremy");
  dependentInfo_ptr_1->add_name("Zhe");
  dependentInfo_ptr_1->add_name("Jin");
  dependentInfo_ptr_1->add_name("June");

  dependentInfo_ptr_1->add_age(29);
  dependentInfo_ptr_1->add_age(30);
  dependentInfo_ptr_1->add_age(32);
  dependentInfo_ptr_1->add_age(26);

  differential_test::dependent_info* dependentInfo_ptr_2 =
      message_second.mutable_dependents();

  dependentInfo_ptr_2->add_name("Zhe");
  dependentInfo_ptr_2->add_name("Jeremy");
  dependentInfo_ptr_2->add_name("June");

  dependentInfo_ptr_2->add_age(29);
  dependentInfo_ptr_2->add_age(30);
  dependentInfo_ptr_2->add_age(26);

  // Write the message to log message.
    DiffMsgRequest diff_request = Client_util::WriteMsgToDiffRequest(message_first,
                                                                   message_second);

  // Set the field areas as set
  std::string field_1 = "areas";
  Client_util::TreatRepeatedFieldAsListOrSet(diff_request, 1, field_1);

  std::string field_2 = "dependents.name";
  Client_util::TreatRepeatedFieldAsListOrSet(diff_request, 1, field_2);

  std::string field_3 = "dependents.age";
  Client_util::TreatRepeatedFieldAsListOrSet(diff_request, 1, field_3);

  // Implements the differential service.
   DiffMsgReply diff_reply = TestStub_->DifferentialService(diff_request);

  // Get the test result.
  const std::string& test_res = diff_reply.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // The first message needs adding multiple elements than the seconde message.
  const char* except_res = "deleted: dependents.name[2]: \"Jin\"\n"
      "deleted: dependents.age[2]: 32\n";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 28: Test a nested repeated field treat as SET. delete element/s
TEST(Differential_unit, repeated_set_9){

  employee message_first;
  employee message_second;
   

  // Write two message with same value and same order.
  message_first.add_areas("Google Ads.");
  message_first.add_areas("YouTube Ads.");
  message_first.add_areas("Search Ads.");
  message_first.add_areas("Click Ads.");

  message_second.add_areas("Google Ads.");
  message_second.add_areas("YouTube Ads.");
  message_second.add_areas("Click Ads.");
  message_second.add_areas("Search Ads.");

  // Write the nested field for two messages.
  differential_test::dependent_info* dependentInfo_ptr_1 =
      message_first.mutable_dependents();
  dependentInfo_ptr_1->add_name("Jeremy");
  dependentInfo_ptr_1->add_name("Zhe");
  dependentInfo_ptr_1->add_name("Jin");
  dependentInfo_ptr_1->add_name("June");

  dependentInfo_ptr_1->add_age(29);
  dependentInfo_ptr_1->add_age(30);
  dependentInfo_ptr_1->add_age(32);
  dependentInfo_ptr_1->add_age(26);

  differential_test::dependent_info* dependentInfo_ptr_2 =
      message_second.mutable_dependents();

  dependentInfo_ptr_2->add_name("Zhe");
  dependentInfo_ptr_2->add_name("Jeremy");
  dependentInfo_ptr_2->add_name("June");
//  dependentInfo_ptr_2->add_name("Jin");

  dependentInfo_ptr_2->add_age(29);
  dependentInfo_ptr_2->add_age(30);
//  dependentInfo_ptr_2->add_age(32);
  dependentInfo_ptr_2->add_age(26);

  // Write the message to log message.
    DiffMsgRequest diff_request = Client_util::WriteMsgToDiffRequest(message_first,
                                                                   message_second);

  // Set the field areas as set
  std::string field_1 = "areas";
  Client_util::TreatRepeatedFieldAsListOrSet(diff_request, 1, field_1);

  std::string field_2 = "dependents.name";
  Client_util::TreatRepeatedFieldAsListOrSet(diff_request, 1, field_2);

  std::string field_3 = "dependents.age";
  Client_util::TreatRepeatedFieldAsListOrSet(diff_request, 1, field_3);

  // Implements the differential service.
   DiffMsgReply diff_reply = TestStub_->DifferentialService(diff_request);

  // Get the test result.
  const std::string& test_res = diff_reply.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // The first message needs adding multiple elements than the seconde message.
  const char* except_res = "deleted: dependents.name[2]: \"Jin\"\n"
                         "deleted: dependents.age[2]: 32\n";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 29: Test a nested repeated field treat as SET. delete and add element/s
TEST(Differential_unit, repeated_set_10){

  employee message_first;
  employee message_second;
   

  // Write two message with same value and same order.
  message_first.add_areas("Google Ads.");
  message_first.add_areas("YouTube Ads.");
  message_first.add_areas("Pop_up Ads.");
  message_first.add_areas("Click Ads.");

  message_second.add_areas("Google Ads.");
  message_second.add_areas("YouTube Ads.");
  message_second.add_areas("Click Ads.");
  message_second.add_areas("Search Ads.");

  // Write the nested field for two messages.
  differential_test::dependent_info* dependentInfo_ptr_1 =
      message_first.mutable_dependents();
  dependentInfo_ptr_1->add_name("Jeremy");
  dependentInfo_ptr_1->add_name("Zhe");
  dependentInfo_ptr_1->add_name("Tom");
  dependentInfo_ptr_1->add_name("June");

  dependentInfo_ptr_1->add_age(29);
  dependentInfo_ptr_1->add_age(30);
  dependentInfo_ptr_1->add_age(31);
  dependentInfo_ptr_1->add_age(26);

  differential_test::dependent_info* dependentInfo_ptr_2 =
      message_second.mutable_dependents();

  dependentInfo_ptr_2->add_name("Zhe");
  dependentInfo_ptr_2->add_name("Jeremy");
  dependentInfo_ptr_2->add_name("June");
  dependentInfo_ptr_2->add_name("Jin");

  dependentInfo_ptr_2->add_age(29);
  dependentInfo_ptr_2->add_age(30);
  dependentInfo_ptr_2->add_age(32);
  dependentInfo_ptr_2->add_age(26);

  // Write the message to log message.
    DiffMsgRequest diff_request = Client_util::WriteMsgToDiffRequest(message_first,
                                                                   message_second);

  // Set the field areas as set
  std::string field_1 = "areas";
  Client_util::TreatRepeatedFieldAsListOrSet(diff_request, 1, field_1);

  std::string field_2 = "dependents.name";
  Client_util::TreatRepeatedFieldAsListOrSet(diff_request, 1, field_2);

  std::string field_3 = "dependents.age";
  Client_util::TreatRepeatedFieldAsListOrSet(diff_request, 1, field_3);

  // Implements the differential service.
   DiffMsgReply diff_reply = TestStub_->DifferentialService(diff_request);

  // Get the test result.
  const std::string& test_res = diff_reply.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // The first message needs adding multiple elements than the seconde message.
  const char* except_res = "added: areas[3]: \"Search Ads.\"\n"
      "deleted: areas[2]: \"Pop_up Ads.\"\n"
      "added: dependents.name[3]: \"Jin\"\n"
      "deleted: dependents.name[2]: \"Tom\"\n"
      "added: dependents.age[2]: 32\n"
      "deleted: dependents.age[2]: 31\n";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 30: Test a nested repeated field treat as SET. Add element/s
TEST(Differential_unit, repeated_list_and_set){

  employee message_first;
  employee message_second;
   

  // Write two message with same value and same order.
  message_first.add_areas("Google Ads.");
  message_first.add_areas("YouTube Ads.");
  message_first.add_areas("Search Ads.");
  message_first.add_areas("Click Ads.");

  message_second.add_areas("Google Ads.");
  message_second.add_areas("YouTube Ads.");
  message_second.add_areas("Search Ads.");
  message_second.add_areas("Click Ads.");


  // Write the nested field for two messages.
  differential_test::dependent_info* dependentInfo_ptr_1 =
      message_first.mutable_dependents();
  dependentInfo_ptr_1->add_name("Jeremy");
  dependentInfo_ptr_1->add_name("Zhe");
  dependentInfo_ptr_1->add_name("June");
  dependentInfo_ptr_1->add_name("Jin");

  dependentInfo_ptr_1->add_age(29);
  dependentInfo_ptr_1->add_age(30);
  dependentInfo_ptr_1->add_age(32);
  dependentInfo_ptr_1->add_age(26);

  differential_test::dependent_info* dependentInfo_ptr_2 =
      message_second.mutable_dependents();

  dependentInfo_ptr_2->add_name("Zhe");
  dependentInfo_ptr_2->add_name("Jeremy");
  dependentInfo_ptr_2->add_name("June");
  dependentInfo_ptr_2->add_name("Jin");

  dependentInfo_ptr_2->add_age(29);
  dependentInfo_ptr_2->add_age(30);
  dependentInfo_ptr_2->add_age(32);
  dependentInfo_ptr_2->add_age(26);

  // Write the message to log message.
    DiffMsgRequest diff_request = Client_util::WriteMsgToDiffRequest(message_first,
                                                                   message_second);

  // Set the field areas as set
  std::string field_1 = "areas";
  Client_util::TreatRepeatedFieldAsListOrSet(diff_request, 0, field_1);

  std::string field_2 = "dependents.name";
  Client_util::TreatRepeatedFieldAsListOrSet(diff_request, 1, field_2);

  std::string field_3 = "dependents.age";
  Client_util::TreatRepeatedFieldAsListOrSet(diff_request, 0, field_3);

  // Implements the differential service.
   DiffMsgReply diff_reply = TestStub_->DifferentialService(diff_request);

  // Get the test result.
  const std::string& test_res = diff_reply.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // The first message needs adding multiple elements than the seconde message.
  const char* except_res = "SAME";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 31: Test a sub filed treat as Map. Map is different
TEST(Differential_unit, map_1){

  employee message_first;
  employee message_second;
   

  education_info* edu_info_1 = message_first.add_education();
  edu_info_1->set_name("University of Dayton");
  edu_info_1->set_degree("PhD");
  edu_info_1->set_major("Computer Science");
  edu_info_1->set_address("OH, US");

  education_info* edu_info_2 = message_second.add_education();
  edu_info_2->set_name("Wright State University");
  edu_info_2->set_degree("PhD");
  edu_info_2->set_major("Computer Science");
  edu_info_2->set_address("OH, US");

  // Write the message to log message.
    DiffMsgRequest diff_request = Client_util::WriteMsgToDiffRequest(message_first,
                                                                   message_second);

  // Set the map value for repeated filed compare.
  std::string map_field_name = "education";
  std::vector<std::string> sub_field_list;
  sub_field_list.push_back("name");
  Client_util::TreatRepeatedFieldAsMap(diff_request, map_field_name, sub_field_list);

  // Implements the differential service.
   DiffMsgReply diff_reply = TestStub_->DifferentialService(diff_request);

  // Get the test result.
  const std::string& test_res = diff_reply.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // Map value is different replace the Map-value.
  const char* except_res = "added: education[0]: { name: \"Wright State University\" degree: \"PhD\" major: \"Computer Science\" address: \"OH, US\" }\n"
      "deleted: education[0]: { name: \"University of Dayton\" degree: \"PhD\" major: \"Computer Science\" address: \"OH, US\" }\n";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 32: Test a sub filed treat as Map. Value is different
TEST(Differential_unit, map_2){

  employee message_first;
  employee message_second;
   

  education_info* edu_info_1 = message_first.add_education();
  edu_info_1->set_name("Wright State University");
  edu_info_1->set_degree("PhD");
  edu_info_1->set_major("Computer Science");
  edu_info_1->set_address("OH, US");

  education_info* edu_info_2 = message_second.add_education();
  edu_info_2->set_name("Wright State University");
  edu_info_2->set_degree("Master of Science");
  edu_info_2->set_major("Computer Science and Engineering");
  edu_info_2->set_address("OH, US");

  // Write the message to log message.
    DiffMsgRequest diff_request = Client_util::WriteMsgToDiffRequest(message_first,
                                                                   message_second);

  // Set the map value for repeated filed compare.
  std::string map_field_name = "education";
  std::vector<std::string> sub_field_list;
  sub_field_list.push_back("name");
  Client_util::TreatRepeatedFieldAsMap(diff_request, map_field_name, sub_field_list);

  // Implements the differential service.
   DiffMsgReply diff_reply = TestStub_->DifferentialService(diff_request);

  // Get the test result.
  const std::string& test_res = diff_reply.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // The first message needs adding multiple elements than the seconde message.
  const char* except_res = "modified: education[0].degree: \"PhD\" -> \"Master of Science\"\n"
      "modified: education[0].major: \"Computer Science\" -> \"Computer Science and Engineering\"\n";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 33: Test a sub filed treat as Map. Map and Value are different at same time.
TEST(Differential_unit, map_3){

  employee message_first;
  employee message_second;
   

  education_info* edu_info_1 = message_first.add_education();
  edu_info_1->set_name("Wright State University");
  edu_info_1->set_degree("PhD");
  edu_info_1->set_major("Computer Science");
  edu_info_1->set_address("OH, US");

  education_info* edu_info_2 = message_second.add_education();
  edu_info_2->set_name("University of Dayton");
  edu_info_2->set_degree("Master of Science");
  edu_info_2->set_major("Computer Science and Engineering");
  edu_info_2->set_address("OH, US");

  // Write the message to log message.
    DiffMsgRequest diff_request = Client_util::WriteMsgToDiffRequest(message_first,
                                                                   message_second);

  // Set the map value for repeated filed compare.
  std::string map_field_name = "education";
  std::vector<std::string> sub_field_list;
  sub_field_list.push_back("name");
  Client_util::TreatRepeatedFieldAsMap(diff_request, map_field_name, sub_field_list);

  // Implements the differential service.
   DiffMsgReply diff_reply = TestStub_->DifferentialService(diff_request);

  // Get the test result.
  const std::string& test_res = diff_reply.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // map value different add and delete.
  const char* except_res = "added: education[0]: { name: \"University of Dayton\" degree: \"Master of Science\" major: \"Computer Science and Engineering\" address: \"OH, US\" }\n"
      "deleted: education[0]: { name: \"Wright State University\" degree: \"PhD\" major: \"Computer Science\" address: \"OH, US\" }\n";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 34: Test multiple sub fields as Map. Map is different.
TEST(Differential_unit, map_4){

  employee message_first;
  employee message_second;
   

  education_info* edu_info_1 = message_first.add_education();
  edu_info_1->set_name("Wright State University");
  edu_info_1->set_degree("PhD");
  edu_info_1->set_major("Computer Science");
  edu_info_1->set_address("OH, US");

  education_info* edu_info_2 = message_second.add_education();
  edu_info_2->set_name("University of Dayton");
  edu_info_2->set_degree("PhD");
  edu_info_2->set_major("Computer Science");
  edu_info_2->set_address("OH, US");

  // Write the message to log message.
    DiffMsgRequest diff_request = Client_util::WriteMsgToDiffRequest(message_first,
                                                                   message_second);

  // Set the map value for repeated filed compare.
  std::string map_field_name = "education";
  std::vector<std::string> sub_field_list;
  sub_field_list.push_back("name");
  sub_field_list.push_back("degree");
  Client_util::TreatRepeatedFieldAsMap(diff_request, map_field_name, sub_field_list);

  // Implements the differential service.
   DiffMsgReply diff_reply = TestStub_->DifferentialService(diff_request);

  // Get the test result.
  const std::string& test_res = diff_reply.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // Map value different
  const char* except_res = "added: education[0]: { name: \"University of Dayton\" degree: \"PhD\" major: \"Computer Science\" address: \"OH, US\" }\n"
      "deleted: education[0]: { name: \"Wright State University\" degree: \"PhD\" major: \"Computer Science\" address: \"OH, US\" }\n";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 35: Test multiple sub fields as Map. Value is different.
TEST(Differential_unit, map_5){

  employee message_first;
  employee message_second;
   

  education_info* edu_info_1 = message_first.add_education();
  edu_info_1->set_name("Wright State University");
  edu_info_1->set_degree("PhD");
  edu_info_1->set_major("Computer Science");
  edu_info_1->set_address("OH, US");

  education_info* edu_info_2 = message_second.add_education();
  edu_info_2->set_name("Wright State University");
  edu_info_2->set_major("Computer Science and Engineering");
  edu_info_2->set_address("OH, US");
  edu_info_2->set_degree("PhD");

  // Write the message to log message.
    DiffMsgRequest diff_request = Client_util::WriteMsgToDiffRequest(message_first,
                                                                   message_second);

  // Set the map value for repeated filed compare.
  std::string map_field_name = "education";
  std::vector<std::string> sub_field_list;
  sub_field_list.push_back("name");
  sub_field_list.push_back("degree");
  Client_util::TreatRepeatedFieldAsMap(diff_request, map_field_name, sub_field_list);

  // Implements the differential service.
   DiffMsgReply diff_reply = TestStub_->DifferentialService(diff_request);

  // Get the test result.
  const std::string& test_res = diff_reply.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // The first message needs adding multiple elements than the seconde message.
  const char* except_res = "modified: education[0].major: \"Computer Science\" -> \"Computer Science and Engineering\"\n";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 36: Test multiple sub fields as Map. Map and Value are different.
TEST(Differential_unit, map_6){

  employee message_first;
  employee message_second;
   

  education_info* edu_info_1 = message_first.add_education();
  edu_info_1->set_name("Wright State University");
  edu_info_1->set_degree("PhD");
  edu_info_1->set_major("Computer Science");
  edu_info_1->set_address("OH, US");

  education_info* edu_info_2 = message_second.add_education();
  edu_info_2->set_name("University of Dayton");
  edu_info_2->set_major("Computer Science and Engineering");
  edu_info_2->set_address("OH, US");
  edu_info_2->set_degree("PhD");


  // Write the message to log message.
    DiffMsgRequest diff_request = Client_util::WriteMsgToDiffRequest(message_first,
                                                                   message_second);

  // Set the map value for repeated filed compare.
  std::string map_field_name = "education";
  std::vector<std::string> sub_field_list;
  sub_field_list.push_back("name");
  sub_field_list.push_back("degree");
  Client_util::TreatRepeatedFieldAsMap(diff_request, map_field_name, sub_field_list);

  // Implements the differential service.
   DiffMsgReply diff_reply = TestStub_->DifferentialService(diff_request);

  // Get the test result.
  const std::string& test_res = diff_reply.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // The first message needs adding multiple elements than the seconde message.
  const char* except_res = "added: education[0]: { name: \"University of Dayton\" degree: \"PhD\" major: \"Computer Science and Engineering\" address: \"OH, US\" }\n"
      "deleted: education[0]: { name: \"Wright State University\" degree: \"PhD\" major: \"Computer Science\" address: \"OH, US\" }\n";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 37: Test multiple sub fields as Map. message 1 is empty
TEST(Differential_unit, map_7){

  employee message_first;
  employee message_second;
   

  education_info* edu_info_1 = message_first.add_education();

  education_info* edu_info_2 = message_second.add_education();
  edu_info_2->set_name("University of Dayton");
  edu_info_2->set_major("Computer Science and Engineering");
  edu_info_2->set_address("OH, US");
  edu_info_2->set_degree("PhD");


  // Write the message to log message.
    DiffMsgRequest diff_request = Client_util::WriteMsgToDiffRequest(message_first,
                                                                   message_second);

  // Set the map value for repeated filed compare.
  std::string map_field_name = "education";
  std::vector<std::string> sub_field_list;
  sub_field_list.push_back("name");
  sub_field_list.push_back("degree");
  Client_util::TreatRepeatedFieldAsMap(diff_request, map_field_name, sub_field_list);

  // Implements the differential service.
   DiffMsgReply diff_reply = TestStub_->DifferentialService(diff_request);

  // Get the test result.
  const std::string& test_res = diff_reply.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // The first message needs adding multiple elements than the seconde message.
  const char* except_res = "added: education[0]: { name: \"University of Dayton\" "
      "degree: \"PhD\" "
      "major: \"Computer Science and Engineering\" "
      "address: \"OH, US\" }\ndeleted: education[0]: { }\n";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 38: Test multiple sub fields as Map. message 2 is empty
TEST(Differential_unit, map_8){

  employee message_first;
  employee message_second;
   

  education_info* edu_info_1 = message_first.add_education();
  edu_info_1->set_name("Wright State University");
  edu_info_1->set_degree("PhD");
  edu_info_1->set_major("Computer Science");
  edu_info_1->set_address("OH, US");

  education_info* edu_info_2 = message_second.add_education();

  // Write the message to log message.
    DiffMsgRequest diff_request = Client_util::WriteMsgToDiffRequest(message_first,
                                                                   message_second);

  // Set the map value for repeated filed compare.
  std::string map_field_name = "education";
  std::vector<std::string> sub_field_list;
  sub_field_list.emplace_back("name");
  sub_field_list.emplace_back("degree");
  Client_util::TreatRepeatedFieldAsMap(diff_request, map_field_name, sub_field_list);


  // Implements the differential service.
   DiffMsgReply diff_reply = TestStub_->DifferentialService(diff_request);

  // Get the test result.
  const std::string& test_res = diff_reply.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // The first message needs adding multiple elements than the second message.
  const char* except_res = "added: education[0]: { }\n"
      "deleted: education[0]: { name: \"Wright State University\" degree: \"PhD\" major: \"Computer Science\" address: \"OH, US\" }\n";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 39 - 41: Test the fraction and margin for float number comparison
TEST(Differential_unit, float_and_double_1){

  employee message_first;
  employee message_second;
   

  message_first.set_floatpoint(100.0f);
  message_second.set_floatpoint(109.9f);

  // Write the message to log message.
  DiffMsgRequest diff_request = Client_util::WriteMsgToDiffRequest(message_first,
                                                                   message_second);

  //1)
  // Should fail since the fraction is smaller than error.
  Client_util::SetFractionAndMargin(diff_request, 0.01, 0.0);

  // Implements the differential service.
   DiffMsgReply diff_reply = TestStub_->DifferentialService(diff_request);

  // Get the test result.
  const std::string& test_res = diff_reply.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // should fail.
  const char* except_res = "modified: floatpoint: 100 -> 109.90000152587891\n";

  EXPECT_STREQ(c_test_res, except_res);

  // 2)
  // Test out float comparison with fraction.
  Client_util::SetFractionAndMargin(diff_request, 0.2, 0.0);

  // Implements the differential service.
  DiffMsgReply diff_reply_1 = TestStub_->DifferentialService(diff_request);

  // Get the test result.
  const std::string& test_res_1 = diff_reply_1.result();

  const char* c_test_res_1 = test_res_1.c_str();

  // should same.
  const char* except_res_1 = "SAME";

  EXPECT_STREQ(c_test_res_1, except_res_1);

  // 3)
  // Test out float comparison with fraction.
  Client_util::SetFractionAndMargin(diff_request, 0.01, 10.0);

  // Implements the differential service.
  DiffMsgReply diff_reply_2 = TestStub_->DifferentialService(diff_request);

  // Get the test result.
  const std::string& test_res_2 = diff_reply_2.result();


  const char* c_test_res_2 = test_res_2.c_str();

  // should same.
  const char* except_res_2 = "SAME";
  EXPECT_STREQ(c_test_res_2, except_res_2);
}

// Test 42 - 44: Test the fraction and margin for double number comparison
TEST(Differential_unit, float_and_double_2){

  employee message_first;
  employee message_second;


  message_first.set_floatpoint(100.0);
  message_second.set_floatpoint(109.9);

  // Write the message to log message.
  DiffMsgRequest diff_request = Client_util::WriteMsgToDiffRequest(message_first,
                                                                   message_second);

  //1)
  // Should fail since the fraction is smaller than error.
  Client_util::SetFractionAndMargin(diff_request, 0.01, 0.0);

  // Implements the differential service.
  DiffMsgReply diff_reply = TestStub_->DifferentialService(diff_request);

  // Get the test result.
  const std::string& test_res = diff_reply.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // should fail.
  const char* except_res = "modified: floatpoint: 100 -> 109.9\n";

  EXPECT_STREQ(c_test_res, except_res);

  // 2)
  // Test out float comparison with fraction.
  Client_util::SetFractionAndMargin(diff_request, 0.2, 0.0);

  // Implements the differential service.
  DiffMsgReply diff_reply_1 = TestStub_->DifferentialService(diff_request);

  // Get the test result.
  const std::string& test_res_1 = diff_reply_1.result();

  const char* c_test_res_1 = test_res_1.c_str();

  // should same.
  const char* except_res_1 = "SAME";

  EXPECT_STREQ(c_test_res_1, except_res_1);

  // 3)
  // Test out float comparison with fraction.
  Client_util::SetFractionAndMargin(diff_request, 0.01, 10.0);

  // Implements the differential service.
  DiffMsgReply diff_reply_2 = TestStub_->DifferentialService(diff_request);

  // Get the test result.
  const std::string& test_res_2 = diff_reply_2.result();


  const char* c_test_res_2 = test_res_2.c_str();

  // should same.
  const char* except_res_2 = "SAME";
  EXPECT_STREQ(c_test_res_2, except_res_2);
}

// Test 45: Test the ignore field isolated from repeated field set.
TEST(Differential_unit, ignore_isolate_1){

  employee message_first;
  employee message_second;
   

  message_first.set_fullname("Jin Huang");
  // Write two message with same value and same order.
  message_first.add_areas("Google Ads.");
  message_first.add_areas("YouTube Ads.");
  message_first.add_areas("Search Ads.");
  message_first.add_areas("Click Ads.");

  message_second.set_fullname("Zhe Liu");
  message_second.add_areas("Google Ads.");
  message_second.add_areas("YouTube Ads.");
  message_second.add_areas("Search Ads.");
  message_second.add_areas("Click Ads.");

  // Write the message to log message.
    DiffMsgRequest diff_request = Client_util::WriteMsgToDiffRequest(message_first,
                                                                   message_second);

  // ignore the areas field
  std::vector<std::string> fields;
  std::string field_1("differential_test.employee.areas");
  fields.push_back(field_1);
  Client_util::IgnoreFields(diff_request, fields);

  // Set the field areas as list
  std::string repeated_field = "areas";
  Client_util::TreatRepeatedFieldAsListOrSet(diff_request, 0, repeated_field);

  // Implements the differential service.
   DiffMsgReply diff_reply = TestStub_->DifferentialService(diff_request);

  // Get the test result.
  const std::string& test_res = diff_reply.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // Because we ignore the area field so differential only at fullname will presented.
  const char* except_res = "modified: fullname: \"Jin Huang\" -> \"Zhe Liu\"\n";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 46: Test the ignore field isolated from repeated field set.
TEST(Differential_unit, ignore_isolate_2){

  employee message_first;
  employee message_second;
   

  message_first.set_fullname("Jin Huang");
  // Write two message with same value and same order.
  message_first.add_areas("Google Ads.");
  message_first.add_areas("YouTube Ads.");
  message_first.add_areas("Search Ads.");
  message_first.add_areas("Click Ads.");

  message_second.set_fullname("Zhe Liu");
  message_second.add_areas("Google Ads.");
  message_second.add_areas("YouTube Ads.");
  message_second.add_areas("Search Ads.");
  message_second.add_areas("Click Ads.");

  education_info* edu_info_1 = message_first.add_education();
  edu_info_1->set_name("University of Dayton");
  edu_info_1->set_degree("PhD");
  edu_info_1->set_major("Computer Science");
  edu_info_1->set_address("OH, US");

  education_info* edu_info_2 = message_second.add_education();
  edu_info_2->set_name("Wright State University");
  edu_info_2->set_degree("PhD");
  edu_info_2->set_major("Computer Science");
  edu_info_2->set_address("OH, US");

  // Write the message to log message.
    DiffMsgRequest diff_request = Client_util::WriteMsgToDiffRequest(message_first,
                                                                   message_second);

  // ignore the areas field
  std::vector<std::string> fields;
  std::string field_1("differential_test.employee.education");
  std::string field_2("differential_test.employee.areas");
  fields.push_back(field_1);
  fields.push_back(field_2);
  Client_util::IgnoreFields(diff_request, fields);

  // Set the field areas as list
  std::string repeated_field = "areas";
  Client_util::TreatRepeatedFieldAsListOrSet(diff_request, 0, repeated_field);

  // Set the map value for repeated filed compare.
  std::string map_field_name = "education";
  std::vector<std::string> sub_field_list;
  sub_field_list.push_back("name");
  Client_util::TreatRepeatedFieldAsMap(diff_request, map_field_name, sub_field_list);


  // Implements the differential service.
   DiffMsgReply diff_reply = TestStub_->DifferentialService(diff_request);

  // Get the test result.
  const std::string& test_res = diff_reply.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // Because we ignore the area field so differential only at fullname will presented.
  const char* except_res = "modified: fullname: \"Jin Huang\" -> \"Zhe Liu\"\n";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 47: Test the differential service comprehensively
TEST(Differential_unit, total_test_1){

  employee message_first;
  employee message_second;
   

  // write the message 1.
  message_first.set_employ_id(001);
  message_first.set_fullname("Jin Huang");
  message_first.set_age(32);

  company* company_ptr = message_first.mutable_employer();
  company_ptr->set_name("Google Inc.");
  company_ptr->set_occupation("Softwear Engineer");
  company_ptr->set_address("CA, US");

    // set the working areas
  message_first.add_areas("Google Ads.");
  message_first.add_areas("YouTube Ads.");
  message_first.add_areas("Search Ads.");
  message_first.add_areas("Click Ads.");

    // set the education info No. 1
  education_info* edu_info_1 = message_first.add_education();
  edu_info_1->set_name("Wright State University");
  edu_info_1->set_degree("PhD");
  edu_info_1->set_major("Computer Science");
  edu_info_1->set_address("OH, US");

  // set the dependent info
  differential_test::dependent_info* dependentInfo_ptr_1 =
      message_first.mutable_dependents();
  dependentInfo_ptr_1->add_name("Jeremy");
  dependentInfo_ptr_1->add_name("Zhe");
  dependentInfo_ptr_1->add_name("Jin");
  dependentInfo_ptr_1->add_name("June");
  dependentInfo_ptr_1->add_age(29);
  dependentInfo_ptr_1->add_age(30);
  dependentInfo_ptr_1->add_age(32);
  dependentInfo_ptr_1->add_age(26);

  // Set the double number
  double num1 = 100.0;
  message_first.set_floatpoint(num1);

//  write message 2
  message_second.set_employ_id(002);
  message_second.set_fullname("Zhe Liu");
  message_second.set_age(32);

  company* company_ptr_2 = message_second.mutable_employer();
  company_ptr_2->set_name("Google Inc.");
  company_ptr_2->set_occupation("Softweare Engineer");
  company_ptr_2->set_address("CA, US");

  // set the working areas
  message_second.add_areas("Google Ads.");
  message_second.add_areas("YouTube Ads.");
  message_second.add_areas("Search Ads.");
  message_second.add_areas("Click Ads.");

  // set the education info No. 1
  education_info* edu_info_2 = message_second.add_education();
  edu_info_2->set_name("Wright State University");
  edu_info_2->set_degree("PhD");
  edu_info_2->set_major("Computer Science");
  edu_info_2->set_address("OH, US");

  // set the dependent info
  differential_test::dependent_info* dependentInfo_ptr_2 =
      message_second.mutable_dependents();
  dependentInfo_ptr_2->add_name("Jeremy");
  dependentInfo_ptr_2->add_name("June");
  dependentInfo_ptr_2->add_name("Jin");
  dependentInfo_ptr_2->add_name("Zhe");
  dependentInfo_ptr_2->add_age(29);
  dependentInfo_ptr_2->add_age(30);
  dependentInfo_ptr_2->add_age(26);
  dependentInfo_ptr_2->add_age(32);

  // Set the double number
  double num2 = 100.0;
  message_second.set_floatpoint(num2);

  // Write the message to log message.
    DiffMsgRequest diff_request = Client_util::WriteMsgToDiffRequest(message_first,
                                                                   message_second);

  // add ignore fields
  std::vector<std::string> ignore_fields;
  std::string field_1("differential_test.employee.employ_id");
  std::string field_2("differential_test.company.address");
  std::string field_3("differential_test.education_info.address");

  ignore_fields.push_back(field_1);
  ignore_fields.push_back(field_2);
  ignore_fields.push_back(field_3);
  Client_util::IgnoreFields(diff_request, ignore_fields);

  // Set the field areas as list
  std::string repeated_field_1("areas");
  Client_util::TreatRepeatedFieldAsListOrSet(diff_request, 0, repeated_field_1);

  // Set the field dependents.name as set
  std::string repeated_field_2("dependents.name");
  Client_util::TreatRepeatedFieldAsListOrSet(diff_request, 1, repeated_field_2);

  // Set the field dependents.age as set
  std::string repeated_field_3("dependents.age");
  Client_util::TreatRepeatedFieldAsListOrSet(diff_request, 1, repeated_field_3);

  // Set the map value comparison
  std::string map_field_name("education");
  std::vector<std::string> sub_field_list;
  sub_field_list.emplace_back("name");
  sub_field_list.emplace_back("degree");
  Client_util::TreatRepeatedFieldAsMap(diff_request, map_field_name, sub_field_list);

  // Implements the differential service.
   DiffMsgReply diff_reply = TestStub_->DifferentialService(diff_request);

  // Get the test result.
  const std::string& test_res = diff_reply.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  const char* except_res = "modified: fullname: \"Jin Huang\" -> \"Zhe Liu\"\n"
      "modified: employer.occupation: \"Softwear Engineer\" -> \"Softweare Engineer\"\n";

  EXPECT_STREQ(c_test_res, except_res);
}



// TEST end line.
}


int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  // Create a global test environment for all test cases.
  testing::AddGlobalTestEnvironment(new DifferentialTestEnvironment());
  return RUN_ALL_TESTS();
}

