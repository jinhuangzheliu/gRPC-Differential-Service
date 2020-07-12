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

#include "differential_service.grpc.pb.h"
#include "differential_client.grpc.pb.h"
//#include "message_service.grpc.pb.h"
//#include "message_write.grpc.pb.h"

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


void blackListCriteria(differentialservice::log* log_message) {
  differentialservice::ignoreCriteria* ignoreCriteria_ptr =
      log_message->mutable_user_ignore();
  ignoreCriteria_ptr->set_flag(
      differentialservice::ignoreCriteria_ignoreFlag_FLAG_IGNORE);

  ignoreCriteria_ptr->add_ignore_fields_list("differential_client.employee.fullname");
//  ignoreCriteria_ptr->add_ignore_fields_list("differential_client.dependent_info.age");
//  ignoreCriteria_ptr->add_ignore_fields_list("differential_client.company.address");
//  ignoreCriteria_ptr->add_ignore_fields_list("differential_client.education_info.address");
}

void whiteListCriteria(differentialservice::log* log_message) {
  differentialservice::ignoreCriteria* ignoreCriteria_ptr =
      log_message->mutable_user_ignore();
  ignoreCriteria_ptr->set_flag(
      differentialservice::ignoreCriteria_ignoreFlag_FLAG_COMPARE);

  ignoreCriteria_ptr->add_ignore_fields_list("differential_client.employee.areas");
  ignoreCriteria_ptr->add_ignore_fields_list("differential_client.employee.dependents");
  ignoreCriteria_ptr->add_ignore_fields_list("differential_client.dependent_info.name");
  ignoreCriteria_ptr->add_ignore_fields_list("differential_client.dependent_info.age");
  ignoreCriteria_ptr->add_ignore_fields_list("differential_client.employee.education");
  ignoreCriteria_ptr->add_ignore_fields_list("differential_client.university.name");
  ignoreCriteria_ptr->add_ignore_fields_list("differential_client.university.degree");
  ignoreCriteria_ptr->add_ignore_fields_list("differential_client.university.major");
  ignoreCriteria_ptr->add_ignore_fields_list("differential_client.university.address");
}

void RegexCriteria(differentialservice::log* log_message) {
  log_message->mutable_user_ignore()->set_regex("^mes.*ress$");
}

void messageWriter(employee* msg_1, employee* msg_2, differentialservice::log* log_msg) {
  // two message serialized values.
  std::string firstMessageStr;
  std::string secondMessageStr;

  /**************************************************
   *
   *  Write the first message
   *
   ***************************************************/
  msg_1->set_employ_id(0001);
  msg_1->set_fullname("Alex Lee");
  msg_1->set_age(30);

  // set the employer info.
  msg_1->mutable_employer()->set_name("Google Inc.");
  msg_1->mutable_employer()->set_occupation("Software Engineer");
  msg_1->mutable_employer()->set_address("CA, US");

  // set the working areas
  msg_1->add_areas("Google Ads.");
  msg_1->add_areas("YouTube Ads.");
  msg_1->add_areas("Search Ads.");
  msg_1->add_areas("Click Ads.");

  // set the education info No. 1
  education_info* edu_info_1 = msg_1->add_education();
  edu_info_1->set_name("Wright State University");
  edu_info_1->set_degree("PhD");
  edu_info_1->set_major("Computer Science");
  edu_info_1->set_address("OH, US");
  // No. 2
//  education_info* edu_info_2 = msg_1->add_education();
//  edu_info_2->set_name("University of Dayton");
//  edu_info_2->set_degree("Master of Science");
//  edu_info_2->set_major("computer Science");
//  edu_info_2->set_address("OH, US");

  // set the dependent info
  differential_client::dependent_info* dependentInfo_ptr_1 =
      msg_1->mutable_dependents();
  dependentInfo_ptr_1->add_name("Jeremy");
  dependentInfo_ptr_1->add_name("Zhe");
  dependentInfo_ptr_1->add_name("Jin");
  dependentInfo_ptr_1->add_name("June");

  dependentInfo_ptr_1->add_age(29);
  dependentInfo_ptr_1->add_age(30);
  dependentInfo_ptr_1->add_age(32);
  dependentInfo_ptr_1->add_age(26);

  double num1 = 100.0;
  msg_1->set_floatpoint(num1);

  differential_client::field_set* fieldSet_ptr_1 = msg_1->add_foo();
  fieldSet_ptr_1->set_exam1("midterm");
  fieldSet_ptr_1->set_score1(93);
  fieldSet_ptr_1->set_exam2("final");
  fieldSet_ptr_1->set_score2(87.5);

  // set the work infomation
  //  Map<std::string, std::string>* map_ptr_frt = msg_1->mutable_info();
  //  (*map_ptr_frt)["Tenure"] = "1 month";
  //  (*map_ptr_frt)["Job"] = "Intern-Technical";
  //  (*map_ptr_frt)["Area"] = "EngPM Interns";

  // Serialized to string
  msg_1->SerializeToString(&firstMessageStr);
  // set the messages.
  log_msg->set_message_1(firstMessageStr);


  /***************************************************
   *
   *  Write the second message
   *
   ***************************************************/
  // 2) set the second message
  msg_2->set_employ_id(0002);
  msg_2->set_fullname("Jin Huang");
  msg_2->set_age(30);

  msg_2->mutable_employer()->set_name("Google Inc.");
  msg_2->mutable_employer()->set_occupation("Sofeware Engineer Intern");
  msg_2->mutable_employer()->set_address("CA, US");

  msg_2->add_areas("Google Ads.");
  msg_2->add_areas("YouTube Ads.");
  msg_2->add_areas("Click Ads.");
  msg_2->add_areas("Search Ads.");

  // First eduction for message 2
  education_info* edu_info_3 = msg_2->add_education();
  edu_info_3->set_name("Wright State University");
  edu_info_3->set_degree("PhD");
  edu_info_3->set_major("Computer Science and Engineering");
  edu_info_3->set_address("OH, US");

  differential_client::dependent_info* dependentInfo_ptr_2 =
      msg_2->mutable_dependents();

  dependentInfo_ptr_2->add_name("Zhe");
  dependentInfo_ptr_2->add_name("Jeremy");
  dependentInfo_ptr_2->add_name("June");
  dependentInfo_ptr_2->add_name("Jin");

  dependentInfo_ptr_2->add_age(32);
  dependentInfo_ptr_2->add_age(26);
  dependentInfo_ptr_2->add_age(29);
  dependentInfo_ptr_2->add_age(30);

  double num2 = 109.9;
  msg_2->set_floatpoint(num2);

  differential_client::field_set* fieldSet_ptr_2 = msg_2->add_foo();
  fieldSet_ptr_2->set_exam1("final");
  fieldSet_ptr_2->set_score1(93);
  fieldSet_ptr_2->set_exam2("midterm");
  fieldSet_ptr_2->set_score2(87.5);

  //  (*map_ptr_scd)["Tenure"] = "9 years";
  //  (*map_ptr_scd)["Type"] = "Employee";
  //  (*map_ptr_scd)["Job"] = "Software Engineer";
  //  (*map_ptr_scd)["Area"] = "EngProd - Search Ads";

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

class ServiceClient {
 public:
  // Constructor for Service client derived from the stub of DifferentialServer
  explicit ServiceClient(const std::shared_ptr<Channel>& channel)
      : stub_(DifferentialServer::NewStub(channel)) {}

  std::string GetConnect(const std::string& msg) {
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

  std::string DefaultDifferentialService( employee& message_1, employee& message_2,
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

  std::string DifferentialService( employee& message_1, employee& message_2,
                                   differentialservice::log& log_message)
  {
    // Set the ignore Criteria
//    whiteListCriteria(&log_message);
    blackListCriteria(&log_message);

    // set the repeated field comparison
    differentialservice::repeatedFieldTuple* tuple_ptr_1 =
        log_message.add_repeated_field();
    tuple_ptr_1->set_flag(
        differentialservice::repeatedFieldTuple_treatAsFlag_FLAG_SET);
    tuple_ptr_1->set_field_name("areas");

    differentialservice::repeatedFieldTuple* tuple_ptr_2 =
        log_message.add_repeated_field();
    tuple_ptr_2->set_flag(
        differentialservice::repeatedFieldTuple_treatAsFlag_FLAG_SET);
    tuple_ptr_2->set_field_name("dependents.name");

    differentialservice::repeatedFieldTuple* tuple_ptr_3 =
        log_message.add_repeated_field();
    tuple_ptr_3->set_flag(
        differentialservice::repeatedFieldTuple_treatAsFlag_FLAG_LIST);
    tuple_ptr_3->set_field_name("dependents.age");

    // Set the map value for repeated filed compare.
    differentialservice::mapvalueTuple* mapvalue_ptr =
        log_message.add_mapvaluecompare();
    mapvalue_ptr->set_name_of_repeated_field("education");
    mapvalue_ptr->add_name_of_sub_field("name");
    mapvalue_ptr->add_name_of_sub_field("degree");


    // Set the fraction and margin for float number comparison
    differentialservice::float_comparison* fracAndMar_ptr =
        log_message.mutable_fraction_margin();
    fracAndMar_ptr->set_fraction(0.01);
    fracAndMar_ptr->set_margin(0.0);

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

 private:
  std::unique_ptr<DifferentialServer::Stub> stub_;
};

int main(int argc, char* argv[]) {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  // Establish the gRPC channel with port number.
  std::string target_str;
  target_str = "0.0.0.0:50053";

  // Initial the service client instance.
  ServiceClient writer(
      grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials())
      );

  /*
   *  Check the Service Connection
   */
  std::string user(" Established");
  std::string reply = writer.GetConnect(user);
  std::cout << "Status received: " << reply << std::endl;

  /*
   *  Implement the Differential Service
   */
  employee message_first;
  employee message_second;
  differentialservice::log log_message;

  // Write two input messages into service log.
  messageWriter(&message_first, &message_second, &log_message);

  // compare two messages by default.
  std::string repeatedRes = writer.DefaultDifferentialService(
      message_first, message_second, log_message);
  std::cout << "Message differencer result (Default): \n" << repeatedRes << std::endl;

  // compare two messages by default.
  std::string repeatedRes1 = writer.DifferentialService(
      message_first, message_second, log_message);
  std::cout << "Message differencer result: \n" << repeatedRes1 << std::endl;

  google::protobuf::ShutdownProtobufLibrary();
  return 0;
}