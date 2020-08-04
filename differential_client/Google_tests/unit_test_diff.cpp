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

#include "../client_util.h"
#include "../differential_service_client.h"
#include "differential_service.grpc.pb.h"
#include "differential_test.grpc.pb.h"
#include "gtest/gtest.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using grpc::StatusCode;

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

using DifferentialService::DiffRequest;
using DifferentialService::DiffResponse;
using DifferentialService::MsgReply;
using DifferentialService::MsgRequest;
using DifferentialService::ServerDifferential;

using DifferentialTest::Company;
using DifferentialTest::DependentInfo;
using DifferentialTest::EducationInfo;
using DifferentialTest::ExamScore;
using DifferentialTest::TestEmployee;

namespace {
// Initial the test stub for testing the differential service
DifferentialServiceClient TestStub_;
std::string server_address;
}  // namespace

// Create the test environment of the differential service.
class DifferentialTestEnvironment : public testing::Environment {
 public:
  explicit DifferentialTestEnvironment() { server_address = "0.0.0.0:50053"; }
};

namespace google {

/*
 * In this Unit Test, The message "TestEmployee" was used to generate the input
 * message that was defined in the file DifferentialTest.proto. (Link:
 * https://github.com/jinhuangzheliu/gRPC-Differential-Service/blob/master/protos/DifferentialTest.proto)
 */

// Test 1: Test the default differential service.
TEST(DifferentialUnitTest, test_the_basic_diff_service) {
  // Generate two message by message type TestEmployee(defined in
  // DifferentialTest.proto)
  TestEmployee message_first;
  TestEmployee message_second;

  // write two messages
  message_first.set_fullname("Jin Huang");
  message_first.set_age(32);

  message_second.set_fullname("Zhe Liu");
  message_second.set_age(32);

  // Generate the differential request
  DiffRequest diff_request =
      ClientUtil::WriteMsgToDiffRequest(message_first, message_second);

  // Receive the differential response.
  DiffResponse diff_response;

  // Request the differential service.
  StatusCode service_response_status = TestStub_.CompareInputMessages(
      diff_request, &diff_response, server_address);

  ASSERT_TRUE(StatusCode::OK == service_response_status);

  // Get the test result.
  const std::string& test_res = diff_response.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // Only fullname is different
  const char* except_res = "modified: fullname: \"Jin Huang\" -> \"Zhe Liu\"\n";
  EXPECT_STREQ(c_test_res, except_res);
}

// Test 2: Test the nested field
TEST(DifferentialUnitTest, test_the_basic_diff_service_by_nested_field) {
  // Generate two message by message type TestEmployee(defined in
  // DifferentialTest.proto)
  TestEmployee message_first;
  TestEmployee message_second;

  // write two messages
  Company* company_ptr_1 = message_first.mutable_employer();
  company_ptr_1->set_name("Google Inc.");
  company_ptr_1->set_occupation("Intern");
  company_ptr_1->set_address("CA, U.S.");

  Company* company_ptr_2 = message_second.mutable_employer();
  company_ptr_2->set_name("Alphabet Inc.");
  company_ptr_2->set_occupation("Software Engineer");
  company_ptr_2->set_address("CA, U.S.");

  // Generate the differential message request
  DiffRequest diff_request =
      ClientUtil::WriteMsgToDiffRequest(message_first, message_second);

  // Receive the differential response.
  DiffResponse diff_response;

  // Request the differential service.
  StatusCode service_response_status = TestStub_.CompareInputMessages(
      diff_request, &diff_response, server_address);

  // Check the return Status
  ASSERT_TRUE(StatusCode::OK == service_response_status);

  // Get the test result.
  const std::string& test_res = diff_response.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // Only the company name and occupation are different
  const char* except_res =
      "modified: employer.name: \"Google Inc.\" -> \"Alphabet Inc.\"\n"
      "modified: employer.occupation: \"Intern\" -> \"Software Engineer\"\n";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 3: Test the wrong connection address. Service will return StatusCode::UNAVAILABLE
TEST(DifferentialUnitTest, test_error_address) {
  TestEmployee message_first;
  TestEmployee message_second;

  // write two messages
  message_first.set_fullname("Jin Huang");
  message_first.set_age(32);

  message_second.set_fullname("Zhe Liu");
  message_second.set_age(32);

  const std::string test_address = "0.0.0.0:50052";

  // Generate the differential request
  DiffRequest diff_request = ClientUtil::WriteMsgToDiffRequest(message_first, message_second);

  // Receive the differential response.
  DiffResponse diff_response;

  // Request the differential service.
  StatusCode service_response_status = TestStub_.CompareInputMessages(diff_request, &diff_response, test_address);

  ASSERT_TRUE(StatusCode::UNAVAILABLE == service_response_status);
}

// Test 4: Test the maximum size of input message.
TEST(DifferentialUnitTest, test_maximum_size_of_input_message_with_10000_repeated_field) {
  /*
   *  In this testing case, we will try to input two same messages with 50000
   *  repeated "education" fields.
   */
  TestEmployee message_first;
  TestEmployee message_second;

  for (int i = 0; i < 10000; ++i) {
    int num = i;
    std::string name_1 = "A";
    name_1 = name_1 + "_" + std::to_string(num);

    std::string name_2 = "A";
    name_2 = name_2 + "_" + std::to_string(num);

    std::string degree = "PhD";
    std::string major = "Computer Science";
    std::string address = "CA, US";

    EducationInfo* edu_1 = message_first.add_education();
    edu_1->set_name(name_1);
    edu_1->set_degree(degree);
    edu_1->set_major(major);
    edu_1->set_address(address);

    EducationInfo* edu_2 = message_second.add_education();
    edu_2->set_name(name_2);
    edu_2->set_degree(degree);
    edu_2->set_major(major);
    edu_2->set_address(address);
  }

  // Write the message to log message.
  DiffRequest diff_request =
      ClientUtil::WriteMsgToDiffRequest(message_first, message_second);

  // Receive the differential response.
  DiffResponse diff_response;

  // Request the differential service.
  StatusCode service_response_status = TestStub_.CompareInputMessages(
      diff_request, &diff_response, server_address);

  // Check the return Status
  ASSERT_TRUE(StatusCode::OK == service_response_status);

  // Get the test result.
  const std::string& test_res = diff_response.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  const char* except_res = "SAME";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 5: Test the maximum size of input message.
TEST(DifferentialUnitTest, test_maximum_size_of_input_message_with_50000_repeated_field) {
  /*
   *  In this testing case, we will try to input two same messages with 50000
   *  repeated "education" fields.
   */
  TestEmployee message_first;
  TestEmployee message_second;

  for (int i = 0; i < 50000; ++i) {
    int num = i;
    std::string name_1 = "A";
    name_1 = name_1 + "_" + std::to_string(num);

    std::string name_2 = "A";
    name_2 = name_2 + "_" + std::to_string(num);

    std::string degree = "PhD";
    std::string major = "Computer Science";
    std::string address = "CA, US";

    EducationInfo* edu_1 = message_first.add_education();
    edu_1->set_name(name_1);
    edu_1->set_degree(degree);
    edu_1->set_major(major);
    edu_1->set_address(address);

    EducationInfo* edu_2 = message_second.add_education();
    edu_2->set_name(name_2);
    edu_2->set_degree(degree);
    edu_2->set_major(major);
    edu_2->set_address(address);
  }

  // Write the message to log message.
  DiffRequest diff_request =
      ClientUtil::WriteMsgToDiffRequest(message_first, message_second);

  // Receive the differential response.
  DiffResponse diff_response;

  // Request the differential service.
  StatusCode service_response_status = TestStub_.CompareInputMessages(
      diff_request, &diff_response, server_address);

  // Check the return Status
  ASSERT_TRUE(StatusCode::OK == service_response_status);

  // Get the test result.
  const std::string& test_res = diff_response.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  const char* except_res = "SAME";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 6: Test the maximum size of input message.
TEST(DifferentialUnitTest, test_maximum_size_of_input_message_with_100000_repeated_field) {
  /*
   *  In this testing case, we will try to input two same messages with 50000
   *  repeated "education" fields.
   */
  TestEmployee message_first;
  TestEmployee message_second;

  for (int i = 0; i < 100000; ++i) {
    int num = i;
    std::string name_1 = "A";
    name_1 = name_1 + "_" + std::to_string(num);

    std::string name_2 = "A";
    name_2 = name_2 + "_" + std::to_string(num);

    std::string degree = "PhD";
    std::string major = "Computer Science";
    std::string address = "CA, US";

    EducationInfo* edu_1 = message_first.add_education();
    edu_1->set_name(name_1);
    edu_1->set_degree(degree);
    edu_1->set_major(major);
    edu_1->set_address(address);

    EducationInfo* edu_2 = message_second.add_education();
    edu_2->set_name(name_2);
    edu_2->set_degree(degree);
    edu_2->set_major(major);
    edu_2->set_address(address);
  }

  // Write the message to log message.
  DiffRequest diff_request =
      ClientUtil::WriteMsgToDiffRequest(message_first, message_second);

  // Receive the differential response.
  DiffResponse diff_response;

  // Request the differential service.
  StatusCode service_response_status = TestStub_.CompareInputMessages(
      diff_request, &diff_response, server_address);

  // Check the return Status
  ASSERT_TRUE(StatusCode::INVALID_ARGUMENT == service_response_status);
}

// Test 7: Test the single field ignore.
TEST(DifferentialUnitTest, test_ignore_one_field) {
  // Generate two message by message type TestEmployee(defined in
  // DifferentialTest.proto)
  TestEmployee message_first;
  TestEmployee message_second;

  // write two messages
  message_first.set_fullname("Jin Huang");
  message_first.set_age(32);

  message_second.set_fullname("Zhe Liu");
  message_second.set_age(32);

  // Generate the differential message request
  DiffRequest diff_request =
      ClientUtil::WriteMsgToDiffRequest(message_first, message_second);

  // Try to ignore the employee's fullname.
  std::vector<std::string> fields;
  std::string field_1("DifferentialTest.TestEmployee.fullname");
  fields.push_back(field_1);
  ClientUtil::IgnoreFields(&diff_request, fields);

  // Receive the differential response.
  DiffResponse diff_response;

  // Request the differential service.
  StatusCode service_response_status = TestStub_.CompareInputMessages(
      diff_request, &diff_response, server_address);

  // Check the return Status
  ASSERT_TRUE(StatusCode::OK == service_response_status);

  // Get the test result.
  const std::string& test_res = diff_response.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // the fullname field was ignored so return is same.
  const char* except_res = "SAME";

  EXPECT_STREQ(c_test_res, except_res);

  // Check the result error
  const std::string& test_error = diff_response.error();
  const char* c_test_error = test_error.c_str();
  const char* except_error = "";

  EXPECT_STREQ(c_test_error, except_error);
}

// Test 8: Test the multiple fields ignore
TEST(DifferentialUnitTest, test_ignore_two_fields) {
  // Generate two message by message type TestEmployee(defined in
  // DifferentialTest.proto)
  TestEmployee message_first;
  TestEmployee message_second;

  // write two messages
  message_first.set_employ_id(001);
  message_first.set_fullname("Jin Huang");
  message_first.set_age(39);

  message_second.set_employ_id(002);
  message_second.set_fullname("Zhe Liu");
  message_second.set_age(32);

  // Generate the differential message request
  DiffRequest diff_request =
      ClientUtil::WriteMsgToDiffRequest(message_first, message_second);

  std::vector<std::string> fields;
  std::string field_1("DifferentialTest.TestEmployee.fullname");
  std::string field_2("DifferentialTest.TestEmployee.employ_id");

  fields.push_back(field_1);
  fields.push_back(field_2);

  // Add the ignore field to the diff request
  ClientUtil::IgnoreFields(&diff_request, fields);

  // Receive the differential response.
  DiffResponse diff_response;

  // Request the differential service.
  StatusCode service_response_status = TestStub_.CompareInputMessages(
      diff_request, &diff_response, server_address);

  // Check the return Status
  ASSERT_TRUE(StatusCode::OK == service_response_status);

  // Get the test result.
  const std::string& test_res = diff_response.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // fullname and employ_id were ignored but age is different
  const char* except_res = "modified: age: 39 -> 32\n";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 9: Test the multiple fields ignore include the nested field.
TEST(DifferentialUnitTest, test_ignore_nested_fields) {
  // Generate two message by message type TestEmployee(defined in
  // DifferentialTest.proto)
  TestEmployee message_first;
  TestEmployee message_second;

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
  DiffRequest diff_request =
      ClientUtil::WriteMsgToDiffRequest(message_first, message_second);

  //******************** Test Content ****************************************

  // Add the ignore fields. we add two more field nested under the message
  // TestEmployee.
  std::vector<std::string> ignore_fields;
  std::string field_1("DifferentialTest.TestEmployee.fullname");
  std::string field_2("DifferentialTest.TestEmployee.employ_id");

  std::string nested_field_1("DifferentialTest.Company.name");
  std::string nested_field_2("DifferentialTest.Company.occupation");

  // push the fields
  ignore_fields.push_back(field_1);
  ignore_fields.push_back(field_2);

  // push the nested fields.
  ignore_fields.push_back(nested_field_1);
  ignore_fields.push_back(nested_field_2);

  // Set the ignore to differential request
  ClientUtil::IgnoreFields(&diff_request, ignore_fields);

  // ********************** Differential Service **********************
  // Receive the differential response.
  DiffResponse diff_response;

  // Request the differential service.
  StatusCode service_response_status = TestStub_.CompareInputMessages(
      diff_request, &diff_response, server_address);

  // Check the return Status
  ASSERT_TRUE(StatusCode::OK == service_response_status);

  // Get the test result.
  const std::string& test_res = diff_response.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // ******************** Prospective result **********************
  // Test the nested field ignore Criteria. The different between the nested
  // fields are ignored so only the different in age should be presented.
  const char* except_res = "modified: age: 39 -> 32\n";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 10: Test ignore nothing
TEST(DifferentialUnitTest, test_ignore_nothing) {
  // Generate two message by message type TestEmployee(defined in
  // DifferentialTest.proto)
  TestEmployee message_first;
  TestEmployee message_second;

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
  DiffRequest diff_request =
      ClientUtil::WriteMsgToDiffRequest(message_first, message_second);

  //******************** Test Content ****************************************
  // Add the ignore fields. we add two more field nested under the message
  // TestEmployee.
  std::vector<std::string> ignore_fields;

  // Leave the ignore_fields as empty.

  // Write the ignore to differential request
  ClientUtil::IgnoreFields(&diff_request, ignore_fields);

  // ********************** Differential Service **********************
  // Receive the differential response.
  DiffResponse diff_response;

  // Request the differential service.
  StatusCode service_response_status = TestStub_.CompareInputMessages(
      diff_request, &diff_response, server_address);

  // Check the return Status
  ASSERT_TRUE(StatusCode::OK == service_response_status);

  // Get the test result.
  const std::string& test_res = diff_response.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // ******************** Prospective result **********************
  // Test if the ignore is empty.

  const char* except_res =
      "modified: employ_id: 1 -> 2\n"
      "modified: fullname: \"Jin Huang\" -> \"Zhe Liu\"\n"
      "modified: age: 39 -> 32\n"
      "modified: employer.name: \"Google Inc.\" -> \"Alphabet Inc.\"\n"
      "modified: employer.occupation: \"Software Engineer\" -> \"Research "
      "Scientist\"\n";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 11: Test ignore all fields in message.
TEST(DifferentialUnitTest, test_ignore_all_fields) {
  // Generate two message by message type TestEmployee(defined in
  // DifferentialTest.proto)
  TestEmployee message_first;
  TestEmployee message_second;

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
  DiffRequest diff_request =
      ClientUtil::WriteMsgToDiffRequest(message_first, message_second);

  //******************** Test Content ****************************************

  // Add the ignore fields. we add two more field nested under the message
  // TestEmployee.
  std::vector<std::string> ignore_fields;

  std::string field_1("DifferentialTest.TestEmployee.fullname");
  std::string field_2("DifferentialTest.TestEmployee.employ_id");
  std::string field_3("DifferentialTest.TestEmployee.age");

  std::string nested_field_1("DifferentialTest.Company.name");
  std::string nested_field_2("DifferentialTest.Company.occupation");
  std::string nested_field_3("DifferentialTest.Company.address");

  // push the fields
  ignore_fields.push_back(field_1);
  ignore_fields.push_back(field_2);
  ignore_fields.push_back(field_3);

  // push the nested fields.
  ignore_fields.push_back(nested_field_1);
  ignore_fields.push_back(nested_field_2);
  ignore_fields.push_back(nested_field_3);

  // Black list criteria will do the ignore
  ClientUtil::IgnoreFields(&diff_request, ignore_fields);

  // ********************** Differential Service **********************
  // Receive the differential response.
  DiffResponse diff_response;

  // Request the differential service.
  StatusCode service_response_status = TestStub_.CompareInputMessages(
      diff_request, &diff_response, server_address);

  // Check the return Status
  ASSERT_TRUE(StatusCode::OK == service_response_status);

  // Get the test result.
  const std::string& test_res = diff_response.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // ******************** Prospective result **********************
  // Test all fields are ignored so result should be same.
  const char* except_res = "SAME";
  EXPECT_STREQ(c_test_res, except_res);

  // Check the result error
  const std::string& test_error = diff_response.error();
  const char* c_test_error = test_error.c_str();
  const char* except_error = "";

  EXPECT_STREQ(c_test_error, except_error);
}

// Test 12: Beside setting ignore fields we also support the compare function to
//         only compare the specific fields (p.s. "Compare one field" means
//         expect the field you selected others filed will be ignored.)
TEST(DifferentialUnitTest, test_compare_one_field) {
  // Generate two message by message type TestEmployee(defined in
  // DifferentialTest.proto)
  TestEmployee message_first;
  TestEmployee message_second;

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
  DiffRequest diff_request =
      ClientUtil::WriteMsgToDiffRequest(message_first, message_second);

  //******************** Test Content ****************************************

  // Add the compare fields.
  std::vector<std::string> compare_fields;
  std::string field_1("DifferentialTest.TestEmployee.fullname");
  // push the fields
  compare_fields.push_back(field_1);

  // Set compare field in diff request.
  ClientUtil::CompareFields(&diff_request, compare_fields);

  // ********************** Differential Service **********************
  // Receive the differential response.
  DiffResponse diff_response;

  // Request the differential service.
  StatusCode service_response_status = TestStub_.CompareInputMessages(
      diff_request, &diff_response, server_address);

  // Check the return Status
  ASSERT_TRUE(StatusCode::OK == service_response_status);

  // Get the test result.
  const std::string& test_res = diff_response.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // ******************** Prospective result **********************
  // Only employee's fullname was compared
  const char* except_res = "modified: fullname: \"Jin Huang\" -> \"Zhe Liu\"\n";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 13: Test compare multiple fields.
TEST(DifferentialUnitTest, test_compare_more_fields) {
  // Generate two message by message type TestEmployee(defined in
  // DifferentialTest.proto)
  TestEmployee message_first;
  TestEmployee message_second;

  // ******************** write two messages **********************
  message_first.set_employ_id(001);
  message_first.set_fullname("Jin Huang");
  message_first.set_age(39);

  message_first.mutable_employer()->set_name("Google Inc.");

  // Set the message 2 by the following stuff.
  message_second.set_employ_id(002);
  message_second.set_fullname("Zhe Liu");
  message_second.set_age(32);

  message_second.mutable_employer()->set_name("Alphabet Inc.");

  // ***************** Generate the differential message request ***************
  DiffRequest diff_request =
      ClientUtil::WriteMsgToDiffRequest(message_first, message_second);

  //******************** Test Content ****************************************
  // Add the ignore fields. we add two more field nested under the message
  // TestEmployee.
  std::vector<std::string> compare_fields;

  std::string field_1("DifferentialTest.TestEmployee.fullname");
  std::string field_2("DifferentialTest.TestEmployee.employ_id");
  std::string field_3("DifferentialTest.TestEmployee.age");

  // push the fields
  compare_fields.push_back(field_1);
  compare_fields.push_back(field_2);
  compare_fields.push_back(field_3);

  // White list criteria will do the ignore
  ClientUtil::CompareFields(&diff_request, compare_fields);

  // ********************** Differential Service **********************
  // Receive the differential response.
  DiffResponse diff_response;

  // Request the differential service.
  StatusCode service_response_status = TestStub_.CompareInputMessages(
      diff_request, &diff_response, server_address);

  // Check the return Status
  ASSERT_TRUE(StatusCode::OK == service_response_status);

  // Get the test result.
  const std::string& test_res = diff_response.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // ******************** Prospective result **********************
  // In this test we only compare these three fields and ignore the difference
  // in others field.
  const char* except_res =
      "modified: employ_id: 1 -> 2\n"
      "modified: fullname: \"Jin Huang\" -> \"Zhe Liu\"\n"
      "modified: age: 39 -> 32\n";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 14: Test compare a nested field in message.
TEST(DifferentialUnitTest, test_compare_one_nested_field) {
  // Generate two message by message type TestEmployee(defined in
  // DifferentialTest.proto)
  TestEmployee message_first;
  TestEmployee message_second;

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
  DiffRequest diff_request =
      ClientUtil::WriteMsgToDiffRequest(message_first, message_second);

  //******************** Test Content ****************************************
  // Add the ignore fields. we add two more field nested under the message
  // TestEmployee.
  std::vector<std::string> compare_fields;

  // because we comapre a nested field so we have to add its parent field first.
  std::string parent_field_1("DifferentialTest.TestEmployee.employer");
  std::string nested_field_1("DifferentialTest.Company.name");

  // push the nested fields.
  compare_fields.push_back(parent_field_1);
  compare_fields.push_back(nested_field_1);

  // White list criteria will do the ignore
  ClientUtil::CompareFields(&diff_request, compare_fields);

  // ********************** Differential Service **********************
  // Receive the differential response.
  DiffResponse diff_response;

  // Request the differential service.
  StatusCode service_response_status = TestStub_.CompareInputMessages(
      diff_request, &diff_response, server_address);

  // Check the return Status
  ASSERT_TRUE(StatusCode::OK == service_response_status);

  // Get the test result.
  const std::string& test_res = diff_response.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // ******************** Prospective result **********************
  // In this test case, we want to test the nested fields comparison
  const char* except_res =
      "modified: employer.name: \"Google Inc.\" -> \"Alphabet Inc.\"\n";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 15: Test compare multiple nested fields in message.
TEST(DifferentialUnitTest, test_compare_more_nested_fields) {
  // Generate two message by message type TestEmployee(defined in
  // DifferentialTest.proto)
  TestEmployee message_first;
  TestEmployee message_second;

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
  DiffRequest diff_request =
      ClientUtil::WriteMsgToDiffRequest(message_first, message_second);

  //******************** Test Content ****************************************
  // Add the ignore fields. we add two more field nested under the message
  // TestEmployee.
  std::vector<std::string> compare_fields;

  // because we comapre a nested field so we have to add its parent field first.
  std::string parent_field_1("DifferentialTest.TestEmployee.employer");
  std::string nested_field_1("DifferentialTest.Company.name");
  std::string nested_field_2("DifferentialTest.Company.occupation");
  std::string nested_field_3("DifferentialTest.Company.address");

  // push the nested fields.
  compare_fields.push_back(parent_field_1);
  compare_fields.push_back(nested_field_1);
  compare_fields.push_back(nested_field_2);
  compare_fields.push_back(nested_field_3);

  // White list criteria will do the ignore
  ClientUtil::CompareFields(&diff_request, compare_fields);

  // ********************** Differential Service **********************
  // Receive the differential response.
  DiffResponse diff_response;

  // Request the differential service.
  StatusCode service_response_status = TestStub_.CompareInputMessages(
      diff_request, &diff_response, server_address);

  // Check the return Status
  ASSERT_TRUE(StatusCode::OK == service_response_status);

  // Get the test result.
  const std::string& test_res = diff_response.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // ******************** Prospective result **********************
  // Test multiple fields again.
  const char* except_res =
      "modified: employer.name: \"Google Inc.\" -> \"Alphabet Inc.\"\n"
      "modified: employer.occupation: \"Software Engineer\" -> \"Research "
      "Scientist\"\n";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 16: Test compare no field in message. If the user want to compare
//          nothing (leave the compare fields blank), our service will compare
//          all fields in the message.
TEST(DifferentialUnitTest, test_compare_nothing) {
  // Generate two message by message type TestEmployee(defined in
  // DifferentialTest.proto)
  TestEmployee message_first;
  TestEmployee message_second;

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
  DiffRequest diff_request = ClientUtil::WriteMsgToDiffRequest(message_first, message_second);

  //******************** Test Content ****************************************
  // Add the ignore fields. we add two more field nested under the message
  // TestEmployee.
  std::vector<std::string> compare_fields;

  // Leave the compare fields empty.

  // White list criteria will do the ignore
  ClientUtil::CompareFields(&diff_request, compare_fields);

  // ********************** Differential Service **********************
  // Receive the differential response.
  DiffResponse diff_response;

  // Request the differential service.
  StatusCode service_response_status = TestStub_.CompareInputMessages( diff_request, &diff_response, server_address);

  // Check the return Status
  ASSERT_TRUE(StatusCode::OK == service_response_status);

  // Get the test result.
  const std::string& test_res = diff_response.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // ******************** Prospective result **********************
  // Compare all fields that user write
  const char* except_res =
      "modified: employ_id: 1 -> 2\n"
      "modified: fullname: \"Jin Huang\" -> \"Zhe Liu\"\n"
      "modified: age: 39 -> 32\n"
      "modified: employer.name: \"Google Inc.\" -> \"Alphabet Inc.\"\n"
      "modified: employer.occupation: \"Software Engineer\" -> \"Research "
      "Scientist\"\n";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 17: Test the regular expression for ignoring the fields of message.
TEST(DifferentialUnitTest, test_ignore_field_with_the_suffix_ame) {
  // Generate two message by message type TestEmployee(defined in
  // DifferentialTest.proto)
  TestEmployee message_first;
  TestEmployee message_second;

  // ******************** write two messages **********************
  message_first.set_fullname("Jin Huang");
  message_first.set_age(32);

  // Set the message 2 by the following stuff.
  message_second.set_fullname("Zhe Liu");
  message_second.set_age(29);

  // ***************** Generate the differential message request ***************
  DiffRequest diff_request =
      ClientUtil::WriteMsgToDiffRequest(message_first, message_second);

  //******************** Test Content ****************************************
  std::string regex(".*ame$");
  ClientUtil::RegexCriteria(&diff_request, regex);

  // ********************** Differential Service **********************
  // Receive the differential response.
  DiffResponse diff_response;

  // Request the differential service.
  StatusCode service_response_status = TestStub_.CompareInputMessages(
      diff_request, &diff_response, server_address);

  // Check the return Status
  ASSERT_TRUE(StatusCode::OK == service_response_status);

  // Get the test result.
  const std::string& test_res = diff_response.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // ******************** Prospective result **********************
  // Any different in suffix "ame" will be ignore.
  // So the different between age, employer address, and education address
  // should be presented.
  const char* except_res = "modified: age: 32 -> 29\n";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 18: Test the regular expression for ignoring the fields of message.
TEST(DifferentialUnitTest, test_ignore_field_with_the_suffix_ame_in_nested_field) {
  // Generate two message by message type TestEmployee(defined in
  // DifferentialTest.proto)
  TestEmployee message_first;
  TestEmployee message_second;

  // ******************** write two messages **********************

  message_first.mutable_employer()->set_name("Google Inc.");
  message_first.mutable_employer()->set_occupation("Software Engineer");
  message_first.mutable_employer()->set_address("CA, US");

  EducationInfo* edu_info_1 = message_first.add_education();
  edu_info_1->set_name("University of Florida");
  edu_info_1->set_degree("PhD");
  edu_info_1->set_major("Computer Science");
  edu_info_1->set_address("FL, US");

  // Set the message 2 by the following stuff.
  message_second.mutable_employer()->set_name("Google Inc.");
  message_second.mutable_employer()->set_occupation("Software Engineer");
  message_second.mutable_employer()->set_address("NY, US");

  EducationInfo* edu_info_2 = message_second.add_education();
  edu_info_2->set_name("Wright State University");
  edu_info_2->set_degree("PhD");
  edu_info_2->set_major("Computer Science");
  edu_info_2->set_address("OH, US");

  // ***************** Generate the differential message request ***************
  DiffRequest diff_request =
      ClientUtil::WriteMsgToDiffRequest(message_first, message_second);

  //******************** Test Content ****************************************
  std::string regex(".*ame$");
  ClientUtil::RegexCriteria(&diff_request, regex);

  // ********************** Differential Service **********************
  // Receive the differential response.
  DiffResponse diff_response;

  // Request the differential service.
  StatusCode service_response_status = TestStub_.CompareInputMessages(
      diff_request, &diff_response, server_address);

  // Check the return Status
  ASSERT_TRUE(StatusCode::OK == service_response_status);

  // Get the test result.
  const std::string& test_res = diff_response.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // ******************** Prospective result **********************
  // Any different in suffix "ame" will be ignore so the different in address
  // should be presented.
  const char* except_res =
      "modified: employer.address: \"CA, US\" -> \"NY, US\"\n"
      "modified: education[0].address: \"FL, US\" -> \"OH, US\"\n";

  EXPECT_STREQ(c_test_res, except_res);

  // Check the result error
  const std::string& test_error = diff_response.error();
  const char* c_test_error = test_error.c_str();
  const char* except_error = "";

  EXPECT_STREQ(c_test_error, except_error);
}

// Test 19: Test the regular expression for ignoring the fields of message.
TEST(DifferentialUnitTest, test_ignore_field_with_the_suffix_ress_in_nested_field) {
  // Generate two message by message type TestEmployee(defined in
  // DifferentialTest.proto)
  TestEmployee message_first;
  TestEmployee message_second;

  // ******************** write two messages **********************

  message_first.mutable_employer()->set_name("Google Inc.");
  message_first.mutable_employer()->set_occupation("Software Engineer");
  message_first.mutable_employer()->set_address("CA, US");

  EducationInfo* edu_info_1 = message_first.add_education();
  edu_info_1->set_name("University of Florida");
  edu_info_1->set_degree("PhD");
  edu_info_1->set_major("Computer Science");
  edu_info_1->set_address("FL, US");

  // Set the message 2 by the following stuff.
  message_second.mutable_employer()->set_name("Google Inc.");
  message_second.mutable_employer()->set_occupation("Software Engineer");
  message_second.mutable_employer()->set_address("NY, US");

  EducationInfo* edu_info_2 = message_second.add_education();
  edu_info_2->set_name("Wright State University");
  edu_info_2->set_degree("PhD");
  edu_info_2->set_major("Computer Science");
  edu_info_2->set_address("OH, US");

  // ***************** Generate the differential message request ***************
  DiffRequest diff_request =
      ClientUtil::WriteMsgToDiffRequest(message_first, message_second);

  //******************** Test Content ****************************************
  std::string regex(".*ress$");
  ClientUtil::RegexCriteria(&diff_request, regex);

  // ********************** Differential Service **********************
  // Receive the differential response.
  DiffResponse diff_response;

  // Request the differential service.
  StatusCode service_response_status = TestStub_.CompareInputMessages(
      diff_request, &diff_response, server_address);

  // Check the return Status
  ASSERT_TRUE(StatusCode::OK == service_response_status);

  // Get the test result.
  const std::string& test_res = diff_response.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // ******************** Prospective result **********************
  // Any different in suffix "ress" will be ignore.
  const char* except_res =
      "modified: education[0].name: \"University of Florida\" -> \"Wright "
      "State University\"\n";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 20: Test the repeated field treated as LIST.
TEST(DifferentialUnitTest, test_repeated_field_treat_as_list_same) {
  // Generate two message by message type TestEmployee(defined in
  // DifferentialTest.proto)
  TestEmployee message_first;
  TestEmployee message_second;

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
  DiffRequest diff_request =
      ClientUtil::WriteMsgToDiffRequest(message_first, message_second);

  //******************** Test Content ****************************************
  // Set the repeated field "areas" as list
  std::string field = "areas";
  ClientUtil::TreatRepeatedFieldAsListOrSet(&diff_request, true, field);

  // ********************** Differential Service **********************
  // Receive the differential response.
  DiffResponse diff_response;

  // Request the differential service.
  StatusCode service_response_status = TestStub_.CompareInputMessages(
      diff_request, &diff_response, server_address);

  // Check the return Status
  ASSERT_TRUE(StatusCode::OK == service_response_status);

  // Get the test result.
  const std::string& test_res = diff_response.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // ******************** Prospective result **********************
  // Compare field "area" as List so it should same.
  const char* except_res = "SAME";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 21: Test the repeated field treated as LIST.
TEST(DifferentialUnitTest, test_repeated_field_treat_as_list_diff) {
  // Generate two message by message type TestEmployee(defined in
  // DifferentialTest.proto)
  TestEmployee message_first;
  TestEmployee message_second;

  // ******************** write two messages **********************

  // Write two message with same value and same order.
  message_first.add_areas("Google Ads.");
  message_first.add_areas("YouTube Ads.");
  message_first.add_areas("Search Ads.");
  message_first.add_areas("Click Ads.");

  message_second.add_areas("Google Ads.");
  message_second.add_areas("YouTube Ads.");
  message_second.add_areas("Click Ads.");  // different
  message_second.add_areas("Search Ads.");

  // ***************** Generate the differential message request ***************
  DiffRequest diff_request =
      ClientUtil::WriteMsgToDiffRequest(message_first, message_second);

  //******************** Test Content ****************************************
  // Set the field areas as list
  std::string field_1 = "areas";
  ClientUtil::TreatRepeatedFieldAsListOrSet(&diff_request, true, field_1);

  // ********************** Differential Service **********************
  // Receive the differential response.
  DiffResponse diff_response;

  // Request the differential service.
  StatusCode service_response_status = TestStub_.CompareInputMessages(
      diff_request, &diff_response, server_address);

  // Check the return Status
  ASSERT_TRUE(StatusCode::OK == service_response_status);

  // Get the test result.
  const std::string& test_res = diff_response.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // ******************** Prospective result **********************
  // The order of field "areas" is different.
  const char* except_res =
      "modified: areas[2]: \"Search Ads.\" -> \"Click Ads.\"\n"
      "modified: areas[3]: \"Click Ads.\" -> \"Search Ads.\"\n";

  EXPECT_STREQ(c_test_res, except_res);

  // Check the result error
  const std::string& test_error = diff_response.error();
  const char* c_test_error = test_error.c_str();
  const char* except_error = "";

  EXPECT_STREQ(c_test_error, except_error);
}

// Test 22: Test a nested repeated field treated as LIST.
TEST(DifferentialUnitTest, test_repeated_nested_field_treat_as_list_same) {
  // Generate two message by message type TestEmployee(defined in
  // DifferentialTest.proto)
  TestEmployee message_first;
  TestEmployee message_second;

  // ******************** write two messages **********************

  // Write the nested field for two messages.
  DifferentialTest::DependentInfo* dependentInfo_ptr_1 =
      message_first.mutable_dependents();
  dependentInfo_ptr_1->add_name("Jeremy");
  dependentInfo_ptr_1->add_name("Zhe");
  dependentInfo_ptr_1->add_name("Jin");
  dependentInfo_ptr_1->add_name("June");

  DifferentialTest::DependentInfo* dependentInfo_ptr_2 =
      message_second.mutable_dependents();

  dependentInfo_ptr_2->add_name("Jeremy");
  dependentInfo_ptr_2->add_name("Zhe");
  dependentInfo_ptr_2->add_name("Jin");
  dependentInfo_ptr_2->add_name("June");

  // ***************** Generate the differential message request ***************
  DiffRequest diff_request =
      ClientUtil::WriteMsgToDiffRequest(message_first, message_second);

  //******************** Test Content ****************************************
  std::string field = "dependents.name";
  ClientUtil::TreatRepeatedFieldAsListOrSet(&diff_request, true, field);

  // ********************** Differential Service **********************
  // Receive the differential response.
  DiffResponse diff_response;

  // Request the differential service.
  StatusCode service_response_status = TestStub_.CompareInputMessages(
      diff_request, &diff_response, server_address);

  // Check the return Status
  ASSERT_TRUE(StatusCode::OK == service_response_status);

  // Get the test result.
  const std::string& test_res = diff_response.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // ******************** Prospective result **********************
  // should same.
  const char* except_res = "SAME";

  EXPECT_STREQ(c_test_res, except_res);

  // Check the result error
  const std::string& test_error = diff_response.error();
  const char* c_test_error = test_error.c_str();
  const char* except_error = "";

  EXPECT_STREQ(c_test_error, except_error);
}

// Test 23: Test a nested repeated field treated as LIST.
TEST(DifferentialUnitTest, test_repeated_nested_field_treat_as_list_diff) {
  // Generate two message by message type TestEmployee(defined in
  // DifferentialTest.proto)
  TestEmployee message_first;
  TestEmployee message_second;

  // ******************** write two messages **********************
  // Write the nested field for two messages.
  DifferentialTest::DependentInfo* dependentInfo_ptr_1 =
      message_first.mutable_dependents();
  dependentInfo_ptr_1->add_name("Jeremy");
  dependentInfo_ptr_1->add_name("Zhe");
  dependentInfo_ptr_1->add_name("Jin");
  dependentInfo_ptr_1->add_name("June");

  DifferentialTest::DependentInfo* dependentInfo_ptr_2 =
      message_second.mutable_dependents();

  dependentInfo_ptr_2->add_name("Zhe");
  dependentInfo_ptr_2->add_name("Jeremy");
  dependentInfo_ptr_2->add_name("June");
  dependentInfo_ptr_2->add_name("Jin");

  // ***************** Generate the differential message request ***************
  DiffRequest diff_request =
      ClientUtil::WriteMsgToDiffRequest(message_first, message_second);

  //******************** Test Content ****************************************
  // Set the field areas as list
  //  std::string field_1 = "areas";
  //  ClientUtil::TreatRepeatedFieldAsListOrSet(&diff_request, 0, field_1);
  std::string field = "dependents.name";
  ClientUtil::TreatRepeatedFieldAsListOrSet(&diff_request, true, field);

  // ********************** Differential Service **********************
  // Receive the differential response.
  DiffResponse diff_response;

  // Request the differential service.
  StatusCode service_response_status = TestStub_.CompareInputMessages(
      diff_request, &diff_response, server_address);

  // Check the return Status
  ASSERT_TRUE(StatusCode::OK == service_response_status);

  // Get the test result.
  const std::string& test_res = diff_response.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // ******************** Prospective result **********************
  // should same.
  const char* except_res =
      "modified: dependents.name[0]: \"Jeremy\" -> \"Zhe\"\n"
      "modified: dependents.name[1]: \"Zhe\" -> \"Jeremy\"\n"
      "modified: dependents.name[2]: \"Jin\" -> \"June\"\n"
      "modified: dependents.name[3]: \"June\" -> \"Jin\"\n";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 24: Test multiple nested repeated fields treated as LIST.
TEST(DifferentialUnitTest, test_multiple_repeated_fields_treat_as_list_same) {
  // Generate two message by message type TestEmployee(defined in
  // DifferentialTest.proto)
  TestEmployee message_first;
  TestEmployee message_second;

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
  DifferentialTest::DependentInfo* dependentInfo_ptr_1 =
      message_first.mutable_dependents();
  dependentInfo_ptr_1->add_name("Jeremy");
  dependentInfo_ptr_1->add_name("Zhe");
  dependentInfo_ptr_1->add_name("Jin");
  dependentInfo_ptr_1->add_name("June");

  dependentInfo_ptr_1->add_age(29);
  dependentInfo_ptr_1->add_age(30);
  dependentInfo_ptr_1->add_age(32);
  dependentInfo_ptr_1->add_age(26);

  DifferentialTest::DependentInfo* dependentInfo_ptr_2 =
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
  DiffRequest diff_request =
      ClientUtil::WriteMsgToDiffRequest(message_first, message_second);

  //******************** Test Content ****************************************
  // Set the field areas as list
  std::string field_1 = "areas";
  ClientUtil::TreatRepeatedFieldAsListOrSet(&diff_request, true, field_1);
  std::string field_2 = "dependents.name";
  ClientUtil::TreatRepeatedFieldAsListOrSet(&diff_request, true, field_2);
  std::string field_3 = "dependents.age";
  ClientUtil::TreatRepeatedFieldAsListOrSet(&diff_request, true, field_3);

  // ********************** Differential Service **********************
  // Receive the differential response.
  DiffResponse diff_response;

  // Request the differential service.
  StatusCode service_response_status = TestStub_.CompareInputMessages(
      diff_request, &diff_response, server_address);

  // Check the return Status
  ASSERT_TRUE(StatusCode::OK == service_response_status);

  // Get the test result.
  const std::string& test_res = diff_response.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // ******************** Prospective result **********************
  const char* except_res = "SAME";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 25: Test multiple nested repeated fields treated as LIST.
TEST(DifferentialUnitTest, test_multiple_repeated_fields_treat_as_list_diff) {
  // Generate two message by message type TestEmployee(defined in
  // DifferentialTest.proto)
  TestEmployee message_first;
  TestEmployee message_second;

  // ******************** write two messages **********************
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
  DifferentialTest::DependentInfo* dependentInfo_ptr_1 =
      message_first.mutable_dependents();
  dependentInfo_ptr_1->add_name("Jeremy");
  dependentInfo_ptr_1->add_name("Zhe");
  dependentInfo_ptr_1->add_name("June");
  dependentInfo_ptr_1->add_name("Jin");

  dependentInfo_ptr_1->add_age(29);
  dependentInfo_ptr_1->add_age(30);
  dependentInfo_ptr_1->add_age(26);
  dependentInfo_ptr_1->add_age(32);

  DifferentialTest::DependentInfo* dependentInfo_ptr_2 =
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
  DiffRequest diff_request =
      ClientUtil::WriteMsgToDiffRequest(message_first, message_second);

  //******************** Test Content ****************************************
  // Set the field areas as list
  std::string field_1 = "areas";
  ClientUtil::TreatRepeatedFieldAsListOrSet(&diff_request, true, field_1);
  std::string field_2 = "dependents.name";
  ClientUtil::TreatRepeatedFieldAsListOrSet(&diff_request, true, field_2);
  std::string field_3 = "dependents.age";
  ClientUtil::TreatRepeatedFieldAsListOrSet(&diff_request, true, field_3);

  // ********************** Differential Service **********************
  // Receive the differential response.
  DiffResponse diff_response;

  // Request the differential service.
  StatusCode service_response_status = TestStub_.CompareInputMessages(
      diff_request, &diff_response, server_address);

  // Check the return Status
  ASSERT_TRUE(StatusCode::OK == service_response_status);

  // Get the test result.
  const std::string& test_res = diff_response.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // ******************** Prospective result **********************
  const char* except_res =
      "modified: areas[2]: \"Search Ads.\" -> \"Click Ads.\"\n"
      "modified: areas[3]: \"Click Ads.\" -> \"Search Ads.\"\n"
      "modified: dependents.name[2]: \"June\" -> \"Jin\"\n"
      "modified: dependents.name[3]: \"Jin\" -> \"June\"\n"
      "modified: dependents.age[2]: 26 -> 32\n"
      "modified: dependents.age[3]: 32 -> 26\n";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 26: Test the repeated field treated as LIST. deleted element.
TEST(DifferentialUnitTest, test_repleated_field_treat_as_list_delete_item) {
  // Generate two message by message type TestEmployee(defined in
  // DifferentialTest.proto)
  TestEmployee message_first;
  TestEmployee message_second;

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
  DiffRequest diff_request =
      ClientUtil::WriteMsgToDiffRequest(message_first, message_second);

  //******************** Test Content ****************************************
  // Set the field areas as list
  std::string field_1 = "areas";
  ClientUtil::TreatRepeatedFieldAsListOrSet(&diff_request, true, field_1);

  // ********************** Differential Service **********************
  // Receive the differential response.
  DiffResponse diff_response;

  // Request the differential service.
  StatusCode service_response_status = TestStub_.CompareInputMessages(
      diff_request, &diff_response, server_address);

  // Check the return Status
  ASSERT_TRUE(StatusCode::OK == service_response_status);

  // Get the test result.
  const std::string& test_res = diff_response.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // ******************** Prospective result **********************
  // Message 1 have one more element than message2.
  const char* except_res = "deleted: areas[3]: \"Click Ads.\"\n";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 27: Test the repeated field treated as LIST. add element.
TEST(DifferentialUnitTest, test_repleated_field_treat_as_list_add_item) {
  TestEmployee message_first;
  TestEmployee message_second;

  // Write two message with same value and same order.
  message_first.add_areas("Google Ads.");
  message_first.add_areas("YouTube Ads.");
  message_first.add_areas("Search Ads.");

  message_second.add_areas("Google Ads.");
  message_second.add_areas("YouTube Ads.");
  message_second.add_areas("Search Ads.");
  message_second.add_areas("Click Ads.");

  // Write the message to log message.
  DiffRequest diff_request =
      ClientUtil::WriteMsgToDiffRequest(message_first, message_second);

  // Set the field areas as list
  std::string field_1 = "areas";
  ClientUtil::TreatRepeatedFieldAsListOrSet(&diff_request, true, field_1);

  // Implements the differential service.
  // Receive the differential response.
  DiffResponse diff_response;

  // Request the differential service.
  StatusCode service_response_status = TestStub_.CompareInputMessages(
      diff_request, &diff_response, server_address);

  // Check the return Status
  ASSERT_TRUE(StatusCode::OK == service_response_status);

  // Get the test result.
  const std::string& test_res = diff_response.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // ******************** Prospective result **********************
  // Message 1 need add one element to be equal with message 2.
  const char* except_res = "added: areas[3]: \"Click Ads.\"\n";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 28: Test the repeated field treated as SET.
TEST(DifferentialUnitTest, test_repeated_field_treat_as_set_same) {
  TestEmployee message_first;
  TestEmployee message_second;

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
  DiffRequest diff_request =
      ClientUtil::WriteMsgToDiffRequest(message_first, message_second);

  // Set the field areas as set
  std::string field_1 = "areas";
  ClientUtil::TreatRepeatedFieldAsListOrSet(&diff_request, false, field_1);

  // Receive the differential response.
  DiffResponse diff_response;

  // Request the differential service.
  StatusCode service_response_status = TestStub_.CompareInputMessages(
      diff_request, &diff_response, server_address);

  // Check the return Status
  ASSERT_TRUE(StatusCode::OK == service_response_status);

  // Get the test result.
  const std::string& test_res = diff_response.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // Because we treat the repeated field as set-based so the result is same.
  const char* except_res = "SAME";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 29: Test the repeated field treated as SET.
TEST(DifferentialUnitTest, test_repeated_field_treat_as_set_diff) {
  TestEmployee message_first;
  TestEmployee message_second;

  // Write two message with different value
  message_first.add_areas("Google Ads.");
  message_first.add_areas("YouTube Ads.");  // The different one.
  message_first.add_areas("Search Ads.");
  message_first.add_areas("Click Ads.");

  message_second.add_areas("Search Ads.");
  message_second.add_areas("Click Ads.");
  message_second.add_areas("Google Ads.");
  message_second.add_areas("Pop_up Ads.");  // The different one.

  // Write the message to log message.
  DiffRequest diff_request =
      ClientUtil::WriteMsgToDiffRequest(message_first, message_second);

  // Set the field areas as set
  std::string field_1 = "areas";
  ClientUtil::TreatRepeatedFieldAsListOrSet(&diff_request, false, field_1);

  // Implements the differential service.
  // Receive the differential response.
  DiffResponse diff_response;

  // Request the differential service.
  StatusCode service_response_status = TestStub_.CompareInputMessages(
      diff_request, &diff_response, server_address);

  // Check the return Status
  ASSERT_TRUE(StatusCode::OK == service_response_status);

  // Get the test result.
  const std::string& test_res = diff_response.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // The first message needs deleting the areas[1] and add "Pop_up Ads.".
  const char* except_res =
      "added: areas[3]: \"Pop_up Ads.\"\ndeleted: areas[1]: \"YouTube Ads.\"\n";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 30: Test the repeated field treated as SET. delete element.
TEST(DifferentialUnitTest, test_repeated_field_treat_as_set_delete) {
  TestEmployee message_first;
  TestEmployee message_second;

  // Write two message with different value
  message_first.add_areas("Google Ads.");
  message_first.add_areas("YouTube Ads.");  // The different one.
  message_first.add_areas("Search Ads.");
  message_first.add_areas("Click Ads.");

  message_second.add_areas("Search Ads.");
  message_second.add_areas("Click Ads.");
  message_second.add_areas("Google Ads.");

  // Write the message to log message.
  DiffRequest diff_request =
      ClientUtil::WriteMsgToDiffRequest(message_first, message_second);

  // Set the field areas as set
  std::string field_1 = "areas";
  ClientUtil::TreatRepeatedFieldAsListOrSet(&diff_request, false, field_1);

  // Receive the differential response.
  DiffResponse diff_response;

  // Request the differential service.
  StatusCode service_response_status = TestStub_.CompareInputMessages(
      diff_request, &diff_response, server_address);

  // Check the return Status
  ASSERT_TRUE(StatusCode::OK == service_response_status);

  // Get the test result.
  const std::string& test_res = diff_response.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // The first message needs deleting the areas[1]
  const char* except_res = "deleted: areas[1]: \"YouTube Ads.\"\n";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 31: Test the repeated field treated as SET. Add element
TEST(DifferentialUnitTest, test_repeated_field_treat_as_set_add) {
  TestEmployee message_first;
  TestEmployee message_second;

  // Write two message with different value
  message_first.add_areas("Google Ads.");
  message_first.add_areas("Search Ads.");
  message_first.add_areas("Click Ads.");

  message_second.add_areas("Search Ads.");
  message_second.add_areas("Click Ads.");
  message_second.add_areas("Google Ads.");
  message_second.add_areas("Pop_up Ads.");  // The different one.

  // Write the message to log message.
  DiffRequest diff_request =
      ClientUtil::WriteMsgToDiffRequest(message_first, message_second);

  // Set the field areas as set
  std::string field_1 = "areas";
  ClientUtil::TreatRepeatedFieldAsListOrSet(&diff_request, false, field_1);

  // Receive the differential response.
  DiffResponse diff_response;

  // Request the differential service.
  StatusCode service_response_status = TestStub_.CompareInputMessages(
      diff_request, &diff_response, server_address);

  // Check the return Status
  ASSERT_TRUE(StatusCode::OK == service_response_status);

  // Get the test result.
  const std::string& test_res = diff_response.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // The first message needs adding "Pop_up Ads.".
  const char* except_res = "added: areas[3]: \"Pop_up Ads.\"\n";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 32: Test the repeated field treated as SET. adding mulitpe elements
TEST(DifferentialUnitTest, test_repeated_field_treat_as_set_add_more_fields) {
  TestEmployee message_first;
  TestEmployee message_second;

  // Write two message with different value
  message_first.add_areas("Google Ads.");

  message_second.add_areas("Search Ads.");
  message_second.add_areas("Click Ads.");
  message_second.add_areas("Google Ads.");
  message_second.add_areas("Pop_up Ads.");  // The different one.

  // Write the message to log message.
  DiffRequest diff_request =
      ClientUtil::WriteMsgToDiffRequest(message_first, message_second);

  // Set the field areas as set
  std::string field_1 = "areas";
  ClientUtil::TreatRepeatedFieldAsListOrSet(&diff_request, false, field_1);

  // Receive the differential response.
  DiffResponse diff_response;

  // Request the differential service.
  StatusCode service_response_status = TestStub_.CompareInputMessages(
      diff_request, &diff_response, server_address);

  // Check the return Status
  ASSERT_TRUE(StatusCode::OK == service_response_status);

  // Get the test result.
  const std::string& test_res = diff_response.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // The first message needs adding multiple elements than the seconde message.
  const char* except_res =
      "added: areas[0]: \"Search Ads.\"\n"
      "added: areas[1]: \"Click Ads.\"\n"
      "added: areas[3]: \"Pop_up Ads.\"\n";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 33: Test the nested repeated field treated as SET.
TEST(DifferentialUnitTest, test_repeated_nested_field_treat_as_set_same) {
  TestEmployee message_first;
  TestEmployee message_second;

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
  DifferentialTest::DependentInfo* dependentInfo_ptr_1 =
      message_first.mutable_dependents();
  dependentInfo_ptr_1->add_name("Jeremy");
  dependentInfo_ptr_1->add_name("Zhe");
  dependentInfo_ptr_1->add_name("Jin");
  dependentInfo_ptr_1->add_name("June");

  dependentInfo_ptr_1->add_age(29);
  dependentInfo_ptr_1->add_age(30);
  dependentInfo_ptr_1->add_age(32);
  dependentInfo_ptr_1->add_age(26);

  DifferentialTest::DependentInfo* dependentInfo_ptr_2 =
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
  DiffRequest diff_request =
      ClientUtil::WriteMsgToDiffRequest(message_first, message_second);

  // Set the field areas as set
  std::string field_1 = "areas";
  ClientUtil::TreatRepeatedFieldAsListOrSet(&diff_request, false, field_1);

  std::string field_2 = "dependents.name";
  ClientUtil::TreatRepeatedFieldAsListOrSet(&diff_request, false, field_2);

  std::string field_3 = "dependents.age";
  ClientUtil::TreatRepeatedFieldAsListOrSet(&diff_request, false, field_3);

  // Receive the differential response.
  DiffResponse diff_response;

  // Request the differential service.
  StatusCode service_response_status = TestStub_.CompareInputMessages(
      diff_request, &diff_response, server_address);

  // Check the return Status
  ASSERT_TRUE(StatusCode::OK == service_response_status);

  // Get the test result.
  const std::string& test_res = diff_response.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // The first message needs adding multiple elements than the seconde message.
  const char* except_res = "SAME";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 34: Test a nested repeated field treated as SET. Add element/s
TEST(DifferentialUnitTest, test_repeated_nested_field_treat_as_set_add) {
  TestEmployee message_first;
  TestEmployee message_second;

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
  DifferentialTest::DependentInfo* dependentInfo_ptr_1 =
      message_first.mutable_dependents();
  dependentInfo_ptr_1->add_name("Jeremy");
  dependentInfo_ptr_1->add_name("Zhe");

  dependentInfo_ptr_1->add_age(29);
  dependentInfo_ptr_1->add_age(30);
  dependentInfo_ptr_1->add_age(32);

  DifferentialTest::DependentInfo* dependentInfo_ptr_2 =
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
  DiffRequest diff_request =
      ClientUtil::WriteMsgToDiffRequest(message_first, message_second);

  // Set the field areas as set
  std::string field_1 = "areas";
  ClientUtil::TreatRepeatedFieldAsListOrSet(&diff_request, false, field_1);

  std::string field_2 = "dependents.name";
  ClientUtil::TreatRepeatedFieldAsListOrSet(&diff_request, false, field_2);

  std::string field_3 = "dependents.age";
  ClientUtil::TreatRepeatedFieldAsListOrSet(&diff_request, false, field_3);

  // Receive the differential response.
  DiffResponse diff_response;

  // Request the differential service.
  StatusCode service_response_status = TestStub_.CompareInputMessages(
      diff_request, &diff_response, server_address);

  // Check the return Status
  ASSERT_TRUE(StatusCode::OK == service_response_status);

  // Get the test result.
  const std::string& test_res = diff_response.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // The first message needs adding multiple elements than the seconde message.
  const char* except_res =
      "added: dependents.name[2]: \"June\"\n"
      "added: dependents.name[3]: \"Jin\"\n"
      "added: dependents.age[3]: 26\n";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 35: Test a nested repeated field treated as SET. delete element/s
TEST(DifferentialUnitTest, test_repeated_nested_field_treat_as_set_delete) {
  TestEmployee message_first;
  TestEmployee message_second;

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
  DifferentialTest::DependentInfo* dependentInfo_ptr_1 =
      message_first.mutable_dependents();
  dependentInfo_ptr_1->add_name("Jeremy");
  dependentInfo_ptr_1->add_name("Zhe");
  dependentInfo_ptr_1->add_name("Jin");
  dependentInfo_ptr_1->add_name("June");

  dependentInfo_ptr_1->add_age(29);
  dependentInfo_ptr_1->add_age(30);
  dependentInfo_ptr_1->add_age(32);
  dependentInfo_ptr_1->add_age(26);

  DifferentialTest::DependentInfo* dependentInfo_ptr_2 =
      message_second.mutable_dependents();

  dependentInfo_ptr_2->add_name("Zhe");
  dependentInfo_ptr_2->add_name("Jeremy");
  dependentInfo_ptr_2->add_name("June");

  dependentInfo_ptr_2->add_age(29);
  dependentInfo_ptr_2->add_age(30);
  dependentInfo_ptr_2->add_age(26);

  // Write the message to log message.
  DiffRequest diff_request =
      ClientUtil::WriteMsgToDiffRequest(message_first, message_second);

  // Set the field areas as set
  std::string field_1 = "areas";
  ClientUtil::TreatRepeatedFieldAsListOrSet(&diff_request, false, field_1);

  std::string field_2 = "dependents.name";
  ClientUtil::TreatRepeatedFieldAsListOrSet(&diff_request, false, field_2);

  std::string field_3 = "dependents.age";
  ClientUtil::TreatRepeatedFieldAsListOrSet(&diff_request, false, field_3);

  // Receive the differential response.
  DiffResponse diff_response;

  // Request the differential service.
  StatusCode service_response_status = TestStub_.CompareInputMessages(
      diff_request, &diff_response, server_address);

  // Check the return Status
  ASSERT_TRUE(StatusCode::OK == service_response_status);

  // Get the test result.
  const std::string& test_res = diff_response.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // The first message needs adding multiple elements than the seconde message.
  const char* except_res =
      "deleted: dependents.name[2]: \"Jin\"\n"
      "deleted: dependents.age[2]: 32\n";

  EXPECT_STREQ(c_test_res, except_res);

  // Check the result error
  const std::string& test_error = diff_response.error();
  const char* c_test_error = test_error.c_str();
  const char* except_error = "";

  EXPECT_STREQ(c_test_error, except_error);
}

// Test 36: Test a nested repeated field treated as SET. delete and add element/s
TEST(DifferentialUnitTest, test_repeated_nested_field_treat_as_set_delete_and_add) {
  TestEmployee message_first;
  TestEmployee message_second;

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
  DifferentialTest::DependentInfo* dependentInfo_ptr_1 =
      message_first.mutable_dependents();
  dependentInfo_ptr_1->add_name("Jeremy");
  dependentInfo_ptr_1->add_name("Zhe");
  dependentInfo_ptr_1->add_name("Tom");
  dependentInfo_ptr_1->add_name("June");

  dependentInfo_ptr_1->add_age(29);
  dependentInfo_ptr_1->add_age(30);
  dependentInfo_ptr_1->add_age(31);
  dependentInfo_ptr_1->add_age(26);

  DifferentialTest::DependentInfo* dependentInfo_ptr_2 =
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
  DiffRequest diff_request =
      ClientUtil::WriteMsgToDiffRequest(message_first, message_second);

  // Set the field areas as set
  std::string field_1 = "areas";
  ClientUtil::TreatRepeatedFieldAsListOrSet(&diff_request, false, field_1);

  std::string field_2 = "dependents.name";
  ClientUtil::TreatRepeatedFieldAsListOrSet(&diff_request, false, field_2);

  std::string field_3 = "dependents.age";
  ClientUtil::TreatRepeatedFieldAsListOrSet(&diff_request, false, field_3);

  // Receive the differential response.
  DiffResponse diff_response;

  // Request the differential service.
  StatusCode service_response_status = TestStub_.CompareInputMessages(
      diff_request, &diff_response, server_address);

  // Check the return Status
  ASSERT_TRUE(StatusCode::OK == service_response_status);

  // Get the test result.
  const std::string& test_res = diff_response.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // The first message needs adding multiple elements than the seconde message.
  const char* except_res =
      "added: areas[3]: \"Search Ads.\"\n"
      "deleted: areas[2]: \"Pop_up Ads.\"\n"
      "added: dependents.name[3]: \"Jin\"\n"
      "deleted: dependents.name[2]: \"Tom\"\n"
      "added: dependents.age[2]: 32\n"
      "deleted: dependents.age[2]: 31\n";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 37: Test repeated fields treated as SET and LIST
TEST(DifferentialUnitTest, test_repeated_fields_treat_as_list_and_set) {
  TestEmployee message_first;
  TestEmployee message_second;

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
  DifferentialTest::DependentInfo* dependentInfo_ptr_1 =
      message_first.mutable_dependents();
  dependentInfo_ptr_1->add_name("Jeremy");
  dependentInfo_ptr_1->add_name("Zhe");
  dependentInfo_ptr_1->add_name("June");
  dependentInfo_ptr_1->add_name("Jin");

  dependentInfo_ptr_1->add_age(29);
  dependentInfo_ptr_1->add_age(30);
  dependentInfo_ptr_1->add_age(32);
  dependentInfo_ptr_1->add_age(26);

  DifferentialTest::DependentInfo* dependentInfo_ptr_2 =
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
  DiffRequest diff_request =
      ClientUtil::WriteMsgToDiffRequest(message_first, message_second);

  // Set the field areas as set
  std::string field_1 = "areas";
  ClientUtil::TreatRepeatedFieldAsListOrSet(&diff_request, true, field_1);

  std::string field_2 = "dependents.name";
  ClientUtil::TreatRepeatedFieldAsListOrSet(&diff_request, false, field_2);

  std::string field_3 = "dependents.age";
  ClientUtil::TreatRepeatedFieldAsListOrSet(&diff_request, true, field_3);

  // Receive the differential response.
  DiffResponse diff_response;

  // Request the differential service.
  StatusCode service_response_status = TestStub_.CompareInputMessages(
      diff_request, &diff_response, server_address);

  // Check the return Status
  ASSERT_TRUE(StatusCode::OK == service_response_status);

  // Get the test result.
  const std::string& test_res = diff_response.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // The first message needs adding multiple elements than the seconde message.
  const char* except_res = "SAME";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 38: Test the map-value compare
TEST(DifferentialUnitTest, test_map_compare_same) {
  TestEmployee message_first;
  TestEmployee message_second;

  EducationInfo* edu_info_1 = message_first.add_education();
  edu_info_1->set_name("Wright State University");
  edu_info_1->set_degree("PhD");
  edu_info_1->set_major("Computer Science");
  edu_info_1->set_address("OH, US");

  EducationInfo* edu_info_2 = message_second.add_education();
  edu_info_2->set_name("Wright State University");
  edu_info_2->set_degree("PhD");
  edu_info_2->set_major("Computer Science");
  edu_info_2->set_address("OH, US");

  // Write the message to log message.
  DiffRequest diff_request =
      ClientUtil::WriteMsgToDiffRequest(message_first, message_second);

  // Set the map value for repeated filed compare.
  std::string map_field_name = "education";
  std::vector<std::string> sub_field_list;
  sub_field_list.push_back("name");
  ClientUtil::TreatRepeatedFieldAsMap(&diff_request, map_field_name,
                                      sub_field_list);

  // Receive the differential response.
  DiffResponse diff_response;

  // Request the differential service.
  StatusCode service_response_status = TestStub_.CompareInputMessages(
      diff_request, &diff_response, server_address);

  // Check the return Status
  ASSERT_TRUE(StatusCode::OK == service_response_status);

  // Get the test result.
  const std::string& test_res = diff_response.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // Map value is different replace the Map-value.
  const char* except_res = "SAME";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 39: Test a sub filed treated as Map. [key] is different
TEST(DifferentialUnitTest, test_map_compare_key_diff) {
  TestEmployee message_first;
  TestEmployee message_second;

  EducationInfo* edu_info_1 = message_first.add_education();
  edu_info_1->set_name("University of Dayton");
  edu_info_1->set_degree("PhD");
  edu_info_1->set_major("Computer Science");
  edu_info_1->set_address("OH, US");

  EducationInfo* edu_info_2 = message_second.add_education();
  edu_info_2->set_name("Wright State University");
  edu_info_2->set_degree("PhD");
  edu_info_2->set_major("Computer Science");
  edu_info_2->set_address("OH, US");

  // Write the message to log message.
  DiffRequest diff_request =
      ClientUtil::WriteMsgToDiffRequest(message_first, message_second);

  // Set the map value for repeated filed compare.
  std::string map_field_name = "education";
  std::vector<std::string> sub_field_list;
  sub_field_list.push_back("name");
  ClientUtil::TreatRepeatedFieldAsMap(&diff_request, map_field_name,
                                      sub_field_list);

  // Receive the differential response.
  DiffResponse diff_response;

  // Request the differential service.
  StatusCode service_response_status = TestStub_.CompareInputMessages(
      diff_request, &diff_response, server_address);

  // Check the return Status
  ASSERT_TRUE(StatusCode::OK == service_response_status);

  // Get the test result.
  const std::string& test_res = diff_response.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // Map value is different replace the Map-value.
  const char* except_res =
      "added: education[0]: { name: \"Wright State University\" degree: "
      "\"PhD\" major: \"Computer Science\" address: \"OH, US\" }\n"
      "deleted: education[0]: { name: \"University of Dayton\" degree: \"PhD\" "
      "major: \"Computer Science\" address: \"OH, US\" }\n";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 40: Test a sub filed treat as Map. [value] is different
TEST(DifferentialUnitTest, test_map_compare_value_diff) {
  TestEmployee message_first;
  TestEmployee message_second;

  EducationInfo* edu_info_1 = message_first.add_education();
  edu_info_1->set_name("Wright State University");
  edu_info_1->set_degree("PhD");
  edu_info_1->set_major("Computer Science");
  edu_info_1->set_address("OH, US");

  EducationInfo* edu_info_2 = message_second.add_education();
  edu_info_2->set_name("Wright State University");
  edu_info_2->set_degree("Master of Science");
  edu_info_2->set_major("Computer Science and Engineering");
  edu_info_2->set_address("OH, US");

  // Write the message to log message.
  DiffRequest diff_request =
      ClientUtil::WriteMsgToDiffRequest(message_first, message_second);

  // Set the map value for repeated filed compare.
  std::string map_field_name = "education";
  std::vector<std::string> sub_field_list;
  sub_field_list.push_back("name");
  ClientUtil::TreatRepeatedFieldAsMap(&diff_request, map_field_name,
                                      sub_field_list);

  // Receive the differential response.
  DiffResponse diff_response;

  // Request the differential service.
  StatusCode service_response_status = TestStub_.CompareInputMessages(
      diff_request, &diff_response, server_address);

  // Check the return Status
  ASSERT_TRUE(StatusCode::OK == service_response_status);

  // Get the test result.
  const std::string& test_res = diff_response.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // The first message needs adding multiple elements than the seconde message.
  const char* except_res =
      "modified: education[0].degree: \"PhD\" -> \"Master of Science\"\n"
      "modified: education[0].major: \"Computer Science\" -> \"Computer "
      "Science and Engineering\"\n";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 41: Test a sub filed treat as Map. Map and Value are different at same time.
TEST(DifferentialUnitTest, test_map_compare_key_and_value_diff) {
  TestEmployee message_first;
  TestEmployee message_second;

  EducationInfo* edu_info_1 = message_first.add_education();
  edu_info_1->set_name("Wright State University");
  edu_info_1->set_degree("PhD");
  edu_info_1->set_major("Computer Science");
  edu_info_1->set_address("OH, US");

  EducationInfo* edu_info_2 = message_second.add_education();
  edu_info_2->set_name("University of Dayton");
  edu_info_2->set_degree("Master of Science");
  edu_info_2->set_major("Computer Science and Engineering");
  edu_info_2->set_address("OH, US");

  // Write the message to log message.
  DiffRequest diff_request =
      ClientUtil::WriteMsgToDiffRequest(message_first, message_second);

  // Set the map value for repeated filed compare.
  std::string map_field_name = "education";
  std::vector<std::string> sub_field_list;
  sub_field_list.push_back("name");
  ClientUtil::TreatRepeatedFieldAsMap(&diff_request, map_field_name,
                                      sub_field_list);

  // Receive the differential response.
  DiffResponse diff_response;

  // Request the differential service.
  StatusCode service_response_status = TestStub_.CompareInputMessages(
      diff_request, &diff_response, server_address);

  // Check the return Status
  ASSERT_TRUE(StatusCode::OK == service_response_status);

  // Get the test result.
  const std::string& test_res = diff_response.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // map value different add and delete.
  const char* except_res =
      "added: education[0]: { name: \"University of Dayton\" degree: \"Master "
      "of Science\" major: \"Computer Science and Engineering\" address: \"OH, "
      "US\" }\n"
      "deleted: education[0]: { name: \"Wright State University\" degree: "
      "\"PhD\" major: \"Computer Science\" address: \"OH, US\" }\n";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 42: Test multiple sub fields as Map. Map is different.
TEST(DifferentialUnitTest, test_map_compare_multi_fields_as_key_same) {
  TestEmployee message_first;
  TestEmployee message_second;

  EducationInfo* edu_info_1 = message_first.add_education();
  edu_info_1->set_name("University of Dayton");
  edu_info_1->set_degree("PhD");
  edu_info_1->set_major("Computer Science");
  edu_info_1->set_address("OH, US");

  EducationInfo* edu_info_2 = message_second.add_education();
  edu_info_2->set_name("University of Dayton");
  edu_info_2->set_degree("PhD");
  edu_info_2->set_major("Computer Science");
  edu_info_2->set_address("OH, US");

  // Write the message to log message.
  DiffRequest diff_request =
      ClientUtil::WriteMsgToDiffRequest(message_first, message_second);

  // Set the map value for repeated filed compare.
  std::string map_field_name = "education";
  std::vector<std::string> sub_field_list;
  sub_field_list.push_back("name");
  sub_field_list.push_back("degree");
  ClientUtil::TreatRepeatedFieldAsMap(&diff_request, map_field_name,
                                      sub_field_list);

  // Receive the differential response.
  DiffResponse diff_response;

  // Request the differential service.
  StatusCode service_response_status = TestStub_.CompareInputMessages(
      diff_request, &diff_response, server_address);

  // Check the return Status
  ASSERT_TRUE(StatusCode::OK == service_response_status);

  // Get the test result.
  const std::string& test_res = diff_response.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // Map value different
  const char* except_res = "SAME";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 43: Test multiple sub fields as Map. Map is different.
TEST(DifferentialUnitTest, test_map_compare_multi_fields_as_key_diff) {
  TestEmployee message_first;
  TestEmployee message_second;

  EducationInfo* edu_info_1 = message_first.add_education();
  edu_info_1->set_name("Wright State University");
  edu_info_1->set_degree("PhD");
  edu_info_1->set_major("Computer Science");
  edu_info_1->set_address("OH, US");

  EducationInfo* edu_info_2 = message_second.add_education();
  edu_info_2->set_name("University of Dayton");
  edu_info_2->set_degree("PhD");
  edu_info_2->set_major("Computer Science");
  edu_info_2->set_address("OH, US");

  // Write the message to log message.
  DiffRequest diff_request =
      ClientUtil::WriteMsgToDiffRequest(message_first, message_second);

  // Set the map value for repeated filed compare.
  std::string map_field_name = "education";
  std::vector<std::string> sub_field_list;
  sub_field_list.push_back("name");
  sub_field_list.push_back("degree");
  ClientUtil::TreatRepeatedFieldAsMap(&diff_request, map_field_name,
                                      sub_field_list);

  // Receive the differential response.
  DiffResponse diff_response;

  // Request the differential service.
  StatusCode service_response_status = TestStub_.CompareInputMessages(
      diff_request, &diff_response, server_address);

  // Check the return Status
  ASSERT_TRUE(StatusCode::OK == service_response_status);

  // Get the test result.
  const std::string& test_res = diff_response.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // Map value different
  const char* except_res =
      "added: education[0]: { name: \"University of Dayton\" degree: \"PhD\" "
      "major: \"Computer Science\" address: \"OH, US\" }\n"
      "deleted: education[0]: { name: \"Wright State University\" degree: "
      "\"PhD\" major: \"Computer Science\" address: \"OH, US\" }\n";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 44: Test multiple sub fields as Map. Value is different.
TEST(DifferentialUnitTest, test_map_compare_multi_fields_as_key_value_diff) {
  TestEmployee message_first;
  TestEmployee message_second;

  EducationInfo* edu_info_1 = message_first.add_education();
  edu_info_1->set_name("Wright State University");
  edu_info_1->set_degree("PhD");
  edu_info_1->set_major("Computer Science");
  edu_info_1->set_address("OH, US");

  EducationInfo* edu_info_2 = message_second.add_education();
  edu_info_2->set_name("Wright State University");
  edu_info_2->set_major("Computer Science and Engineering");
  edu_info_2->set_address("OH, US");
  edu_info_2->set_degree("PhD");

  // Write the message to log message.
  DiffRequest diff_request =
      ClientUtil::WriteMsgToDiffRequest(message_first, message_second);

  // Set the map value for repeated filed compare.
  std::string map_field_name = "education";
  std::vector<std::string> sub_field_list;
  sub_field_list.push_back("name");
  sub_field_list.push_back("degree");
  ClientUtil::TreatRepeatedFieldAsMap(&diff_request, map_field_name,
                                      sub_field_list);

  // Receive the differential response.
  DiffResponse diff_response;

  // Request the differential service.
  StatusCode service_response_status = TestStub_.CompareInputMessages(
      diff_request, &diff_response, server_address);

  // Check the return Status
  ASSERT_TRUE(StatusCode::OK == service_response_status);

  // Get the test result.
  const std::string& test_res = diff_response.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // The first message needs adding multiple elements than the seconde message.
  const char* except_res =
      "modified: education[0].major: \"Computer Science\" -> \"Computer "
      "Science and Engineering\"\n";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 45: Test multiple sub fields as Map. Map and Value are different.
TEST(DifferentialUnitTest, test_map_compare_multi_fields_as_key_all_diff) {
  TestEmployee message_first;
  TestEmployee message_second;

  EducationInfo* edu_info_1 = message_first.add_education();
  edu_info_1->set_name("Wright State University");
  edu_info_1->set_degree("PhD");
  edu_info_1->set_major("Computer Science");
  edu_info_1->set_address("OH, US");

  EducationInfo* edu_info_2 = message_second.add_education();
  edu_info_2->set_name("University of Dayton");
  edu_info_2->set_major("Computer Science and Engineering");
  edu_info_2->set_address("OH, US");
  edu_info_2->set_degree("PhD");

  // Write the message to log message.
  DiffRequest diff_request =
      ClientUtil::WriteMsgToDiffRequest(message_first, message_second);

  // Set the map value for repeated filed compare.
  std::string map_field_name = "education";
  std::vector<std::string> sub_field_list;
  sub_field_list.push_back("name");
  sub_field_list.push_back("degree");
  ClientUtil::TreatRepeatedFieldAsMap(&diff_request, map_field_name,
                                      sub_field_list);

  // Receive the differential response.
  DiffResponse diff_response;

  // Request the differential service.
  StatusCode service_response_status = TestStub_.CompareInputMessages(
      diff_request, &diff_response, server_address);

  // Check the return Status
  ASSERT_TRUE(StatusCode::OK == service_response_status);

  // Get the test result.
  const std::string& test_res = diff_response.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // The first message needs adding multiple elements than the seconde message.
  const char* except_res =
      "added: education[0]: { name: \"University of Dayton\" degree: \"PhD\" "
      "major: \"Computer Science and Engineering\" address: \"OH, US\" }\n"
      "deleted: education[0]: { name: \"Wright State University\" degree: "
      "\"PhD\" major: \"Computer Science\" address: \"OH, US\" }\n";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 46: Test multiple sub fields as Map. message 1 is empty
TEST(DifferentialUnitTest, test_map_compare_first_message_empty) {
  TestEmployee message_first;
  TestEmployee message_second;

  EducationInfo* edu_info_1 = message_first.add_education();

  EducationInfo* edu_info_2 = message_second.add_education();
  edu_info_2->set_name("University of Dayton");
  edu_info_2->set_major("Computer Science and Engineering");
  edu_info_2->set_address("OH, US");
  edu_info_2->set_degree("PhD");

  // Write the message to log message.
  DiffRequest diff_request =
      ClientUtil::WriteMsgToDiffRequest(message_first, message_second);

  // Set the map value for repeated filed compare.
  std::string map_field_name = "education";
  std::vector<std::string> sub_field_list;
  sub_field_list.push_back("name");
  sub_field_list.push_back("degree");
  ClientUtil::TreatRepeatedFieldAsMap(&diff_request, map_field_name,
                                      sub_field_list);

  // Receive the differential response.
  DiffResponse diff_response;

  // Request the differential service.
  StatusCode service_response_status = TestStub_.CompareInputMessages(
      diff_request, &diff_response, server_address);

  // Check the return Status
  ASSERT_TRUE(StatusCode::OK == service_response_status);

  // Get the test result.
  const std::string& test_res = diff_response.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // The first message needs adding multiple elements than the seconde message.
  const char* except_res =
      "added: education[0]: { name: \"University of Dayton\" "
      "degree: \"PhD\" "
      "major: \"Computer Science and Engineering\" "
      "address: \"OH, US\" }\ndeleted: education[0]: { }\n";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 47: Test multiple sub fields as Map. message 2 is empty
TEST(DifferentialUnitTest, test_map_compare_second_message_empty) {
  TestEmployee message_first;
  TestEmployee message_second;

  EducationInfo* edu_info_1 = message_first.add_education();
  edu_info_1->set_name("Wright State University");
  edu_info_1->set_degree("PhD");
  edu_info_1->set_major("Computer Science");
  edu_info_1->set_address("OH, US");

  EducationInfo* edu_info_2 = message_second.add_education();

  // Write the message to log message.
  DiffRequest diff_request =
      ClientUtil::WriteMsgToDiffRequest(message_first, message_second);

  // Set the map value for repeated filed compare.
  std::string map_field_name = "education";
  std::vector<std::string> sub_field_list;
  sub_field_list.emplace_back("name");
  sub_field_list.emplace_back("degree");
  ClientUtil::TreatRepeatedFieldAsMap(&diff_request, map_field_name,
                                      sub_field_list);

  // Receive the differential response.
  DiffResponse diff_response;

  // Request the differential service.
  StatusCode service_response_status = TestStub_.CompareInputMessages(
      diff_request, &diff_response, server_address);

  // Check the return Status
  ASSERT_TRUE(StatusCode::OK == service_response_status);

  // Get the test result.
  const std::string& test_res = diff_response.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // The first message needs adding multiple elements than the second message.
  const char* except_res =
      "added: education[0]: { }\n"
      "deleted: education[0]: { name: \"Wright State University\" degree: "
      "\"PhD\" major: \"Computer Science\" address: \"OH, US\" }\n";

  EXPECT_STREQ(c_test_res, except_res);

  // Check the result error
  const std::string& test_error = diff_response.error();
  const char* c_test_error = test_error.c_str();
  const char* except_error = "";

  EXPECT_STREQ(c_test_error, except_error);
}

// Test 48: Test the fraction and margin for float number comparison
TEST(DifferentialUnitTest, test_fraction_margin_for_float_number_1) {
  TestEmployee message_first;
  TestEmployee message_second;

  message_first.set_floatpoint(100.0f);
  message_second.set_floatpoint(109.9f);

  // Write the message to log message.
  DiffRequest diff_request =
      ClientUtil::WriteMsgToDiffRequest(message_first, message_second);

  // Should fail since the fraction is smaller than error.
  ClientUtil::SetFractionAndMargin(&diff_request, 0.01, 0.0);

  // Receive the differential response.
  DiffResponse diff_response;

  // Request the differential service.
  StatusCode service_response_status = TestStub_.CompareInputMessages(
      diff_request, &diff_response, server_address);

  // Check the return Status
  ASSERT_TRUE(StatusCode::OK == service_response_status);

  // Get the test result.
  const std::string& test_res = diff_response.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // should fail.
  const char* except_res = "modified: floatpoint: 100 -> 109.90000152587891\n";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 49: Test the fraction and margin for float number comparison
TEST(DifferentialUnitTest, test_fraction_margin_for_float_number_2) {
  TestEmployee message_first;
  TestEmployee message_second;

  message_first.set_floatpoint(100.0f);
  message_second.set_floatpoint(109.9f);

  // Write the message to log message.
  DiffRequest diff_request =
      ClientUtil::WriteMsgToDiffRequest(message_first, message_second);

  // Should fail since the fraction is smaller than error.
  ClientUtil::SetFractionAndMargin(&diff_request, 0.2, 0.0);

  // Receive the differential response.
  DiffResponse diff_response;

  // Request the differential service.
  StatusCode service_response_status = TestStub_.CompareInputMessages(
      diff_request, &diff_response, server_address);

  // Check the return Status
  ASSERT_TRUE(StatusCode::OK == service_response_status);

  // Get the test result.
  const std::string& test_res = diff_response.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // should fail.
  const char* except_res = "SAME";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 50: Test the fraction and margin for float number comparison
TEST(DifferentialUnitTest, test_fraction_margin_for_float_number_3) {
  TestEmployee message_first;
  TestEmployee message_second;

  message_first.set_floatpoint(100.0f);
  message_second.set_floatpoint(109.9f);

  // Write the message to log message.
  DiffRequest diff_request =
      ClientUtil::WriteMsgToDiffRequest(message_first, message_second);

  // Should fail since the fraction is smaller than error.
  ClientUtil::SetFractionAndMargin(&diff_request, 0.01, 10.0);

  // Receive the differential response.
  DiffResponse diff_response;

  // Request the differential service.
  StatusCode service_response_status = TestStub_.CompareInputMessages(
      diff_request, &diff_response, server_address);

  // Check the return Status
  ASSERT_TRUE(StatusCode::OK == service_response_status);

  // Get the test result.
  const std::string& test_res = diff_response.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // should fail.
  const char* except_res = "SAME";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 51 Test the fraction and margin for double number comparison
TEST(DifferentialUnitTest, test_fraction_margin_for_double_number_1) {
  TestEmployee message_first;
  TestEmployee message_second;

  message_first.set_floatpoint(100.0);
  message_second.set_floatpoint(109.9);

  // Write the message to log message.
  DiffRequest diff_request =
      ClientUtil::WriteMsgToDiffRequest(message_first, message_second);

  // Should fail since the fraction is smaller than error.
  ClientUtil::SetFractionAndMargin(&diff_request, 0.01, 0.0);

  // Receive the differential response.
  DiffResponse diff_response;

  // Request the differential service.
  StatusCode service_response_status = TestStub_.CompareInputMessages(
      diff_request, &diff_response, server_address);

  // Check the return Status
  ASSERT_TRUE(StatusCode::OK == service_response_status);

  // Get the test result.
  const std::string& test_res = diff_response.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // should fail.
  const char* except_res = "modified: floatpoint: 100 -> 109.9\n";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 52 Test the fraction and margin for double number comparison
TEST(DifferentialUnitTest, test_fraction_margin_for_double_number_2) {
  TestEmployee message_first;
  TestEmployee message_second;

  message_first.set_floatpoint(100.0);
  message_second.set_floatpoint(109.9);

  // Write the message to log message.
  DiffRequest diff_request =
      ClientUtil::WriteMsgToDiffRequest(message_first, message_second);

  // Should fail since the fraction is smaller than error.
  ClientUtil::SetFractionAndMargin(&diff_request, 0.2, 0.0);

  // Receive the differential response.
  DiffResponse diff_response;

  // Request the differential service.
  StatusCode service_response_status = TestStub_.CompareInputMessages(
      diff_request, &diff_response, server_address);

  // Check the return Status
  ASSERT_TRUE(StatusCode::OK == service_response_status);

  // Get the test result.
  const std::string& test_res = diff_response.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // should fail.
  const char* except_res = "SAME";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 53 Test the fraction and margin for double number comparison
TEST(DifferentialUnitTest, test_fraction_margin_for_double_number_3) {
  TestEmployee message_first;
  TestEmployee message_second;

  message_first.set_floatpoint(100.0);
  message_second.set_floatpoint(109.9);

  // Write the message to log message.
  DiffRequest diff_request =
      ClientUtil::WriteMsgToDiffRequest(message_first, message_second);

  // Should fail since the fraction is smaller than error.
  ClientUtil::SetFractionAndMargin(&diff_request, 0.01, 10.0);

  // Receive the differential response.
  DiffResponse diff_response;

  // Request the differential service.
  StatusCode service_response_status = TestStub_.CompareInputMessages(
      diff_request, &diff_response, server_address);

  // Check the return Status
  ASSERT_TRUE(StatusCode::OK == service_response_status);

  // Get the test result.
  const std::string& test_res = diff_response.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // should fail.
  const char* except_res = "SAME";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 54: Test the ignore field isolated from repeated field set.
TEST(DifferentialUnitTest, test_ignore_and_treatASList) {
  TestEmployee message_first;
  TestEmployee message_second;

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
  DiffRequest diff_request =
      ClientUtil::WriteMsgToDiffRequest(message_first, message_second);

  // ignore the areas field
  std::vector<std::string> fields;
  std::string field_1("DifferentialTest.TestEmployee.areas");
  fields.push_back(field_1);
  ClientUtil::IgnoreFields(&diff_request, fields);

  // Set the field areas as list
  std::string repeated_field = "areas";
  ClientUtil::TreatRepeatedFieldAsListOrSet(&diff_request, true,
                                            repeated_field);

  // Receive the differential response.
  DiffResponse diff_response;

  // Request the differential service.
  StatusCode service_response_status = TestStub_.CompareInputMessages(
      diff_request, &diff_response, server_address);

  // Check the return Status
  ASSERT_TRUE(StatusCode::OK == service_response_status);

  // Get the test result.
  const std::string& test_res = diff_response.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // Because we ignore the area field so differential only at fullname will
  // presented.
  const char* except_res = "modified: fullname: \"Jin Huang\" -> \"Zhe Liu\"\n";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 55: Test the ignore field isolated from repeated field set.
TEST(DifferentialUnitTest, test_ignore_and_treatASList_treatASMap) {
  TestEmployee message_first;
  TestEmployee message_second;

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

  EducationInfo* edu_info_1 = message_first.add_education();
  edu_info_1->set_name("University of Dayton");
  edu_info_1->set_degree("PhD");
  edu_info_1->set_major("Computer Science");
  edu_info_1->set_address("OH, US");

  EducationInfo* edu_info_2 = message_second.add_education();
  edu_info_2->set_name("Wright State University");
  edu_info_2->set_degree("PhD");
  edu_info_2->set_major("Computer Science");
  edu_info_2->set_address("OH, US");

  // Write the message to log message.
  DiffRequest diff_request =
      ClientUtil::WriteMsgToDiffRequest(message_first, message_second);

  // ignore the areas field
  std::vector<std::string> fields;
  std::string field_1("DifferentialTest.TestEmployee.education");
  std::string field_2("DifferentialTest.TestEmployee.areas");
  fields.push_back(field_1);
  fields.push_back(field_2);
  ClientUtil::IgnoreFields(&diff_request, fields);

  // Set the field areas as list
  std::string repeated_field = "areas";
  ClientUtil::TreatRepeatedFieldAsListOrSet(&diff_request, true,
                                            repeated_field);

  // Set the map value for repeated filed compare.
  std::string map_field_name = "education";
  std::vector<std::string> sub_field_list;
  sub_field_list.push_back("name");
  ClientUtil::TreatRepeatedFieldAsMap(&diff_request, map_field_name,
                                      sub_field_list);

  // Receive the differential response.
  DiffResponse diff_response;

  // Request the differential service.
  StatusCode service_response_status = TestStub_.CompareInputMessages(
      diff_request, &diff_response, server_address);

  // Check the return Status
  ASSERT_TRUE(StatusCode::OK == service_response_status);

  // Get the test result.
  const std::string& test_res = diff_response.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  // Because we ignore the area field so differential only at fullname will
  // presented.
  const char* except_res = "modified: fullname: \"Jin Huang\" -> \"Zhe Liu\"\n";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 56: Test the differential service comprehensively
TEST(DifferentialUnitTest, test_ignore_and_treatASList_treatASMap_fractionAndMargin) {
  TestEmployee message_first;
  TestEmployee message_second;

  // write the message 1.
  message_first.set_employ_id(001);
  message_first.set_fullname("Jin Huang");
  message_first.set_age(32);

  Company* company_ptr = message_first.mutable_employer();
  company_ptr->set_name("Google Inc.");
  company_ptr->set_occupation("Softwear Engineer");
  company_ptr->set_address("CA, US");

  // set the working areas
  message_first.add_areas("Google Ads.");
  message_first.add_areas("YouTube Ads.");
  message_first.add_areas("Search Ads.");
  message_first.add_areas("Click Ads.");

  // set the education info No. 1
  EducationInfo* edu_info_1 = message_first.add_education();
  edu_info_1->set_name("Wright State University");
  edu_info_1->set_degree("PhD");
  edu_info_1->set_major("Computer Science");
  edu_info_1->set_address("OH, US");

  // set the dependent info
  DifferentialTest::DependentInfo* dependentInfo_ptr_1 =
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

  Company* company_ptr_2 = message_second.mutable_employer();
  company_ptr_2->set_name("Google Inc.");
  company_ptr_2->set_occupation("Softweare Engineer");
  company_ptr_2->set_address("CA, US");

  // set the working areas
  message_second.add_areas("Google Ads.");
  message_second.add_areas("YouTube Ads.");
  message_second.add_areas("Search Ads.");
  message_second.add_areas("Click Ads.");

  // set the education info No. 1
  EducationInfo* edu_info_2 = message_second.add_education();
  edu_info_2->set_name("Wright State University");
  edu_info_2->set_degree("PhD");
  edu_info_2->set_major("Computer Science");
  edu_info_2->set_address("OH, US");

  // set the dependent info
  DifferentialTest::DependentInfo* dependentInfo_ptr_2 =
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
  double num2 = 109.9;
  message_second.set_floatpoint(num2);

  // Write the message to log message.
  DiffRequest diff_request =
      ClientUtil::WriteMsgToDiffRequest(message_first, message_second);

  // add ignore fields
  std::vector<std::string> ignore_fields;
  std::string field_1("DifferentialTest.TestEmployee.employ_id");
  std::string field_2("DifferentialTest.Company.address");
  std::string field_3("DifferentialTest.EducationInfo.address");

  ignore_fields.push_back(field_1);
  ignore_fields.push_back(field_2);
  ignore_fields.push_back(field_3);
  ClientUtil::IgnoreFields(&diff_request, ignore_fields);

  // Set the field areas as list
  std::string repeated_field_1("areas");
  ClientUtil::TreatRepeatedFieldAsListOrSet(&diff_request, true,
                                            repeated_field_1);

  // Set the field dependents.name as set
  std::string repeated_field_2("dependents.name");
  ClientUtil::TreatRepeatedFieldAsListOrSet(&diff_request, false,
                                            repeated_field_2);

  // Set the field dependents.age as set
  std::string repeated_field_3("dependents.age");
  ClientUtil::TreatRepeatedFieldAsListOrSet(&diff_request, false,
                                            repeated_field_3);

  // Set the map value comparison
  std::string map_field_name("education");
  std::vector<std::string> sub_field_list;
  sub_field_list.emplace_back("name");
  sub_field_list.emplace_back("degree");
  ClientUtil::TreatRepeatedFieldAsMap(&diff_request, map_field_name,
                                      sub_field_list);

  // set the fraction and margin
  ClientUtil::SetFractionAndMargin(&diff_request, 0.01, 0.0);

  // Receive the differential response.
  DiffResponse diff_response;

  // Request the differential service.
  StatusCode service_response_status = TestStub_.CompareInputMessages(
      diff_request, &diff_response, server_address);

  // Check the return Status
  ASSERT_TRUE(StatusCode::OK == service_response_status);

  // Get the test result.
  const std::string& test_res = diff_response.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  const char* except_res =
      "modified: fullname: \"Jin Huang\" -> \"Zhe Liu\"\n"
      "modified: employer.occupation: \"Softwear Engineer\" -> \"Softweare "
      "Engineer\"\n"
      "modified: floatpoint: 100 -> 109.9\n";

  EXPECT_STREQ(c_test_res, except_res);
}

// Test 57: Test the maximum size of input message.
TEST(DifferentialUnitTest, test_maximum_size_of_input_message_with_1000_repeated_field) {
  /*
   *  In this testing case, we will try to input two same messages with 50000
   *  repeated "education" fields.
   */
  TestEmployee message_first;
  TestEmployee message_second;

  for (int i = 0; i < 1000; ++i) {
    int num = i;
    std::string name_1 = "A";
    name_1 = name_1 + "_" + std::to_string(num);

    std::string name_2 = "A";
    name_2 = name_2 + "_" + std::to_string(num);

    std::string degree = "PhD";
    std::string major = "Computer Science";
    std::string address = "CA, US";

    EducationInfo* edu_1 = message_first.add_education();
    edu_1->set_name(name_1);
    edu_1->set_degree(degree);
    edu_1->set_major(major);
    edu_1->set_address(address);

    EducationInfo* edu_2 = message_second.add_education();
    edu_2->set_name(name_2);
    edu_2->set_degree(degree);
    edu_2->set_major(major);
    edu_2->set_address(address);
  }

  // Write the message to log message.
  DiffRequest diff_request =
      ClientUtil::WriteMsgToDiffRequest(message_first, message_second);

  // Receive the differential response.
  DiffResponse diff_response;

  // Request the differential service.
  StatusCode service_response_status = TestStub_.CompareInputMessages(
      diff_request, &diff_response, server_address);

  // Check the return Status
  ASSERT_TRUE(StatusCode::OK == service_response_status);

  // Get the test result.
  const std::string& test_res = diff_response.result();

  // Casting to char* for unit test.
  const char* c_test_res = test_res.c_str();

  const char* except_res = "SAME";

  EXPECT_STREQ(c_test_res, except_res);

  // Check the result error
  const std::string& test_error = diff_response.error();
  const char* c_test_error = test_error.c_str();
  const char* except_error = "";

  EXPECT_STREQ(c_test_error, except_error);
}

// TEST end line.
}  // namespace google

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  // Create a global test environment for all test cases.
  testing::AddGlobalTestEnvironment(new DifferentialTestEnvironment());
  return RUN_ALL_TESTS();
}
