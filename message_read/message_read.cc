#include <google/protobuf/compiler/importer.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/message.h>
#include <google/protobuf/util/message_differencer.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <regex>

#include "message_read.grpc.pb.h"
#include "message_service.grpc.pb.h"

using google::protobuf::Descriptor;
using google::protobuf::DescriptorPool;
using google::protobuf::DescriptorProto;
using google::protobuf::FileDescriptor;
using google::protobuf::FileDescriptorProto;
using google::protobuf::FieldDescriptor;

using google::protobuf::Message;
using google::protobuf::MessageFactory;
using google::protobuf::Reflection;
using google::protobuf::util::MessageDifferencer;
using google::protobuf::util::DefaultFieldComparator;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using messageservice::log;
using messageservice::MessageServer;
using messageservice::MsgReply;
using messageservice::MsgRequest;
using messageservice::result;


class blackListIgnoreCriteria : public MessageDifferencer::IgnoreCriteria {
 public:
  blackListIgnoreCriteria(){
    set_ptr = new std::unordered_set<std::string>;
  }

  void add_featureField(const std::string& field){
    set_ptr->insert(field);
  }

  bool IsIgnored(const Message& msg1,
                 const Message& msg2,
                 const FieldDescriptor* field,
                 const std::vector<MessageDifferencer::SpecificField>& parent_fields) override
  {
    std::cout << field->full_name() << std::endl;
    if(set_ptr->find(field->full_name()) == set_ptr->end()){
      return false;
    } else {
      return true;
    }
  }

 private:
  std::unordered_set<std::string>* set_ptr;
};

class whiteListIgnoreCriteria : public MessageDifferencer::IgnoreCriteria {
 public:
  whiteListIgnoreCriteria(){
    set_ptr = new std::unordered_set<std::string>;
  }

  void add_featureField(const std::string& field){
    set_ptr->insert(field);
  }

  bool IsIgnored(const Message& msg1,
                 const Message& msg2,
                 const FieldDescriptor* field,
                 const std::vector<MessageDifferencer::SpecificField>& parent_fields) override
  {
    std::cout << field->full_name() << std::endl;
    if(set_ptr->find(field->full_name()) == set_ptr->end()){
      return true;
    } else {
      return false;
    }
  }

 private:
  std::unordered_set<std::string>* set_ptr;
};

class RegexIgnoreCriteria : public MessageDifferencer::IgnoreCriteria {
 public:
  RegexIgnoreCriteria(const std::string& regex){
    str_ptr = regex;
  }
  bool IsIgnored(const Message& msg1,
                 const Message& msg2,
                 const FieldDescriptor* field,
                 const std::vector<MessageDifferencer::SpecificField>& parent_fields) override
  {
    std::regex feature("^mes.*ress$");
    if ( std::regex_match(field->full_name(), feature) ){
      return true;
    } else {
      return false;
    }
  }

 private:
  std::string str_ptr;
};

// Deprecated!!!
class MapKeyCompatratorImpl : public MessageDifferencer::MapKeyComparator {
 public:
  MapKeyCompatratorImpl(const Message& message, const Descriptor* descriptor){
    map_ptr = new std::unordered_map<std::string, const FieldDescriptor*>;
    MapKeyCompatratorImpl::MapTraversalFiledDsp(message, descriptor, *map_ptr);

//    for (std::pair<std::string, const FieldDescriptor*> KeyVal : *map_ptr)
//    {
//      std::cout << KeyVal.first << " => " << KeyVal.second << std::endl;
//    }

    list_ptr = new std::vector<const FieldDescriptor*>;
    MapKeyCompatratorImpl::ListTraversalFieldDsp(message, descriptor, *list_ptr);

    for (int i = 0; i < (*list_ptr).size(); ++i){
      const FieldDescriptor* tmp = (*list_ptr).at(i);
      std::cout << "Field: " << i << "\t" << tmp->DebugString() << std::endl;
    }
  }

  static void MapTraversalFiledDsp(const Message& message,
                                    const Descriptor* descriptor,
                                    std::unordered_map<std::string, const FieldDescriptor*>& field_dict)
  {
    const Reflection* reflection = message.GetReflection();

    if (nullptr == reflection) {
      std::cout << "Reflection is null" << std::endl;
      return;
    }

    if (nullptr == descriptor) {
      return;
    }

    std::vector<const FieldDescriptor*> field_list;

    reflection->ListFields(message, &field_list);

    for(size_t i = 0; i < field_list.size(); ++i){
      const FieldDescriptor* field_ptr = field_list[i];

      if (field_ptr->cpp_type() != FieldDescriptor::CPPTYPE_MESSAGE &&
          !field_ptr->is_repeated())
      {
        std::string fullname = field_ptr->full_name();
        field_dict[fullname] = field_ptr;
        continue;
      }

      // if CPPTYPE_MESSAGE
      if (!field_ptr->is_repeated()) // Not Repeated
      {
        const Message& cur_msg = reflection->GetMessage(message, field_ptr);
        std::string fullname = field_ptr->full_name();
        field_dict[fullname] = field_ptr;

        const Descriptor* sub_dst = field_ptr->message_type();
        MapTraversalFiledDsp(cur_msg, sub_dst, field_dict);
      }
      else { // if repeated
        int count = reflection->FieldSize(message, field_ptr);
        std::string fullname = field_ptr->full_name();
        field_dict[fullname] = field_ptr;
        if (field_ptr->cpp_type() != FieldDescriptor::CPPTYPE_MESSAGE){
          continue;
        }
        // repeated and CPPTYPE_MESSAGE
        for (int j = 0; j < count; ++j) {
          const Message& cur_msg = reflection->GetRepeatedMessage(message, field_ptr, j);
          const Descriptor* sub_dst = field_ptr->message_type();
          MapTraversalFiledDsp(cur_msg, sub_dst, field_dict);
        }
      }
    }
  }

  static void ListTraversalFieldDsp(const Message& message,
                                    const Descriptor* descriptor,
                                    std::vector<const FieldDescriptor*>& field_list) {

    const Reflection* reflection = message.GetReflection();
    if (nullptr == reflection) {
      std::cout << "Reflection is null" << std::endl;
      return;
    }

    if (descriptor == nullptr){
      return;
    }

    const FieldDescriptor* field_ptr = nullptr;

    std::vector<const FieldDescriptor*> v_field_list;

    reflection->ListFields(message, &v_field_list);

    for(size_t i = 0; i < v_field_list.size(); ++i){
      field_ptr = v_field_list[i];
      if (field_ptr->cpp_type() != FieldDescriptor::CPPTYPE_MESSAGE &&
          !field_ptr->is_repeated()){
        field_list.push_back(field_ptr);
        continue;
      }

      // CPPTYPE_MESSAGE
      if (!field_ptr->is_repeated()) {
        //not repeated
        const Message& current_msg = reflection->GetMessage(message, field_ptr);
        field_list.push_back(field_ptr);
        const Descriptor* sub_dst = field_ptr->message_type();
        ListTraversalFieldDsp(current_msg, sub_dst, field_list);
      }
      else {
        int count = reflection->FieldSize(message, field_ptr);
        field_list.push_back(field_ptr);
        if (field_ptr->cpp_type() != FieldDescriptor::CPPTYPE_MESSAGE){
          continue;
        }
        // repeated and CPPTYPE_MESSAGE
        for (int j = 0; j < count; ++j) {
          const Message& current_msg = reflection->GetRepeatedMessage(message, field_ptr, j);
          const Descriptor* sub_dst = field_ptr->message_type();
          ListTraversalFieldDsp(current_msg, sub_dst, field_list);
        }
      }
    }
  }

//  bool IsMatch(const Message &message1, const Message &message2,
//               const std::vector<MessageDifferencer::SpecificField> &) const override{
//
//    bool res = true;
//    DefaultFieldComparator field_comparator;
//    for (int i = 0; i < (*list_ptr).size(); ++i) {
//      const FieldDescriptor* tmp_field = (*list_ptr).at(i);
//
//      if (tmp_field->cpp_type() != FieldDescriptor::CPPTYPE_MESSAGE &&
//          !tmp_field->is_repeated()){
//          field_comparator.Compare(message1, message2, tmp_field, -1, -1, nullptr);
//      }
////      field_comparator.Compare(message1, message2, tmp_field, -1, -1, nullptr);
//    }
//  }


 private:
  // I know these two is redundant.
  std::unordered_map<std::string, const FieldDescriptor*>* map_ptr;
  std::vector<const FieldDescriptor*>* list_ptr;
};

class KeyComparater : public MessageDifferencer::MapKeyComparator {
 public:
  typedef MessageDifferencer::SpecificField SpecificField;
  bool IsMatch(const Message& message1, const Message& message2,
                       const std::vector<SpecificField>& parent_fields) const override {
    const Reflection* reflection1 = message1.GetReflection();
    const Reflection* reflection2 = message2.GetReflection();

    // get the filed descriptor of a repeated message.
    const Descriptor* dsp = message1.GetDescriptor();

    for (int i = 0; i < dsp->field_count(); ++i){
      const FieldDescriptor* tmp_field = dsp->field(i);
      std::string field1 = reflection1->GetString(message1, tmp_field);
      std::string field2 = reflection2->GetString(message2, tmp_field);
      if (field1 != field2){
        return false;
      }
    }
    return true;

//    const FieldDescriptor* key_field = dsp->FindFieldByName("key");
//    std::string res1 = "", res2 = "";
//    for (int i = 0; i < reflection1->FieldSize(message1, key_field); ++i) {
//      res1 += reflection1->GetRepeatedString(message1, key_field, i);
//    }
//    for (int i = 0; i < reflection2->FieldSize(message2, key_field); ++i) {
//      res2 += reflection2->GetRepeatedString(message2, key_field, i);
//    }
//    return res1 == res2;
  }
};

class MessageServiceImpl final : public MessageServer::Service {
  Status GetContect(ServerContext* context, const MsgRequest* request,
                    MsgReply* reply) override {
    std::string prefix("Contect has");
    reply->set_reply(prefix + request->request());
    return Status::OK;
  }

  Status DifferTest(ServerContext* context, const log* log_message,
                     result* resDiff) override {

    // read the file descriptorProto
    FileDescriptorProto fdspProto;
    fdspProto.ParseFromString(log_message->file_descriptor_proto(0));

    // fetch all dependency filedescriptorProto
    DescriptorPool pool;
    for (int i = 1; i < log_message->file_descriptor_proto_size(); i++) {
      FileDescriptorProto tempProto_dep;
      tempProto_dep.ParseFromString(log_message->file_descriptor_proto(i));
      pool.BuildFile(tempProto_dep);
    }

    // Build the file descriptor.
    const FileDescriptor* fdsp = pool.BuildFile(fdspProto);

    // Read the name of the message descriptor
    std::string nameofMessageDsp;
    nameofMessageDsp = log_message->name_of_message_descriptor();

    // Generate the message descriptor by the name
    const Descriptor* descriptor =
        fdsp->FindMessageTypeByName(nameofMessageDsp);

    // Get the the abstract Message instance
    google::protobuf::DynamicMessageFactory factory(&pool);
    const Message* msg = factory.GetPrototype(descriptor);

    // Parer the base and test message from the log
    Message* msg_base = msg->New();
    msg_base->ParseFromString(log_message->base());
    Message* msg_test = msg->New();
    msg_test->ParseFromString(log_message->test());

    // Implements the IgnoreCriteria for specific filed list if the log.ignore_field_list is set.
    MessageDifferencer::IgnoreCriteria* ignore_list = nullptr;
    if (log_message->ignore_field_list_size()) {
      // Get the WHITE/BLACK list from the index 0.
      const std::string& criteria_flag = log_message->ignore_field_list(0);
      // Loop the filed list.
      if (criteria_flag == "BLACK"){
        auto* tmp_criteria = new blackListIgnoreCriteria();
        for (int i = 1; i < log_message->ignore_field_list_size(); i++){
          tmp_criteria->add_featureField(log_message->ignore_field_list(i));
        }
        ignore_list = tmp_criteria;
      }
      else if (criteria_flag == "WHITE"){
        auto* tmp_criteria = new whiteListIgnoreCriteria();
        for (int i = 1; i < log_message->ignore_field_list_size(); i++){
          tmp_criteria->add_featureField(log_message->ignore_field_list(i));
        }
        ignore_list = tmp_criteria;
      }
    }

    // Implements the IgnoreCriteria for regular expression if log.regex is set
    MessageDifferencer::IgnoreCriteria* criteria_regex = nullptr;
    if (!log_message->regex().empty()){
      auto* tmp_criteria = new RegexIgnoreCriteria(log_message->regex());
      criteria_regex = tmp_criteria;
    }

    // Compare the two message
    MessageDifferencer differ_obj;

    // Add the ignoreCriteria if set.
    if (ignore_list != nullptr){
      differ_obj.AddIgnoreCriteria(ignore_list);
    }
    if (criteria_regex != nullptr){
      differ_obj.AddIgnoreCriteria(criteria_regex);
    }

    // Output string for compare result.
    std::string differRes;
    differ_obj.ReportDifferencesToString(&differRes);
    differ_obj.Compare(*msg_base, *msg_test);

    // Set the compare result and return to the user.
    resDiff->set_res(differRes);
    return Status::OK;
  }

  Status MapKeyCompare(ServerContext* context, const log* log_message,
                       result* resDiff) override {

    // read the file descriptorProto
    FileDescriptorProto fdspProto;
    fdspProto.ParseFromString(log_message->file_descriptor_proto(0));

    // fetch all dependency filedescriptorProto
    DescriptorPool pool;
    for (int i = 1; i < log_message->file_descriptor_proto_size(); i++) {
      FileDescriptorProto tempProto_dep;
      tempProto_dep.ParseFromString(log_message->file_descriptor_proto(i));
      pool.BuildFile(tempProto_dep);
    }

    // Build the file descriptor.
    const FileDescriptor* fdsp = pool.BuildFile(fdspProto);

    // Read the name of the message descriptor
    std::string nameofMessageDsp;
    nameofMessageDsp = log_message->name_of_message_descriptor();

    // Generate the message descriptor by the name
    const Descriptor* descriptor =
        fdsp->FindMessageTypeByName(nameofMessageDsp);

    if (nullptr == descriptor){
      std::cout << "Descriptor is null" << std::endl;
      return Status::CANCELLED;
    }

    google::protobuf::DynamicMessageFactory factory(&pool);
    const Message* msg = factory.GetPrototype(descriptor);

    // Parer the base and test message from the log
    Message* msg_base = msg->New();
    msg_base->ParseFromString(log_message->base());
//    std::cout << "Base Message: \n"
//              << msg_base->DebugString() << std::endl;


    Message* msg_test = msg->New();
    msg_test->ParseFromString(log_message->test());
//    std::cout << "Test Message: \n"
//              << msg_test->DebugString() << std::endl;


    // repeated filed input from the user.
    std::string field_name = log_message->field_names(0);
    const FieldDescriptor* repeated_FieldDescriptor = descriptor->FindFieldByName(field_name);


    // Generate a MapkeyComparator to compare the repeated
    const MessageDifferencer::MapKeyComparator* comparater = new KeyComparater();

    // Create a Message Differencer and add the MapKeyComparator for the repeated field.
    MessageDifferencer differencer;
    differencer.TreatAsMapUsingKeyComparator(repeated_FieldDescriptor, comparater);

    std::string output;
    differencer.ReportDifferencesToString(&output);
    differencer.Compare(*msg_base, *msg_test);

//    std::cout << "Result: \n"
//              << output << std::endl;

    resDiff->set_res(output);
    return Status::OK;
  }


};

void RunServer() {
  std::string server_address("0.0.0.0:50053");
  MessageServiceImpl service;

  grpc::EnableDefaultHealthCheckService(true);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();

  ServerBuilder builder;

  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());

  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);

  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
}

int main(int argc, const char* argv[]) {
  RunServer();

  return 0;
}