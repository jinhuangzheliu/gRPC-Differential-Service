#include <iostream>
#include <memory>
#include <string>
#include <fstream>
#include <iostream>

#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/compiler/importer.h>
#include <google/protobuf/message.h>
#include <grpcpp/grpcpp.h>

#include "message_write.grpc.pb.h"
#include "message_service.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using google::protobuf::Message;
using google::protobuf::DescriptorPool;
using google::protobuf::MessageFactory;
using google::protobuf::FieldDescriptor;
using google::protobuf::FieldDescriptorProto;
using google::protobuf::Descriptor;
using google::protobuf::DescriptorProto;
using google::protobuf::FileDescriptor;
using google::protobuf::FileDescriptorProto;
using google::protobuf::Map;

using messageservice::MessageServer;
using messageservice::MsgRequest;
using messageservice::MsgReply;

using messagewrite::people;
using messagewrite::company;
using messagewrite::university;

void blackListCriteria(messageservice::log* log_message){
  log_message->add_ignore_field_list("BLACK");
  log_message->add_ignore_field_list("messagewrite.people.fullname");
  log_message->add_ignore_field_list("messagewrite.people.age");
  log_message->add_ignore_field_list("messagewrite.company.address");
  log_message->add_ignore_field_list("messagewrite.university.address");

}

void whiteListCriteria(messageservice::log* log_message){
  log_message->add_ignore_field_list("WHITE");
  log_message->add_ignore_field_list("messagewrite.people.fullname");
  log_message->add_ignore_field_list("messagewrite.people.age");
  log_message->add_ignore_field_list("messagewrite.people.employer");
  log_message->add_ignore_field_list("messagewrite.company.address");
  log_message->add_ignore_field_list("messagewrite.people.education");
  log_message->add_ignore_field_list("messagewrite.university.address");
}

void RegexCriteria(messageservice::log* log_message){
  log_message->set_regex("^mes.*ress$");
}

void messageWriter(people* msg_1, people* msg_2,
                    messageservice::log* log_msg)
{
  // two message serialized values.
  std::string firstMessageStr;
  std::string secondMessageStr;

  // 1) set the first message
  msg_1->set_employ_id(0001);
  msg_1->set_fullname("Jeremy Lee");
  msg_1->set_age(29);

  // set the employer info.
  msg_1->mutable_employer()->set_name("Google Inc.");
  msg_1->mutable_employer()->set_occupation("Software Engineer");
  msg_1->mutable_employer()->set_address("NY, US");

  // set the education info.
  msg_1->mutable_education()->set_name("Florida State Univeristy");
  msg_1->mutable_education()->set_degree("Master of Science");
  msg_1->mutable_education()->set_major("computer Science");
  msg_1->mutable_education()->set_address("FL, US");

  // set the work infomation
  Map<std::string, std::string>* map_ptr_frt = msg_1->mutable_info();
  (*map_ptr_frt)["Tenure"] = "1 month";
  (*map_ptr_frt)["Job"] = "Intern-Technical";
  (*map_ptr_frt)["Area"] = "EngPM Interns";

  // Serialized to string
  msg_1->SerializeToString(&firstMessageStr);

  // set the messages.
  log_msg->set_base(firstMessageStr);

  // 2) set the second message
  msg_2->set_employ_id(0002);
  msg_2->set_fullname("Jin Huang");
  msg_2->set_age(30);

  msg_2->mutable_employer()->set_name("Google Inc.");
  msg_2->mutable_employer()->set_occupation("Sofeware Engineer Intern");
  msg_2->mutable_employer()->set_address("CA, US");

  msg_2->mutable_education()->set_name("Wright State University");
  msg_2->mutable_education()->set_degree("PhD");
  msg_2->mutable_education()->set_major("Computer Science and Engineering");
  msg_2->mutable_education()->set_address("OH, US");

  Map<std::string, std::string>* map_ptr_scd = msg_2->mutable_info();
  (*map_ptr_scd)["Tenure"] = "9 years";
  (*map_ptr_scd)["Type"] = "Employee";
  (*map_ptr_scd)["Job"] = "Software Engineer";
  (*map_ptr_scd)["Area"] = "EngProd - Search Ads";

  msg_2->SerializeToString(&secondMessageStr);

  log_msg->set_test(secondMessageStr);

  // 3) Get the message Descriptor.
  const Descriptor *dsp = msg_1->GetDescriptor();
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
  for (int i = 0; i < fdsp->dependency_count(); i++){
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

class MessageWrite {
 public:
  explicit MessageWrite(const std::shared_ptr<Channel>& channel) : stub_(MessageServer::NewStub(channel)) {}

  std::string GetContect(const std::string& msg) {
    // message sending to the server.
    MsgRequest request;
    request.set_request(msg);

    // Container for the data we expect from the server.
    MsgReply reply;

    // Context for the client.
    ClientContext context;

    // The actual RPC.
    Status status = stub_->GetContect(&context, request, &reply);

    // Act upon its status.
    if (status.ok()) {
      return reply.reply();
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return "RPC failed";
    }
  }

  std::string DifferTest(){
    /****************************************************************
     *  Create the differential messages
     ****************************************************************/

    // two message serialized values.
    std::string firstMessageStr;
    std::string secondMessageStr;

    // 1) Create first message
    people message_first;
    message_first.set_employ_id(0001);
    message_first.set_fullname("Jeremy Lee");
    message_first.set_age(29);

    // set the employer info.
    message_first.mutable_employer()->set_name("Google Inc.");
    message_first.mutable_employer()->set_occupation("Software Engineer");
    message_first.mutable_employer()->set_address("NY, US");

    // set the education info.
    message_first.mutable_education()->set_name("Florida State Univeristy");
    message_first.mutable_education()->set_degree("Master of Science");
    message_first.mutable_education()->set_major("computer Science");
    message_first.mutable_education()->set_address("FL, US");

    // set the work infomation
    Map<std::string, std::string>* map_ptr_frt = message_first.mutable_info();
    (*map_ptr_frt)["Tenure"] = "1 month";
    (*map_ptr_frt)["Job"] = "Intern-Technical";
    (*map_ptr_frt)["Area"] = "EngPM Interns";

    // Serialized to string
    message_first.SerializeToString(&firstMessageStr);



    // 2) create the second message
    people message_second;
    message_second.set_employ_id(0002);
    message_second.set_fullname("Jin Huang");
    message_second.set_age(30);

    message_second.mutable_employer()->set_name("Google Inc.");
    message_second.mutable_employer()->set_occupation("Sofeware Engineer Intern");
    message_second.mutable_employer()->set_address("CA, US");

    message_second.mutable_education()->set_name("Wright State University");
    message_second.mutable_education()->set_degree("PhD");
    message_second.mutable_education()->set_major("Computer Science and Engineering");
    message_second.mutable_education()->set_address("OH, US");

    Map<std::string, std::string>* map_ptr_scd = message_second.mutable_info();
    (*map_ptr_scd)["Tenure"] = "9 years";
    (*map_ptr_scd)["Type"] = "Employee";
    (*map_ptr_scd)["Job"] = "Software Engineer";
    (*map_ptr_scd)["Area"] = "EngProd - Search Ads";

    message_second.SerializeToString(&secondMessageStr);



    // 3) Get the message Descriptor.
    const Descriptor *dsp = message_first.GetDescriptor();


    // 4) Get the file descriptor proto.
    const FileDescriptor* fdsp = dsp->file();
    FileDescriptorProto fdspProto;
    fdsp->CopyTo(&fdspProto);

    // Serialized to string
    std::string fdspProtoStr;
    fdspProto.SerializeToString(&fdspProtoStr);



    // 5) Create a log message for communicate with the server.
    messageservice::log log_message;

    // set the messages.
    log_message.set_base(firstMessageStr);
    log_message.set_test(secondMessageStr);

    // set the name of descriptor
    log_message.set_name_of_message_descriptor(dsp->name());

    // set the file descriptor proto and its dependency
    log_message.add_file_descriptor_proto(fdspProtoStr);

    // Add the dependency of file descriptor
    for (int i = 0; i < fdsp->dependency_count(); i++){
      FileDescriptorProto fdspProto_dep;
      std::string fdspProtoStr_dep;

      // Get the file descriptor of dependency
      const FileDescriptor* fdsp_dep = fdsp->dependency(i);

      // Get the file descriptor proto of dependency
      fdsp_dep->CopyTo(&fdspProto_dep);
      fdspProto_dep.SerializeToString(&fdspProtoStr_dep);

      // add dependency to log
      log_message.add_file_descriptor_proto(fdspProtoStr_dep);
    }



    // 6) Set White or black list Criteria with the field name
//    blackListCriteria(&log_message);
//    whiteListCriteria(&log_message);
//    RegexCriteria(&log_message);



    /*******************************************************************
     *
     ******************************************************************/

    messageservice::result resultOfDifferTest;

    ClientContext context;

    Status status = stub_->DifferTest(&context, log_message, &resultOfDifferTest);

    if (status.ok()){
      return resultOfDifferTest.res();
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return "Differ test failed";
    }
  }

  std::string MapKeyCompare(people& message_1, people& message_2,
                     messageservice::log& log_message){

    // Set the name of the field descriptor
    log_message.add_field_names("info");

    messageservice::result resultOfDifferTest;

    ClientContext context;

    Status status = stub_->MapKeyCompare(&context, log_message, &resultOfDifferTest);

    if (status.ok()){
      return resultOfDifferTest.res();
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return "MapKeyCompare failed";
    }
  }

 private:
  std::unique_ptr<MessageServer::Stub> stub_;
};

int main(int argc, char* argv[]){
  GOOGLE_PROTOBUF_VERIFY_VERSION;


  // Establish the gRPC channel
  std::string target_str;
  target_str = "0.0.0.0:50053";

  MessageWrite writer(grpc::CreateChannel(target_str,
                                          grpc::InsecureChannelCredentials()));

  std::string user(" Established");
  std::string reply = writer.GetContect(user);
  std::cout << "Status received: " << reply << std::endl;

  people message_first;
  people message_second;
  messageservice::log log_message;

  // Write the message and log
  messageWriter(&message_first, &message_second, &log_message);

  std::string MapKeyRes = writer.MapKeyCompare(message_first, message_second, log_message);
  std::cout << "MapKeyCompare result: \n"
            << MapKeyRes << std::endl;

//  std::string differTestRes = writer.DiffertTest();
//  std::cout << "Message differencer result: \n"
//            << differTestRes << std::endl;


  google::protobuf::ShutdownProtobufLibrary();
  return 0;
}