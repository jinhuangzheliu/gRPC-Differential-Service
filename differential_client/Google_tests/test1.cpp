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
#include "differential_client.grpc.pb.h"
#include "ServiceClient.h"

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
using differentialservice::log;

using differential_client::company;
using differential_client::education_info;
using differential_client::dependent_info;
using differential_client::field_set;
using differential_client::employee;

namespace google {

// Test 1: Check the connect between the client and server.
TEST(Differential_unit, Connect) {
  // Establish the gRPC channel with port number.
  std::string target_str;
  target_str = "0.0.0.0:50053";
  ServiceClient serviceClient(
      grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));

  std::string user(" Established");
  std::string reply = serviceClient.GetConnect(user);

  EXPECT_EQ(reply, "Contect has Established");
}

// Test 2: Test the default differential service.
TEST(Differential_unit, default_diff) {
  // Establish the gRPC channel with port number.
  std::string target_str;
  target_str = "0.0.0.0:50053";
  ServiceClient serviceClient(
      grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));

  employee message_first;
  employee message_second;
  log log_message;

  message_first.set_fullname("Jin Huang");
  message_first.set_age(32);

  message_second.set_fullname("Zhe Liu");
  message_second.set_age(32);

  ServiceClient::messageWriter(&message_first, &message_second, &log_message);

  std::string res = serviceClient.DefaultDifferentialService(
      message_first, message_second, log_message);

  const char* c_res = res.c_str();

  const char* test_res = "modified: fullname: \"Jin Huang\" -> \"Zhe Liu\"\n";

  EXPECT_STREQ(c_res, test_res);
}

// Test 3: Test the single field ignore.
TEST(Differential_unit, ignore_1) {
  // Establish the gRPC channel with port number.
  std::string target_str;
  target_str = "0.0.0.0:50053";
  ServiceClient serviceClient(
      grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));

  employee message_first;
  employee message_second;
  log log_message;

  message_first.set_fullname("Jin Huang");
  message_first.set_age(32);

  message_second.set_fullname("Zhe Liu");
  message_second.set_age(32);

  ServiceClient::messageWriter(&message_first, &message_second, &log_message);

  std::vector<std::string> fields;
  std::string field_1("differential_client.employee.fullname");
  fields.push_back(field_1);
  ServiceClient::blackListCriteria(log_message, fields);

  std::string res = serviceClient.DifferentialService(
      message_first, message_second, log_message );

  const char* c_res = res.c_str();

  const char* test_res = "SAME";

  EXPECT_STREQ(c_res, test_res);
}

// Test 4: Test the multiple fields ignore
TEST(Differential_unit, ignore_2) {
  // Establish the gRPC channel with port number.
  std::string target_str;
  target_str = "0.0.0.0:50053";
  ServiceClient serviceClient(
      grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));

  employee message_first;
  employee message_second;
  log log_message;

  message_first.set_employ_id(001);
  message_first.set_fullname("Jin Huang");
  message_first.set_age(39);

  message_second.set_employ_id(002);
  message_second.set_fullname("Zhe Liu");
  message_second.set_age(32);

  ServiceClient::messageWriter(&message_first, &message_second, &log_message);

  std::vector<std::string> fields;
  std::string field_1("differential_client.employee.fullname");
  std::string field_2("differential_client.employee.employ_id");

  fields.push_back(field_1);
  fields.push_back(field_2);

  ServiceClient::blackListCriteria(log_message, fields);

  std::string res = serviceClient.DifferentialService(
      message_first, message_second, log_message );

  const char* c_res = res.c_str();

  const char* test_res = "modified: age: 39 -> 32\n";

  EXPECT_STREQ(c_res, test_res);
}

// Test 5: Test the multiple fields ignore include the nested field.
TEST(Differential_unit, ignore_3) {
  // Establish the gRPC channel with port number.
  std::string target_str;
  target_str = "0.0.0.0:50053";
  ServiceClient serviceClient(
      grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));

  employee message_first;
  employee message_second;
  log log_message;

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

  // Write the message to log message.
  ServiceClient::messageWriter(&message_first, &message_second, &log_message);

  // Add the ignore fields. we add two more field nested under the message employee.
  std::vector<std::string> ignore_fields;
  std::string field_1("differential_client.employee.fullname");
  std::string field_2("differential_client.employee.employ_id");

  std::string nested_field_1("differential_client.company.name");
  std::string nested_field_2("differential_client.company.occupation");

  // push the fields
  ignore_fields.push_back(field_1);
  ignore_fields.push_back(field_2);

  // push the nested fields.
  ignore_fields.push_back(nested_field_1);
  ignore_fields.push_back(nested_field_2);

  // Black list criteria will do the ignore
  ServiceClient::blackListCriteria(log_message, ignore_fields);

  // Implements the differential service.
  std::string res = serviceClient.DifferentialService(
      message_first, message_second, log_message );

  const char* c_res = res.c_str();

  // Because ignore the employee.fullname, employee.employ_id, company.name,
  // and  company.occupation so the only different element is age of the employee.
  const char* test_res = "modified: age: 39 -> 32\n";

  EXPECT_STREQ(c_res, test_res);
}

// Test 6: Test ignore nothing
TEST(Differential_unit, ignore_4) {
  // Establish the gRPC channel with port number.
  std::string target_str;
  target_str = "0.0.0.0:50053";
  ServiceClient serviceClient(
      grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));

  employee message_first;
  employee message_second;
  log log_message;

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

  // Write the message to log message.
  ServiceClient::messageWriter(&message_first, &message_second, &log_message);

  // Add the ignore fields. we add two more field nested under the message employee.
  std::vector<std::string> ignore_fields;

  // Leave the ignore_fields as empty.

  // Black list criteria will do the ignore
  ServiceClient::blackListCriteria(log_message, ignore_fields);

  // Implements the differential service.
  std::string res = serviceClient.DifferentialService(
      message_first, message_second, log_message );

  const char* c_res = res.c_str();

  const char* test_res = "modified: employ_id: 1 -> 2\n"
      "modified: fullname: \"Jin Huang\" -> \"Zhe Liu\"\n"
      "modified: age: 39 -> 32\n"
      "modified: employer.name: \"Google Inc.\" -> \"Alphabet Inc.\"\n"
      "modified: employer.occupation: \"Software Engineer\" -> \"Research Scientist\"\n";
  EXPECT_STREQ(c_res, test_res);
}

// Test 7: Test ignore all fields in message.
TEST(Differential_unit, ignore_5) {
  // Establish the gRPC channel with port number.
  std::string target_str;
  target_str = "0.0.0.0:50053";
  ServiceClient serviceClient(
      grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));

  employee message_first;
  employee message_second;
  log log_message;

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

  // Write the message to log message.
  ServiceClient::messageWriter(&message_first, &message_second, &log_message);

  // Add the ignore fields. we add two more field nested under the message employee.
  std::vector<std::string> ignore_fields;

  std::string field_1("differential_client.employee.fullname");
  std::string field_2("differential_client.employee.employ_id");
  std::string field_3("differential_client.employee.age");

  std::string nested_field_1("differential_client.company.name");
  std::string nested_field_2("differential_client.company.occupation");
  std::string nested_field_3("differential_client.company.address");

  // push the fields
  ignore_fields.push_back(field_1);
  ignore_fields.push_back(field_2);
  ignore_fields.push_back(field_3);

  // push the nested fields.
  ignore_fields.push_back(nested_field_1);
  ignore_fields.push_back(nested_field_2);
  ignore_fields.push_back(nested_field_3);

  // Black list criteria will do the ignore
  ServiceClient::blackListCriteria(log_message, ignore_fields);

  // Implements the differential service.
  std::string res = serviceClient.DifferentialService(
      message_first, message_second, log_message );

  const char* c_res = res.c_str();

  // Because ignore all field in the message test value should return "SAME"
  const char* test_res = "SAME";
  EXPECT_STREQ(c_res, test_res);
}

// Test 8: Test compare a single field.
TEST(Differential_unit, compare_1){
  // Establish the gRPC channel with port number.
  std::string target_str;
  target_str = "0.0.0.0:50053";
  ServiceClient serviceClient(
      grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));

  employee message_first;
  employee message_second;
  log log_message;

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

  // Write the message to log message.
  ServiceClient::messageWriter(&message_first, &message_second, &log_message);

  // Add the ignore fields. we add two more field nested under the message employee.
  std::vector<std::string> compare_fields;

  std::string field_1("differential_client.employee.fullname");

  // push the fields
  compare_fields.push_back(field_1);

  // White list criteria will do the ignore
  ServiceClient::whiteListCriteria(log_message, compare_fields);

  // Implements the differential service.
  std::string res = serviceClient.DifferentialService(
      message_first, message_second, log_message );

  const char* c_res = res.c_str();

  // Because only employee.fullname is compare so the result should be.
  const char* test_res = "modified: fullname: \"Jin Huang\" -> \"Zhe Liu\"\n";
  EXPECT_STREQ(c_res, test_res);
}

// Test 9: Test compare multiple fields.
TEST(Differential_unit, compare_2){
  // Establish the gRPC channel with port number.
  std::string target_str;
  target_str = "0.0.0.0:50053";
  ServiceClient serviceClient(
      grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));

  employee message_first;
  employee message_second;
  log log_message;

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

  // Write the message to log message.
  ServiceClient::messageWriter(&message_first, &message_second, &log_message);

  // Add the ignore fields. we add two more field nested under the message employee.
  std::vector<std::string> compare_fields;

  std::string field_1("differential_client.employee.fullname");
  std::string field_2("differential_client.employee.employ_id");
  std::string field_3("differential_client.employee.age");

  // push the fields
  compare_fields.push_back(field_1);
  compare_fields.push_back(field_2);
  compare_fields.push_back(field_3);

  // White list criteria will do the ignore
  ServiceClient::whiteListCriteria(log_message, compare_fields);

  // Implements the differential service.
  std::string res = serviceClient.DifferentialService(
      message_first, message_second, log_message );

  const char* c_res = res.c_str();

  // Because only employee.fullname, employee.employ_id, and
  // employee.age are compare so the result should be.
  const char* test_res = "modified: employ_id: 1 -> 2\n"
                         "modified: fullname: \"Jin Huang\" -> \"Zhe Liu\"\n"
                         "modified: age: 39 -> 32\n";
  EXPECT_STREQ(c_res, test_res);
}

// Test 10: Test compare a nested field in message.
TEST(Differential_unit, compare_3){
  // Establish the gRPC channel with port number.
  std::string target_str;
  target_str = "0.0.0.0:50053";
  ServiceClient serviceClient(
      grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));

  employee message_first;
  employee message_second;
  log log_message;

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

  // Write the message to log message.
  ServiceClient::messageWriter(&message_first, &message_second, &log_message);

  // Add the ignore fields. we add two more field nested under the message employee.
  std::vector<std::string> compare_fields;

  // because we comapre a nested field so we have to add its parent field first.
  std::string parent_field_1("differential_client.employee.employer");
  std::string nested_field_1("differential_client.company.name");

  // push the nested fields.
  compare_fields.push_back(parent_field_1);
  compare_fields.push_back(nested_field_1);

  // White list criteria will do the ignore
  ServiceClient::whiteListCriteria(log_message, compare_fields);

  // Implements the differential service.
  std::string res = serviceClient.DifferentialService(
      message_first, message_second, log_message );

  const char* c_res = res.c_str();

  // we compare a nested field named company.name that under the message employee
  const char* test_res = "modified: employer.name: \"Google Inc.\" -> \"Alphabet Inc.\"\n";

  EXPECT_STREQ(c_res, test_res);
}

// Test 11: Test compare multiple nested fields in message.
TEST(Differential_unit, compare_4){
  // Establish the gRPC channel with port number.
  std::string target_str;
  target_str = "0.0.0.0:50053";
  ServiceClient serviceClient(
      grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));

  employee message_first;
  employee message_second;
  log log_message;

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

  // Write the message to log message.
  ServiceClient::messageWriter(&message_first, &message_second, &log_message);

  // Add the ignore fields. we add two more field nested under the message employee.
  std::vector<std::string> compare_fields;

  // because we comapre a nested field so we have to add its parent field first.
  std::string parent_field_1("differential_client.employee.employer");
  std::string nested_field_1("differential_client.company.name");
  std::string nested_field_2("differential_client.company.occupation");
  std::string nested_field_3("differential_client.company.address");

  // push the nested fields.
  compare_fields.push_back(parent_field_1);
  compare_fields.push_back(nested_field_1);
  compare_fields.push_back(nested_field_2);
  compare_fields.push_back(nested_field_3);

  // White list criteria will do the ignore
  ServiceClient::whiteListCriteria(log_message, compare_fields);

  // Implements the differential service.
  std::string res = serviceClient.DifferentialService(
      message_first, message_second, log_message );

  const char* c_res = res.c_str();

  // we compare multiple fields that under the message employee
  const char* test_res = "modified: employer.name: \"Google Inc.\" -> \"Alphabet Inc.\"\n"
                         "modified: employer.occupation: \"Software Engineer\" -> \"Research Scientist\"\n";

  EXPECT_STREQ(c_res, test_res);
}

// Test 12: Test compare no field in message. If the user want to compare nothing
// (leave the compare fields blank), our service will compare every fields in
// their message.
TEST(Differential_unit, compare_5){
  // Establish the gRPC channel with port number.
  std::string target_str;
  target_str = "0.0.0.0:50053";
  ServiceClient serviceClient(
      grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));

  employee message_first;
  employee message_second;
  log log_message;

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

  // Write the message to log message.
  ServiceClient::messageWriter(&message_first, &message_second, &log_message);

  // Add the ignore fields. we add two more field nested under the message employee.
  std::vector<std::string> compare_fields;

  // Leave the compare fields empty.

  // White list criteria will do the ignore
  ServiceClient::whiteListCriteria(log_message, compare_fields);

  // Implements the differential service.
  std::string res = serviceClient.DifferentialService(
      message_first, message_second, log_message );

  const char* c_res = res.c_str();

  // we compare multiple fields that under the message employee
  const char* test_res = "modified: employ_id: 1 -> 2\n"
                         "modified: fullname: \"Jin Huang\" -> \"Zhe Liu\"\n"
                         "modified: age: 39 -> 32\n"
                         "modified: employer.name: \"Google Inc.\" -> \"Alphabet Inc.\"\n"
                         "modified: employer.occupation: \"Software Engineer\" -> \"Research Scientist\"\n";

  EXPECT_STREQ(c_res, test_res);
}

// Test 13: Test the repeated field treat as LIST.
TEST(Differential_unit, repeated_list_1){
  // Establish the gRPC channel with port number.
  std::string target_str;
  target_str = "0.0.0.0:50053";
  ServiceClient serviceClient(
      grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));

  employee message_first;
  employee message_second;
  log log_message;

  // Write two message with same value and same order.
  message_first.add_areas("Google Ads.");
  message_first.add_areas("YouTube Ads.");
  message_first.add_areas("Search Ads.");
  message_first.add_areas("Click Ads.");

  message_second.add_areas("Google Ads.");
  message_second.add_areas("YouTube Ads.");
  message_second.add_areas("Search Ads.");
  message_second.add_areas("Click Ads.");


  // Write the message to log message.
  ServiceClient::messageWriter(&message_first, &message_second, &log_message);

  // Set the field areas as list
  std::string field_1 = "areas";
  ServiceClient::treat_repeated_field_list_or_set(log_message, 0, field_1);

  // Implements the differential service.
  std::string res = serviceClient.DifferentialService(
      message_first, message_second, log_message );

  const char* c_res = res.c_str();

  // we test the field areas have the same value with the same order.
  const char* test_res = "SAME";

  EXPECT_STREQ(c_res, test_res);
}

// Test 14: Test the repeated field treat as LIST.
TEST(Differential_unit, repeated_list_2){
  // Establish the gRPC channel with port number.
  std::string target_str;
  target_str = "0.0.0.0:50053";
  ServiceClient serviceClient(
      grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));

  employee message_first;
  employee message_second;
  log log_message;

  // Write two message with same value and same order.
  message_first.add_areas("Google Ads.");
  message_first.add_areas("YouTube Ads.");
  message_first.add_areas("Search Ads.");
  message_first.add_areas("Click Ads.");

  message_second.add_areas("Google Ads.");
  message_second.add_areas("YouTube Ads.");
  message_second.add_areas("Click Ads.");
  message_second.add_areas("Search Ads.");

  // Write the message to log message.
  ServiceClient::messageWriter(&message_first, &message_second, &log_message);

  // Set the field areas as list
  std::string field_1 = "areas";
  ServiceClient::treat_repeated_field_list_or_set(log_message, 0, field_1);

  // Implements the differential service.
  std::string res = serviceClient.DifferentialService(
      message_first, message_second, log_message );

  const char* c_res = res.c_str();

  // we test the field areas have the same value with the same order.
  const char* test_res = "modified: areas[2]: \"Search Ads.\" -> \"Click Ads.\"\n"
      "modified: areas[3]: \"Click Ads.\" -> \"Search Ads.\"\n";

  EXPECT_STREQ(c_res, test_res);
}

// Test 15: Test a nested repeated field treat as LIST.
TEST(Differential_unit, repeated_list_3){
  // Establish the gRPC channel with port number.
  std::string target_str;
  target_str = "0.0.0.0:50053";
  ServiceClient serviceClient(
      grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));

  employee message_first;
  employee message_second;
  log log_message;

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
  differential_client::dependent_info* dependentInfo_ptr_1 =
      message_first.mutable_dependents();
  dependentInfo_ptr_1->add_name("Jeremy");
  dependentInfo_ptr_1->add_name("Zhe");
  dependentInfo_ptr_1->add_name("Jin");
  dependentInfo_ptr_1->add_name("June");

  differential_client::dependent_info* dependentInfo_ptr_2 =
      message_second.mutable_dependents();

  dependentInfo_ptr_2->add_name("Zhe");
  dependentInfo_ptr_2->add_name("Jeremy");
  dependentInfo_ptr_2->add_name("June");
  dependentInfo_ptr_2->add_name("Jin");


  // Write the message to log message.
  ServiceClient::messageWriter(&message_first, &message_second, &log_message);

  // Set the field areas as list
  std::string field_1 = "areas";
  ServiceClient::treat_repeated_field_list_or_set(log_message, 0, field_1);
  std::string field_2 = "dependents.name";
  ServiceClient::treat_repeated_field_list_or_set(log_message, 0, field_2);
//  std::string field_3 = "dependents.age";
//  ServiceClient::treat_repeated_field_list_or_set(log_message, 0, field_3);

  // Implements the differential service.
  std::string res = serviceClient.DifferentialService(
      message_first, message_second, log_message );

  const char* c_res = res.c_str();

  // we test the field areas have the same value with the same order.
  const char* test_res = "modified: dependents.name[0]: \"Jeremy\" -> \"Zhe\"\n"
      "modified: dependents.name[1]: \"Zhe\" -> \"Jeremy\"\n"
      "modified: dependents.name[2]: \"Jin\" -> \"June\"\n"
      "modified: dependents.name[3]: \"June\" -> \"Jin\"\n";

  EXPECT_STREQ(c_res, test_res);
}

// Test 16: Test multiple nested repeated fields treat as LIST.
TEST(Differential_unit, repeated_list_4){
  // Establish the gRPC channel with port number.
  std::string target_str;
  target_str = "0.0.0.0:50053";
  ServiceClient serviceClient(
      grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));

  employee message_first;
  employee message_second;
  log log_message;

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
  differential_client::dependent_info* dependentInfo_ptr_1 =
      message_first.mutable_dependents();
  dependentInfo_ptr_1->add_name("Jeremy");
  dependentInfo_ptr_1->add_name("Zhe");
  dependentInfo_ptr_1->add_name("Jin");
  dependentInfo_ptr_1->add_name("June");

  dependentInfo_ptr_1->add_age(29);
  dependentInfo_ptr_1->add_age(30);
  dependentInfo_ptr_1->add_age(32);
  dependentInfo_ptr_1->add_age(26);

  differential_client::dependent_info* dependentInfo_ptr_2 =
      message_second.mutable_dependents();

  dependentInfo_ptr_2->add_name("Zhe");
  dependentInfo_ptr_2->add_name("Jeremy");
  dependentInfo_ptr_2->add_name("June");
  dependentInfo_ptr_2->add_name("Jin");

  dependentInfo_ptr_2->add_age(29);
  dependentInfo_ptr_2->add_age(32);
  dependentInfo_ptr_2->add_age(30);
  dependentInfo_ptr_2->add_age(26);


  // Write the message to log message.
  ServiceClient::messageWriter(&message_first, &message_second, &log_message);

  // Set the field areas as list
  std::string field_1 = "areas";
  ServiceClient::treat_repeated_field_list_or_set(log_message, 0, field_1);
  std::string field_2 = "dependents.name";
  ServiceClient::treat_repeated_field_list_or_set(log_message, 0, field_2);
  std::string field_3 = "dependents.age";
  ServiceClient::treat_repeated_field_list_or_set(log_message, 0, field_3);

  // Implements the differential service.
  std::string res = serviceClient.DifferentialService(
      message_first, message_second, log_message );

  const char* c_res = res.c_str();

  // we test the field areas have the same value with the same order.
  const char* test_res = "modified: dependents.name[0]: \"Jeremy\" -> \"Zhe\"\n"
                         "modified: dependents.name[1]: \"Zhe\" -> \"Jeremy\"\n"
                         "modified: dependents.name[2]: \"Jin\" -> \"June\"\n"
                         "modified: dependents.name[3]: \"June\" -> \"Jin\"\n"
                         "modified: dependents.age[1]: 30 -> 32\n"
                         "modified: dependents.age[2]: 32 -> 30\n";

  EXPECT_STREQ(c_res, test_res);
}

// Test 17: Test multiple nested repeated fields treat as LIST.
TEST(Differential_unit, repeated_list_5){
  // Establish the gRPC channel with port number.
  std::string target_str;
  target_str = "0.0.0.0:50053";
  ServiceClient serviceClient(
      grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));

  employee message_first;
  employee message_second;
  log log_message;

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
  differential_client::dependent_info* dependentInfo_ptr_1 =
      message_first.mutable_dependents();
  dependentInfo_ptr_1->add_name("Jeremy");
  dependentInfo_ptr_1->add_name("Zhe");
  dependentInfo_ptr_1->add_name("Jin");
  dependentInfo_ptr_1->add_name("June");

  dependentInfo_ptr_1->add_age(29);
  dependentInfo_ptr_1->add_age(30);
  dependentInfo_ptr_1->add_age(32);
  dependentInfo_ptr_1->add_age(26);

  differential_client::dependent_info* dependentInfo_ptr_2 =
      message_second.mutable_dependents();

  dependentInfo_ptr_2->add_name("Jeremy");
  dependentInfo_ptr_2->add_name("Zhe");
  dependentInfo_ptr_2->add_name("Jin");
  dependentInfo_ptr_2->add_name("June");

  dependentInfo_ptr_2->add_age(29);
  dependentInfo_ptr_2->add_age(30);
  dependentInfo_ptr_2->add_age(32);
  dependentInfo_ptr_2->add_age(26);


  // Write the message to log message.
  ServiceClient::messageWriter(&message_first, &message_second, &log_message);

  // Set the field areas as list
  std::string field_1 = "areas";
  ServiceClient::treat_repeated_field_list_or_set(log_message, 0, field_1);
  std::string field_2 = "dependents.name";
  ServiceClient::treat_repeated_field_list_or_set(log_message, 0, field_2);
  std::string field_3 = "dependents.age";
  ServiceClient::treat_repeated_field_list_or_set(log_message, 0, field_3);

  // Implements the differential service.
  std::string res = serviceClient.DifferentialService(
      message_first, message_second, log_message );

  const char* c_res = res.c_str();

  // we test the field areas have the same value with the same order.
  const char* test_res = "SAME";

  EXPECT_STREQ(c_res, test_res);
}

// Test 18: Test the repeated field treat as LIST. deleted element.
TEST(Differential_unit, repeated_list_6){
  // Establish the gRPC channel with port number.
  std::string target_str;
  target_str = "0.0.0.0:50053";
  ServiceClient serviceClient(
      grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));

  employee message_first;
  employee message_second;
  log log_message;

  // Write two message with same value and same order.
  message_first.add_areas("Google Ads.");
  message_first.add_areas("YouTube Ads.");
  message_first.add_areas("Search Ads.");
  message_first.add_areas("Click Ads.");

  message_second.add_areas("Google Ads.");
  message_second.add_areas("YouTube Ads.");
  message_second.add_areas("Search Ads.");


  // Write the message to log message.
  ServiceClient::messageWriter(&message_first, &message_second, &log_message);

  // Set the field areas as list
  std::string field_1 = "areas";
  ServiceClient::treat_repeated_field_list_or_set(log_message, 0, field_1);

  // Implements the differential service.
  std::string res = serviceClient.DifferentialService(
      message_first, message_second, log_message );

  const char* c_res = res.c_str();

  // we test the field areas have the same value with the same order.
  const char* test_res = "deleted: areas[3]: \"Click Ads.\"\n";

  EXPECT_STREQ(c_res, test_res);
}

// Test 19: Test the repeated field treat as LIST. add element.
TEST(Differential_unit, repeated_list_7){
  // Establish the gRPC channel with port number.
  std::string target_str;
  target_str = "0.0.0.0:50053";
  ServiceClient serviceClient(
      grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));

  employee message_first;
  employee message_second;
  log log_message;

  // Write two message with same value and same order.
  message_first.add_areas("Google Ads.");
  message_first.add_areas("YouTube Ads.");
  message_first.add_areas("Search Ads.");


  message_second.add_areas("Google Ads.");
  message_second.add_areas("YouTube Ads.");
  message_second.add_areas("Search Ads.");
  message_second.add_areas("Click Ads.");


  // Write the message to log message.
  ServiceClient::messageWriter(&message_first, &message_second, &log_message);

  // Set the field areas as list
  std::string field_1 = "areas";
  ServiceClient::treat_repeated_field_list_or_set(log_message, 0, field_1);

  // Implements the differential service.
  std::string res = serviceClient.DifferentialService(
      message_first, message_second, log_message );

  const char* c_res = res.c_str();

  // we test the field areas have the same value with the same order.
  const char* test_res = "added: areas[3]: \"Click Ads.\"\n";

  EXPECT_STREQ(c_res, test_res);
}

// Test 20: Test the repeated field treat as SET.
TEST(Differential_unit, repeated_set_1){
  // Establish the gRPC channel with port number.
  std::string target_str;
  target_str = "0.0.0.0:50053";
  ServiceClient serviceClient(
      grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));

  employee message_first;
  employee message_second;
  log log_message;

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
  ServiceClient::messageWriter(&message_first, &message_second, &log_message);

  // Set the field areas as set
  std::string field_1 = "areas";
  ServiceClient::treat_repeated_field_list_or_set(log_message, 1, field_1);

  // Implements the differential service.
  std::string res = serviceClient.DifferentialService(
      message_first, message_second, log_message );

  const char* c_res = res.c_str();

  // Because we treat the repeated field as set so even the element have different
  // order between the two messages the result is same.
  const char* test_res = "SAME";

  EXPECT_STREQ(c_res, test_res);
}

// Test 21: Test the repeated field treat as SET.
TEST(Differential_unit, repeated_set_2){
  // Establish the gRPC channel with port number.
  std::string target_str;
  target_str = "0.0.0.0:50053";
  ServiceClient serviceClient(
      grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));

  employee message_first;
  employee message_second;
  log log_message;

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
  ServiceClient::messageWriter(&message_first, &message_second, &log_message);

  // Set the field areas as set
  std::string field_1 = "areas";
  ServiceClient::treat_repeated_field_list_or_set(log_message, 1, field_1);

  // Implements the differential service.
  std::string res = serviceClient.DifferentialService(
      message_first, message_second, log_message );

  const char* c_res = res.c_str();

  // The first message needs deleting the areas[1] and add "Pop_up Ads.".
  const char* test_res = "added: areas[3]: \"Pop_up Ads.\"\ndeleted: areas[1]: \"YouTube Ads.\"\n";

  EXPECT_STREQ(c_res, test_res);
}

// Test 22: Test the repeated field treat as SET. delete element.
TEST(Differential_unit, repeated_set_3){
  // Establish the gRPC channel with port number.
  std::string target_str;
  target_str = "0.0.0.0:50053";
  ServiceClient serviceClient(
      grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));

  employee message_first;
  employee message_second;
  log log_message;

  // Write two message with different value
  message_first.add_areas("Google Ads.");
  message_first.add_areas("YouTube Ads."); // The different one.
  message_first.add_areas("Search Ads.");
  message_first.add_areas("Click Ads.");

  message_second.add_areas("Search Ads.");
  message_second.add_areas("Click Ads.");
  message_second.add_areas("Google Ads.");



  // Write the message to log message.
  ServiceClient::messageWriter(&message_first, &message_second, &log_message);

  // Set the field areas as set
  std::string field_1 = "areas";
  ServiceClient::treat_repeated_field_list_or_set(log_message, 1, field_1);

  // Implements the differential service.
  std::string res = serviceClient.DifferentialService(
      message_first, message_second, log_message );

  const char* c_res = res.c_str();

  // The first message needs deleting the areas[1]
  const char* test_res = "deleted: areas[1]: \"YouTube Ads.\"\n";

  EXPECT_STREQ(c_res, test_res);
}

// Test 23: Test the repeated field treat as SET. Add element
TEST(Differential_unit, repeated_set_4){
  // Establish the gRPC channel with port number.
  std::string target_str;
  target_str = "0.0.0.0:50053";
  ServiceClient serviceClient(
      grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));

  employee message_first;
  employee message_second;
  log log_message;

  // Write two message with different value
  message_first.add_areas("Google Ads.");
  message_first.add_areas("Search Ads.");
  message_first.add_areas("Click Ads.");

  message_second.add_areas("Search Ads.");
  message_second.add_areas("Click Ads.");
  message_second.add_areas("Google Ads.");
  message_second.add_areas("Pop_up Ads."); // The different one.

  // Write the message to log message.
  ServiceClient::messageWriter(&message_first, &message_second, &log_message);

  // Set the field areas as set
  std::string field_1 = "areas";
  ServiceClient::treat_repeated_field_list_or_set(log_message, 1, field_1);

  // Implements the differential service.
  std::string res = serviceClient.DifferentialService(
      message_first, message_second, log_message );

  const char* c_res = res.c_str();

  // The first message needs adding "Pop_up Ads.".
  const char* test_res = "added: areas[3]: \"Pop_up Ads.\"\n";

  EXPECT_STREQ(c_res, test_res);
}

// Test 24: Test the repeated field treat as SET. adding mulitpe elements
TEST(Differential_unit, repeated_set_5){
  // Establish the gRPC channel with port number.
  std::string target_str;
  target_str = "0.0.0.0:50053";
  ServiceClient serviceClient(
      grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));

  employee message_first;
  employee message_second;
  log log_message;

  // Write two message with different value
  message_first.add_areas("Google Ads.");

  message_second.add_areas("Search Ads.");
  message_second.add_areas("Click Ads.");
  message_second.add_areas("Google Ads.");
  message_second.add_areas("Pop_up Ads."); // The different one.

  // Write the message to log message.
  ServiceClient::messageWriter(&message_first, &message_second, &log_message);

  // Set the field areas as set
  std::string field_1 = "areas";
  ServiceClient::treat_repeated_field_list_or_set(log_message, 1, field_1);

  // Implements the differential service.
  std::string res = serviceClient.DifferentialService(
      message_first, message_second, log_message );

  const char* c_res = res.c_str();

  // The first message needs adding multiple elements than the seconde message.
  const char* test_res = "added: areas[0]: \"Search Ads.\"\n"
      "added: areas[1]: \"Click Ads.\"\n"
      "added: areas[3]: \"Pop_up Ads.\"\n";

  EXPECT_STREQ(c_res, test_res);
}

// Test 25: Test the nested repeated field treat as SET.
TEST(Differential_unit, repeated_set_6){
  // Establish the gRPC channel with port number.
  std::string target_str;
  target_str = "0.0.0.0:50053";
  ServiceClient serviceClient(
      grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));

  employee message_first;
  employee message_second;
  log log_message;

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
  differential_client::dependent_info* dependentInfo_ptr_1 =
      message_first.mutable_dependents();
  dependentInfo_ptr_1->add_name("Jeremy");
  dependentInfo_ptr_1->add_name("Zhe");
  dependentInfo_ptr_1->add_name("Jin");
  dependentInfo_ptr_1->add_name("June");

  dependentInfo_ptr_1->add_age(29);
  dependentInfo_ptr_1->add_age(30);
  dependentInfo_ptr_1->add_age(32);
  dependentInfo_ptr_1->add_age(26);

  differential_client::dependent_info* dependentInfo_ptr_2 =
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
  ServiceClient::messageWriter(&message_first, &message_second, &log_message);

  // Set the field areas as set
  std::string field_1 = "areas";
  ServiceClient::treat_repeated_field_list_or_set(log_message, 1, field_1);

  std::string field_2 = "dependents.name";
  ServiceClient::treat_repeated_field_list_or_set(log_message, 1, field_2);

  std::string field_3 = "dependents.age";
  ServiceClient::treat_repeated_field_list_or_set(log_message, 1, field_3);

  // Implements the differential service.
  std::string res = serviceClient.DifferentialService(
      message_first, message_second, log_message );

  const char* c_res = res.c_str();

  // The first message needs adding multiple elements than the seconde message.
  const char* test_res = "SAME";

  EXPECT_STREQ(c_res, test_res);
}

// Test 26: Test a nested repeated field treat as SET. Add element/s
TEST(Differential_unit, repeated_set_7){
  // Establish the gRPC channel with port number.
  std::string target_str;
  target_str = "0.0.0.0:50053";
  ServiceClient serviceClient(
      grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));

  employee message_first;
  employee message_second;
  log log_message;

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
  differential_client::dependent_info* dependentInfo_ptr_1 =
      message_first.mutable_dependents();
  dependentInfo_ptr_1->add_name("Jeremy");
  dependentInfo_ptr_1->add_name("Zhe");


  dependentInfo_ptr_1->add_age(29);
  dependentInfo_ptr_1->add_age(30);
  dependentInfo_ptr_1->add_age(32);
//  dependentInfo_ptr_1->add_age(26);

  differential_client::dependent_info* dependentInfo_ptr_2 =
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
  ServiceClient::messageWriter(&message_first, &message_second, &log_message);

  // Set the field areas as set
  std::string field_1 = "areas";
  ServiceClient::treat_repeated_field_list_or_set(log_message, 1, field_1);

  std::string field_2 = "dependents.name";
  ServiceClient::treat_repeated_field_list_or_set(log_message, 1, field_2);

  std::string field_3 = "dependents.age";
  ServiceClient::treat_repeated_field_list_or_set(log_message, 1, field_3);

  // Implements the differential service.
  std::string res = serviceClient.DifferentialService(
      message_first, message_second, log_message );

  const char* c_res = res.c_str();

  // The first message needs adding multiple elements than the seconde message.
  const char* test_res = "added: dependents.name[2]: \"June\"\n"
      "added: dependents.name[3]: \"Jin\"\n"
      "added: dependents.age[3]: 26\n";

  EXPECT_STREQ(c_res, test_res);
}

// Test 27: Test a nested repeated field treat as SET. delete element/s
TEST(Differential_unit, repeated_set_8){
  // Establish the gRPC channel with port number.
  std::string target_str;
  target_str = "0.0.0.0:50053";
  ServiceClient serviceClient(
      grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));

  employee message_first;
  employee message_second;
  log log_message;

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
  differential_client::dependent_info* dependentInfo_ptr_1 =
      message_first.mutable_dependents();
  dependentInfo_ptr_1->add_name("Jeremy");
  dependentInfo_ptr_1->add_name("Zhe");
  dependentInfo_ptr_1->add_name("Jin");
  dependentInfo_ptr_1->add_name("June");

  dependentInfo_ptr_1->add_age(29);
  dependentInfo_ptr_1->add_age(30);
  dependentInfo_ptr_1->add_age(32);
  dependentInfo_ptr_1->add_age(26);

  differential_client::dependent_info* dependentInfo_ptr_2 =
      message_second.mutable_dependents();

  dependentInfo_ptr_2->add_name("Zhe");
  dependentInfo_ptr_2->add_name("Jeremy");
  dependentInfo_ptr_2->add_name("June");

  dependentInfo_ptr_2->add_age(29);
  dependentInfo_ptr_2->add_age(30);
  dependentInfo_ptr_2->add_age(26);

  // Write the message to log message.
  ServiceClient::messageWriter(&message_first, &message_second, &log_message);

  // Set the field areas as set
  std::string field_1 = "areas";
  ServiceClient::treat_repeated_field_list_or_set(log_message, 1, field_1);

  std::string field_2 = "dependents.name";
  ServiceClient::treat_repeated_field_list_or_set(log_message, 1, field_2);

  std::string field_3 = "dependents.age";
  ServiceClient::treat_repeated_field_list_or_set(log_message, 1, field_3);

  // Implements the differential service.
  std::string res = serviceClient.DifferentialService(
      message_first, message_second, log_message );

  const char* c_res = res.c_str();

  // The first message needs adding multiple elements than the seconde message.
  const char* test_res = "deleted: dependents.name[2]: \"Jin\"\n"
      "deleted: dependents.age[2]: 32\n";

  EXPECT_STREQ(c_res, test_res);
}

// Test 28: Test a nested repeated field treat as SET. delete element/s
TEST(Differential_unit, repeated_set_9){
  // Establish the gRPC channel with port number.
  std::string target_str;
  target_str = "0.0.0.0:50053";
  ServiceClient serviceClient(
      grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));

  employee message_first;
  employee message_second;
  log log_message;

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
  differential_client::dependent_info* dependentInfo_ptr_1 =
      message_first.mutable_dependents();
  dependentInfo_ptr_1->add_name("Jeremy");
  dependentInfo_ptr_1->add_name("Zhe");
  dependentInfo_ptr_1->add_name("Jin");
  dependentInfo_ptr_1->add_name("June");

  dependentInfo_ptr_1->add_age(29);
  dependentInfo_ptr_1->add_age(30);
  dependentInfo_ptr_1->add_age(32);
  dependentInfo_ptr_1->add_age(26);

  differential_client::dependent_info* dependentInfo_ptr_2 =
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
  ServiceClient::messageWriter(&message_first, &message_second, &log_message);

  // Set the field areas as set
  std::string field_1 = "areas";
  ServiceClient::treat_repeated_field_list_or_set(log_message, 1, field_1);

  std::string field_2 = "dependents.name";
  ServiceClient::treat_repeated_field_list_or_set(log_message, 1, field_2);

  std::string field_3 = "dependents.age";
  ServiceClient::treat_repeated_field_list_or_set(log_message, 1, field_3);

  // Implements the differential service.
  std::string res = serviceClient.DifferentialService(
      message_first, message_second, log_message );

  const char* c_res = res.c_str();

  // The first message needs adding multiple elements than the seconde message.
  const char* test_res = "deleted: dependents.name[2]: \"Jin\"\n"
                         "deleted: dependents.age[2]: 32\n";

  EXPECT_STREQ(c_res, test_res);
}

// Test 29: Test a nested repeated field treat as SET. delete and add element/s
TEST(Differential_unit, repeated_set_10){
  // Establish the gRPC channel with port number.
  std::string target_str;
  target_str = "0.0.0.0:50053";
  ServiceClient serviceClient(
      grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));

  employee message_first;
  employee message_second;
  log log_message;

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
  differential_client::dependent_info* dependentInfo_ptr_1 =
      message_first.mutable_dependents();
  dependentInfo_ptr_1->add_name("Jeremy");
  dependentInfo_ptr_1->add_name("Zhe");
  dependentInfo_ptr_1->add_name("Tom");
  dependentInfo_ptr_1->add_name("June");

  dependentInfo_ptr_1->add_age(29);
  dependentInfo_ptr_1->add_age(30);
  dependentInfo_ptr_1->add_age(31);
  dependentInfo_ptr_1->add_age(26);

  differential_client::dependent_info* dependentInfo_ptr_2 =
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
  ServiceClient::messageWriter(&message_first, &message_second, &log_message);

  // Set the field areas as set
  std::string field_1 = "areas";
  ServiceClient::treat_repeated_field_list_or_set(log_message, 1, field_1);

  std::string field_2 = "dependents.name";
  ServiceClient::treat_repeated_field_list_or_set(log_message, 1, field_2);

  std::string field_3 = "dependents.age";
  ServiceClient::treat_repeated_field_list_or_set(log_message, 1, field_3);

  // Implements the differential service.
  std::string res = serviceClient.DifferentialService(
      message_first, message_second, log_message );

  const char* c_res = res.c_str();

  // The first message needs adding multiple elements than the seconde message.
  const char* test_res = "added: areas[3]: \"Search Ads.\"\n"
      "deleted: areas[2]: \"Pop_up Ads.\"\n"
      "added: dependents.name[3]: \"Jin\"\n"
      "deleted: dependents.name[2]: \"Tom\"\n"
      "added: dependents.age[2]: 32\n"
      "deleted: dependents.age[2]: 31\n";

  EXPECT_STREQ(c_res, test_res);
}

// Test 30: Test a nested repeated field treat as SET. Add element/s
TEST(Differential_unit, repeated_list_and_set){
  // Establish the gRPC channel with port number.
  std::string target_str;
  target_str = "0.0.0.0:50053";
  ServiceClient serviceClient(
      grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));

  employee message_first;
  employee message_second;
  log log_message;

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
  differential_client::dependent_info* dependentInfo_ptr_1 =
      message_first.mutable_dependents();
  dependentInfo_ptr_1->add_name("Jeremy");
  dependentInfo_ptr_1->add_name("Zhe");
  dependentInfo_ptr_1->add_name("June");
  dependentInfo_ptr_1->add_name("Jin");

  dependentInfo_ptr_1->add_age(29);
  dependentInfo_ptr_1->add_age(30);
  dependentInfo_ptr_1->add_age(32);
  dependentInfo_ptr_1->add_age(26);

  differential_client::dependent_info* dependentInfo_ptr_2 =
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
  ServiceClient::messageWriter(&message_first, &message_second, &log_message);

  // Set the field areas as set
  std::string field_1 = "areas";
  ServiceClient::treat_repeated_field_list_or_set(log_message, 0, field_1);

  std::string field_2 = "dependents.name";
  ServiceClient::treat_repeated_field_list_or_set(log_message, 1, field_2);

  std::string field_3 = "dependents.age";
  ServiceClient::treat_repeated_field_list_or_set(log_message, 0, field_3);

  // Implements the differential service.
  std::string res = serviceClient.DifferentialService(
      message_first, message_second, log_message );

  const char* c_res = res.c_str();

  // The first message needs adding multiple elements than the seconde message.
  const char* test_res = "SAME";

  EXPECT_STREQ(c_res, test_res);
}

// Test 31: Test a sub filed treat as Map. Map is different
TEST(Differential_unit, map_1){
  // Establish the gRPC channel with port number.
  std::string target_str;
  target_str = "0.0.0.0:50053";
  ServiceClient serviceClient(
      grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));

  employee message_first;
  employee message_second;
  log log_message;

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
  ServiceClient::messageWriter(&message_first, &message_second, &log_message);

  // Set the map value for repeated filed compare.
  std::string map_field_name = "education";
  std::vector<std::string> sub_field_list;
  sub_field_list.push_back("name");
  ServiceClient::treat_repeated_field_map(log_message, map_field_name, sub_field_list);

  // Implements the differential service.
  std::string res = serviceClient.DifferentialService(
      message_first, message_second, log_message );

  const char* c_res = res.c_str();

  // Map value is different replace the Map-value.
  const char* test_res = "added: education[0]: { name: \"Wright State University\" degree: \"PhD\" major: \"Computer Science\" address: \"OH, US\" }\n"
      "deleted: education[0]: { name: \"University of Dayton\" degree: \"PhD\" major: \"Computer Science\" address: \"OH, US\" }\n";

  EXPECT_STREQ(c_res, test_res);
}

// Test 32: Test a sub filed treat as Map. Value is different
TEST(Differential_unit, map_2){
  // Establish the gRPC channel with port number.
  std::string target_str;
  target_str = "0.0.0.0:50053";
  ServiceClient serviceClient(
      grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));

  employee message_first;
  employee message_second;
  log log_message;

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
  ServiceClient::messageWriter(&message_first, &message_second, &log_message);

  // Set the map value for repeated filed compare.
  std::string map_field_name = "education";
  std::vector<std::string> sub_field_list;
  sub_field_list.push_back("name");
  ServiceClient::treat_repeated_field_map(log_message, map_field_name, sub_field_list);

  // Implements the differential service.
  std::string res = serviceClient.DifferentialService(
      message_first, message_second, log_message );

  const char* c_res = res.c_str();

  // The first message needs adding multiple elements than the seconde message.
  const char* test_res = "modified: education[0].degree: \"PhD\" -> \"Master of Science\"\n"
      "modified: education[0].major: \"Computer Science\" -> \"Computer Science and Engineering\"\n";

  EXPECT_STREQ(c_res, test_res);
}

// Test 33: Test a sub filed treat as Map. Map and Value are different at same time.
TEST(Differential_unit, map_3){
  // Establish the gRPC channel with port number.
  std::string target_str;
  target_str = "0.0.0.0:50053";
  ServiceClient serviceClient(
      grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));

  employee message_first;
  employee message_second;
  log log_message;

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
  ServiceClient::messageWriter(&message_first, &message_second, &log_message);

  // Set the map value for repeated filed compare.
  std::string map_field_name = "education";
  std::vector<std::string> sub_field_list;
  sub_field_list.push_back("name");
  ServiceClient::treat_repeated_field_map(log_message, map_field_name, sub_field_list);

  // Implements the differential service.
  std::string res = serviceClient.DifferentialService(
      message_first, message_second, log_message );

  const char* c_res = res.c_str();

  // map value different add and delete.
  const char* test_res = "added: education[0]: { name: \"University of Dayton\" degree: \"Master of Science\" major: \"Computer Science and Engineering\" address: \"OH, US\" }\n"
      "deleted: education[0]: { name: \"Wright State University\" degree: \"PhD\" major: \"Computer Science\" address: \"OH, US\" }\n";

  EXPECT_STREQ(c_res, test_res);
}

// Test 34: Test multiple sub fields as Map. Map is different.
TEST(Differential_unit, map_4){
  // Establish the gRPC channel with port number.
  std::string target_str;
  target_str = "0.0.0.0:50053";
  ServiceClient serviceClient(
      grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));

  employee message_first;
  employee message_second;
  log log_message;

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
  ServiceClient::messageWriter(&message_first, &message_second, &log_message);

  // Set the map value for repeated filed compare.
  std::string map_field_name = "education";
  std::vector<std::string> sub_field_list;
  sub_field_list.push_back("name");
  sub_field_list.push_back("degree");
  ServiceClient::treat_repeated_field_map(log_message, map_field_name, sub_field_list);

  // Implements the differential service.
  std::string res = serviceClient.DifferentialService(
      message_first, message_second, log_message );

  const char* c_res = res.c_str();

  // Map value different
  const char* test_res = "added: education[0]: { name: \"University of Dayton\" degree: \"PhD\" major: \"Computer Science\" address: \"OH, US\" }\n"
      "deleted: education[0]: { name: \"Wright State University\" degree: \"PhD\" major: \"Computer Science\" address: \"OH, US\" }\n";

  EXPECT_STREQ(c_res, test_res);
}

// Test 35: Test multiple sub fields as Map. Value is different.
TEST(Differential_unit, map_5){
  // Establish the gRPC channel with port number.
  std::string target_str;
  target_str = "0.0.0.0:50053";
  ServiceClient serviceClient(
      grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));

  employee message_first;
  employee message_second;
  log log_message;

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
  ServiceClient::messageWriter(&message_first, &message_second, &log_message);

  // Set the map value for repeated filed compare.
  std::string map_field_name = "education";
  std::vector<std::string> sub_field_list;
  sub_field_list.push_back("name");
  sub_field_list.push_back("degree");
  ServiceClient::treat_repeated_field_map(log_message, map_field_name, sub_field_list);

  // Implements the differential service.
  std::string res = serviceClient.DifferentialService(
      message_first, message_second, log_message );

  const char* c_res = res.c_str();

  // The first message needs adding multiple elements than the seconde message.
  const char* test_res = "modified: education[0].major: \"Computer Science\" -> \"Computer Science and Engineering\"\n";

  EXPECT_STREQ(c_res, test_res);
}

// Test 36: Test multiple sub fields as Map. Map and Value are different.
TEST(Differential_unit, map_6){
  // Establish the gRPC channel with port number.
  std::string target_str;
  target_str = "0.0.0.0:50053";
  ServiceClient serviceClient(
      grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));

  employee message_first;
  employee message_second;
  log log_message;

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
  ServiceClient::messageWriter(&message_first, &message_second, &log_message);

  // Set the map value for repeated filed compare.
  std::string map_field_name = "education";
  std::vector<std::string> sub_field_list;
  sub_field_list.push_back("name");
  sub_field_list.push_back("degree");
  ServiceClient::treat_repeated_field_map(log_message, map_field_name, sub_field_list);

  // Implements the differential service.
  std::string res = serviceClient.DifferentialService(
      message_first, message_second, log_message );

  const char* c_res = res.c_str();

  // The first message needs adding multiple elements than the seconde message.
  const char* test_res = "added: education[0]: { name: \"University of Dayton\" degree: \"PhD\" major: \"Computer Science and Engineering\" address: \"OH, US\" }\n"
      "deleted: education[0]: { name: \"Wright State University\" degree: \"PhD\" major: \"Computer Science\" address: \"OH, US\" }\n";

  EXPECT_STREQ(c_res, test_res);
}

// Test 37: Test multiple sub fields as Map. message 1 is empty
TEST(Differential_unit, map_7){
  // Establish the gRPC channel with port number.
  std::string target_str;
  target_str = "0.0.0.0:50053";
  ServiceClient serviceClient(
      grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));

  employee message_first;
  employee message_second;
  log log_message;

  education_info* edu_info_1 = message_first.add_education();

  education_info* edu_info_2 = message_second.add_education();
  edu_info_2->set_name("University of Dayton");
  edu_info_2->set_major("Computer Science and Engineering");
  edu_info_2->set_address("OH, US");
  edu_info_2->set_degree("PhD");


  // Write the message to log message.
  ServiceClient::messageWriter(&message_first, &message_second, &log_message);

  // Set the map value for repeated filed compare.
  std::string map_field_name = "education";
  std::vector<std::string> sub_field_list;
  sub_field_list.push_back("name");
  sub_field_list.push_back("degree");
  ServiceClient::treat_repeated_field_map(log_message, map_field_name, sub_field_list);

  // Implements the differential service.
  std::string res = serviceClient.DifferentialService(
      message_first, message_second, log_message );

  const char* c_res = res.c_str();

  // The first message needs adding multiple elements than the seconde message.
  const char* test_res = "added: education[0]: { name: \"University of Dayton\" "
      "degree: \"PhD\" "
      "major: \"Computer Science and Engineering\" "
      "address: \"OH, US\" }\ndeleted: education[0]: { }\n";

  EXPECT_STREQ(c_res, test_res);
}

// Test 38: Test multiple sub fields as Map. message 2 is empty
TEST(Differential_unit, map_8){
  // Establish the gRPC channel with port number.
  std::string target_str;
  target_str = "0.0.0.0:50053";
  ServiceClient serviceClient(
      grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));

  employee message_first;
  employee message_second;
  log log_message;

  education_info* edu_info_1 = message_first.add_education();
  edu_info_1->set_name("Wright State University");
  edu_info_1->set_degree("PhD");
  edu_info_1->set_major("Computer Science");
  edu_info_1->set_address("OH, US");

  education_info* edu_info_2 = message_second.add_education();

  // Write the message to log message.
  ServiceClient::messageWriter(&message_first, &message_second, &log_message);

  // Set the map value for repeated filed compare.
  std::string map_field_name = "education";
  std::vector<std::string> sub_field_list;
  sub_field_list.push_back("name");
  sub_field_list.push_back("degree");
  ServiceClient::treat_repeated_field_map(log_message, map_field_name, sub_field_list);


  // Implements the differential service.
  std::string res = serviceClient.DifferentialService(
      message_first, message_second, log_message );

  const char* c_res = res.c_str();

  // The first message needs adding multiple elements than the second message.
  const char* test_res = "added: education[0]: { }\n"
      "deleted: education[0]: { name: \"Wright State University\" degree: \"PhD\" major: \"Computer Science\" address: \"OH, US\" }\n";

  EXPECT_STREQ(c_res, test_res);
}

// Test 39 - 41: Test the fraction and margin for float number comparison
TEST(Differential_unit, float_and_double_1){
  // Establish the gRPC channel with port number.
  std::string target_str;
  target_str = "0.0.0.0:50053";
  ServiceClient serviceClient(
      grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));

  employee message_first;
  employee message_second;
  log log_message;

  message_first.set_floatpoint(100.0f);
  message_second.set_floatpoint(109.9f);

  // Write the message to log message.
  ServiceClient::messageWriter(&message_first, &message_second, &log_message);

  //1)
  // Should fail since the fraction is smaller than error.
  ServiceClient::setFractionAndMargin(log_message, 0.01, 0.0);

  // Implements the differential service.
  std::string res = serviceClient.DifferentialService(
      message_first, message_second, log_message );
  const char* c_res = res.c_str();

  // should fail.
  const char* test_res = "modified: floatpoint: 100 -> 109.90000152587891\n";
  EXPECT_STREQ(c_res, test_res);

  // 2)
  // Test out float comparison with fraction.
  ServiceClient::setFractionAndMargin(log_message, 0.2, 0.0);

  // Implements the differential service.
  std::string res_1 = serviceClient.DifferentialService(
      message_first, message_second, log_message );
  const char* c_res_1 = res_1.c_str();

  // should same.
  const char* test_res_1 = "SAME";
  EXPECT_STREQ(c_res_1, test_res_1);

  // 3)
  // Test out float comparison with fraction.
  ServiceClient::setFractionAndMargin(log_message, 0.01, 10.0);

  // Implements the differential service.
  std::string res_2 = serviceClient.DifferentialService(
      message_first, message_second, log_message );
  const char* c_res_2 = res_1.c_str();

  // should same.
  const char* test_res_2 = "SAME";
  EXPECT_STREQ(c_res_2, test_res_2);

}

// Test 42 - 44: Test the fraction and margin for double number comparison
TEST(Differential_unit, float_and_double_2){
  // Establish the gRPC channel with port number.
  std::string target_str;
  target_str = "0.0.0.0:50053";
  ServiceClient serviceClient(
      grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));

  employee message_first;
  employee message_second;
  log log_message;

  message_first.set_floatpoint(100.0);
  message_second.set_floatpoint(109.9);

  // Write the message to log message.
  ServiceClient::messageWriter(&message_first, &message_second, &log_message);

  //1)
  // Should fail since the fraction is smaller than error.
  ServiceClient::setFractionAndMargin(log_message, 0.01, 0.0);

  // Implements the differential service.
  std::string res = serviceClient.DifferentialService(
      message_first, message_second, log_message );
  const char* c_res = res.c_str();

  // should fail.
  const char* test_res = "modified: floatpoint: 100 -> 109.9\n";
  EXPECT_STREQ(c_res, test_res);

  // 2)
  // Test out float comparison with fraction.
  ServiceClient::setFractionAndMargin(log_message, 0.2, 0.0);

  // Implements the differential service.
  std::string res_1 = serviceClient.DifferentialService(
      message_first, message_second, log_message );
  const char* c_res_1 = res_1.c_str();

  // should same.
  const char* test_res_1 = "SAME";
  EXPECT_STREQ(c_res_1, test_res_1);

  // 3)
  // Test out float comparison with fraction.
  ServiceClient::setFractionAndMargin(log_message, 0.01, 10.0);

  // Implements the differential service.
  std::string res_2 = serviceClient.DifferentialService(
      message_first, message_second, log_message );
  const char* c_res_2 = res_1.c_str();

  // should same.
  const char* test_res_2 = "SAME";
  EXPECT_STREQ(c_res_2, test_res_2);

}

// Test 45: Test the ignore field isolated from repeated field set.
TEST(Differential_unit, ignore_isolate_1){
  // Establish the gRPC channel with port number.
  std::string target_str;
  target_str = "0.0.0.0:50053";
  ServiceClient serviceClient(
      grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));

  employee message_first;
  employee message_second;
  log log_message;

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
  ServiceClient::messageWriter(&message_first, &message_second, &log_message);

  // ignore the areas field
  std::vector<std::string> fields;
  std::string field_1("differential_client.employee.areas");
  fields.push_back(field_1);
  ServiceClient::blackListCriteria(log_message, fields);

  // Set the field areas as list
  std::string repeated_field = "areas";
  ServiceClient::treat_repeated_field_list_or_set(log_message, 0, repeated_field);

  // Implements the differential service.
  std::string res = serviceClient.DifferentialService(
      message_first, message_second, log_message );

  const char* c_res = res.c_str();

  // Because we ignore the area field so differential only at fullname will presented.
  const char* test_res = "modified: fullname: \"Jin Huang\" -> \"Zhe Liu\"\n";

  EXPECT_STREQ(c_res, test_res);
}

// Test 46: Test the ignore field isolated from repeated field set.
TEST(Differential_unit, ignore_isolate_2){
  // Establish the gRPC channel with port number.
  std::string target_str;
  target_str = "0.0.0.0:50053";
  ServiceClient serviceClient(
      grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));

  employee message_first;
  employee message_second;
  log log_message;

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
  ServiceClient::messageWriter(&message_first, &message_second, &log_message);

  // ignore the areas field
  std::vector<std::string> fields;
  std::string field_1("differential_client.employee.education");
  std::string field_2("differential_client.employee.areas");
  fields.push_back(field_1);
  fields.push_back(field_2);
  ServiceClient::blackListCriteria(log_message, fields);

  // Set the field areas as list
  std::string repeated_field = "areas";
  ServiceClient::treat_repeated_field_list_or_set(log_message, 0, repeated_field);

  // Set the map value for repeated filed compare.
  std::string map_field_name = "education";
  std::vector<std::string> sub_field_list;
  sub_field_list.push_back("name");
  ServiceClient::treat_repeated_field_map(log_message, map_field_name, sub_field_list);


  // Implements the differential service.
  std::string res = serviceClient.DifferentialService(
      message_first, message_second, log_message );

  const char* c_res = res.c_str();

  // Because we ignore the area field so differential only at fullname will presented.
  const char* test_res = "modified: fullname: \"Jin Huang\" -> \"Zhe Liu\"\n";

  EXPECT_STREQ(c_res, test_res);
}

// Test 47: Test the differential service comprehensively
TEST(Differential_unit, total_test_1){
  // Establish the gRPC channel with port number.
  std::string target_str;
  target_str = "0.0.0.0:50053";
  ServiceClient serviceClient(
      grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));

  employee message_first;
  employee message_second;
  log log_message;

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
  differential_client::dependent_info* dependentInfo_ptr_1 =
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
  differential_client::dependent_info* dependentInfo_ptr_2 =
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
  ServiceClient::messageWriter(&message_first, &message_second, &log_message);

  // add ignore fields
  std::vector<std::string> ignore_fields;
  std::string field_1("differential_client.employee.employ_id");
  std::string field_2("differential_client.company.address");
  std::string field_3("differential_client.education_info.address");

  ignore_fields.push_back(field_1);
  ignore_fields.push_back(field_2);
  ignore_fields.push_back(field_3);
  ServiceClient::blackListCriteria(log_message, ignore_fields);

  // Set the field areas as list
  std::string repeated_field_1("areas");
  ServiceClient::treat_repeated_field_list_or_set(log_message, 0, repeated_field_1);

  // Set the field dependents.name as set
  std::string repeated_field_2("dependents.name");
  ServiceClient::treat_repeated_field_list_or_set(log_message, 1, repeated_field_2);

  // Set the field dependents.age as set
  std::string repeated_field_3("dependents.age");
  ServiceClient::treat_repeated_field_list_or_set(log_message, 1, repeated_field_3);

  // Set the map value comparison
  std::string map_field_name("education");
  std::vector<std::string> sub_field_list;
  sub_field_list.push_back("name");
  sub_field_list.push_back("degree");
  ServiceClient::treat_repeated_field_map(log_message, map_field_name, sub_field_list);

  // Implements the differential service.
  std::string res = serviceClient.DifferentialService(
      message_first, message_second, log_message );

  const char* c_res = res.c_str();

  const char* test_res = "modified: fullname: \"Jin Huang\" -> \"Zhe Liu\"\n"
      "modified: employer.occupation: \"Softwear Engineer\" -> \"Softweare Engineer\"\n";

  EXPECT_STREQ(c_res, test_res);
}


// TEST end line.
}


int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

