#include <google/protobuf/compiler/importer.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/message.h>
#include <google/protobuf/util/message_differencer.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>

#include <iostream>
#include <memory>
#include <regex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "differential_server_lib/differential_service.grpc.pb.h"

using google::protobuf::Descriptor;
using google::protobuf::DescriptorPool;
using google::protobuf::DescriptorProto;
using google::protobuf::FieldDescriptor;
using google::protobuf::FileDescriptor;
using google::protobuf::FileDescriptorProto;

using google::protobuf::Message;
using google::protobuf::MessageFactory;
using google::protobuf::Reflection;
using google::protobuf::util::DefaultFieldComparator;
using google::protobuf::util::MessageDifferencer;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using differentialservice::DifferentialServer;
using differentialservice::DiffMsgRequest;
using differentialservice::DiffMsgReply;
using differentialservice::MsgReply;
using differentialservice::MsgRequest;


// A helper function for GetFieldDescriptor() function.
std::vector<std::string> stringSplit(const std::string& string) {
  std::vector<std::string> res;
  std::stringstream check1(string);
  std::string intermediate;
  while (getline(check1, intermediate, '.')) {
    res.push_back(intermediate);
  }
  return res;
}

// Get the field descriptor of a message.
const FieldDescriptor* GetFieldDescriptor(const Message& message,
                                          const std::string& field_name) {
  std::vector<std::string> field_path = stringSplit(field_name);
  const Descriptor* descriptor = message.GetDescriptor();
  const FieldDescriptor* field = nullptr;
  for (int i = 0; i < field_path.size(); ++i) {
    field = descriptor->FindFieldByName(field_path[i]);
    descriptor = field->message_type();
  }
  return field;
}

class FieldIgnoreCriteria : public MessageDifferencer::IgnoreCriteria {
 public:
  FieldIgnoreCriteria() { set_ptr = new std::unordered_set<std::string>; }

  void add_featureField(const std::string& field) { set_ptr->insert(field); }

  bool IsIgnored(const Message& msg1, const Message& msg2,
                 const FieldDescriptor* field,
                 const std::vector<MessageDifferencer::SpecificField>&
                     parent_fields) override {
    if (set_ptr->find(field->full_name()) == set_ptr->end()) {
      return false;
    } else {
      return true;
    }
  }

 private:
  std::unordered_set<std::string>* set_ptr;
};

class FieldCompareCriteria : public MessageDifferencer::IgnoreCriteria {
 public:
  FieldCompareCriteria() { set_ptr = new std::unordered_set<std::string>; }

  void add_featureField(const std::string& field) { set_ptr->insert(field); }

//  void print(std::unordered_set<std::string> const &s){
//    std::copy(s.begin(), s.end(), std::ostream_iterator<std::string>(std::cout, " "));
//  }

  bool IsIgnored(const Message& msg1, const Message& msg2,
                 const FieldDescriptor* field,
                 const std::vector<MessageDifferencer::SpecificField>&
                     parent_fields) override
  {
    if (set_ptr->find(field->full_name()) == set_ptr->end()) {
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
  explicit RegexIgnoreCriteria(const std::string& regex) { str_ptr = regex; }
  bool IsIgnored(const Message& msg1, const Message& msg2,
                 const FieldDescriptor* field,
                 const std::vector<MessageDifferencer::SpecificField>&
                     parent_fields) override {
    std::regex feature("^mes.*ress$");
    if (std::regex_match(field->full_name(), feature)) {
      return true;
    } else {
      return false;
    }
  }

 private:
  std::string str_ptr;
};

class KeyComparater : public MessageDifferencer::MapKeyComparator {
 public:
  KeyComparater(const FieldDescriptor* field_1,
                const FieldDescriptor* field_2) {
    keyField_1 = field_1;
    keyField_2 = field_2;
  }
  typedef MessageDifferencer::SpecificField SpecificField;
  bool IsMatch(const Message& message1, const Message& message2,
               const std::vector<SpecificField>& parent_fields) const override {
    const Reflection* reflection1 = message1.GetReflection();
    const Reflection* reflection2 = message2.GetReflection();

    if (keyField_1->cpp_type() != keyField_2->cpp_type()) {
      return false;
    }

    FieldDescriptor::CppType type = keyField_1->cpp_type();

    bool key_match = true;

    switch (type) {
      case FieldDescriptor::CPPTYPE_STRING: {
        std::string field1 = reflection1->GetString(message1, keyField_1);
        std::string field2 = reflection2->GetString(message2, keyField_2);
        if (field1 != field2) {
          key_match = false;
        }
        break;
      }
      case FieldDescriptor::CPPTYPE_INT32: {
        std::int32_t field1 = reflection1->GetInt32(message1, keyField_1);
        std::int32_t field2 = reflection2->GetInt32(message2, keyField_2);
        if (field1 != field2) {
          key_match = false;
        }
        break;
      }
      case FieldDescriptor::CPPTYPE_INT64: {
        std::int64_t field1 = reflection1->GetInt64(message1, keyField_1);
        std::int64_t field2 = reflection2->GetInt64(message2, keyField_2);
        if (field1 != field2) {
          key_match = false;
        }
        break;
      }
      case FieldDescriptor::CPPTYPE_UINT32: {
        std::uint32_t field1 = reflection1->GetUInt32(message1, keyField_1);
        std::uint32_t field2 = reflection2->GetUInt32(message2, keyField_2);
        if (field1 != field2) {
          key_match = false;
        }
        break;
      }
      case FieldDescriptor::CPPTYPE_UINT64: {
        std::uint64_t field1 = reflection1->GetUInt64(message1, keyField_1);
        std::uint64_t field2 = reflection2->GetUInt64(message2, keyField_2);
        if (field1 != field2) {
          key_match = false;
        }
        break;
      }
      case FieldDescriptor::CPPTYPE_DOUBLE: {
        double field1 = reflection1->GetDouble(message1, keyField_1);
        double field2 = reflection2->GetDouble(message2, keyField_2);
        if (field1 != field2) {
          key_match = false;
        }
        break;
      }
      case FieldDescriptor::CPPTYPE_FLOAT: {
        float field1 = reflection1->GetFloat(message1, keyField_1);
        float field2 = reflection2->GetFloat(message2, keyField_2);
        if (field1 != field2) {
          key_match = false;
        }
        break;
      }
      case FieldDescriptor::CPPTYPE_BOOL: {
        bool field1 = reflection1->GetBool(message1, keyField_1);
        bool field2 = reflection2->GetBool(message2, keyField_2);
        if (field1 != field2) {
          key_match = false;
        }
        break;
      }
      case FieldDescriptor::CPPTYPE_ENUM:
        break;
      case FieldDescriptor::CPPTYPE_MESSAGE: {
        const Message& sub_msg_1 =
            reflection1->GetMessage(message1, keyField_1);
        const Message& sub_msg_2 =
            reflection2->GetMessage(message2, keyField_2);

        MessageDifferencer differencer;
        if (!differencer.Compare(sub_msg_1, sub_msg_2)) {
          key_match = false;
        }
      }
    }

    if (!key_match){
      return false;
    }
    auto* differencer = new MessageDifferencer();
    if( !differencer->Compare(message1, message2) ){
      return false;
    }

    return true;
  }

 private:
  const FieldDescriptor* keyField_1;
  const FieldDescriptor* keyField_2;
};

class MessageServiceImpl final : public DifferentialServer::Service
{
  Status GetConnect(ServerContext* context, const MsgRequest* request,
                    MsgReply* reply) override {
    std::string prefix("1234");
    reply->set_reply(prefix + request->request());
    return Status::OK;
  }

  Status DefaultDifferentialService(ServerContext* context,
                                    const DiffMsgRequest* user_request,
                                    DiffMsgReply* resDiff) override {
    // read the file descriptorProto
    FileDescriptorProto file_descriptor_proto;
    file_descriptor_proto.ParseFromString(
        user_request->file_descriptor_proto(0));

    // fetch all dependency filedescriptorProto
    DescriptorPool descriptorPool;
    for (int i = 1; i < user_request->file_descriptor_proto_size(); i++) {
      FileDescriptorProto dependency_Proto;
      dependency_Proto.ParseFromString(user_request->file_descriptor_proto(i));
      descriptorPool.BuildFile(dependency_Proto);
    }

    // Build the file descriptor.
    const FileDescriptor* file_descriptor = descriptorPool.BuildFile(file_descriptor_proto);

    // Read the name of the message descriptor
    std::string name_of_message_descriptor;
    name_of_message_descriptor = user_request->name_of_message_descriptor();

    // Generate the message descriptor by the name
    const Descriptor* descriptor =file_descriptor->FindMessageTypeByName(name_of_message_descriptor);

    // Get the the abstract Message instance
    google::protobuf::DynamicMessageFactory factory(&descriptorPool);
    const Message* msg = factory.GetPrototype(descriptor);

    // Parer the base and test message from the log
    Message* msg_base = msg->New();
    msg_base->ParseFromString(user_request->message_1());

    Message* msg_test = msg->New();
    msg_test->ParseFromString(user_request->message_2());

    // Compare the two message
    MessageDifferencer differ_obj;

    // Output string for compare result.
    std::string differRes;
    differ_obj.ReportDifferencesToString(&differRes);
    bool diff_flag = differ_obj.Compare(*msg_base, *msg_test);

    // Set the compare result and return to the user.
//    resDiff->set_result(differRes);

    // Set the compare result and return to the user.
    if (diff_flag) {
      resDiff->set_result("SAME");
    }
    else {
      resDiff->set_result(differRes);
    }

    return Status::OK;


  }

  Status DifferentialService(ServerContext* context, const DiffMsgRequest* user_request,
                             DiffMsgReply* resDiff) override {
    /****************************************
     *
     * Parse the user messages
     *
     ****************************************/
    // read the file descriptorProto
    FileDescriptorProto file_descriptor_proto;
    file_descriptor_proto.ParseFromString(user_request->file_descriptor_proto(0));

    // fetch all dependency filedescriptorProto
    DescriptorPool descriptorPool;
    for (int i = 1; i < user_request->file_descriptor_proto_size(); i++) {
      FileDescriptorProto dependency_Proto;
      dependency_Proto.ParseFromString(user_request->file_descriptor_proto(i));
      descriptorPool.BuildFile(dependency_Proto);
    }

    // Build the file descriptor.
    const FileDescriptor* file_descriptor =
        descriptorPool.BuildFile(file_descriptor_proto);

    // Read the name of the message descriptor
    std::string name_of_message_descriptor;
    name_of_message_descriptor = user_request->name_of_message_descriptor();

    // Generate the message descriptor by the name
    const Descriptor* descriptor =
        file_descriptor->FindMessageTypeByName(name_of_message_descriptor);

    // Get the the abstract Message instance
    google::protobuf::DynamicMessageFactory factory(&descriptorPool);
    const Message* msg = factory.GetPrototype(descriptor);

    // Parer the base and test message from the log
    Message* msg_base = msg->New();
    msg_base->ParseFromString(user_request->message_1());
//    std::cout << "Base Message: \n" << msg_base->DebugString() << std::endl;

    Message* msg_test = msg->New();
    msg_test->ParseFromString(user_request->message_2());
//    std::cout << "Test Message: \n" << msg_test->DebugString() << std::endl;

    /************************************************************************
     *
     * Compare the two message
     *
     ************************************************************************/
    MessageDifferencer differencer;

    /*********************************************************
     *
     * Check the ignore criteria set by user.
     *
     *********************************************************/

    // Don't show the ignore in report.
    differencer.set_report_ignores(false);

    // User implements the ignore criteria.
    if (user_request->has_user_ignore()) {
      const differentialservice::IgnoreCriteria& user_ignoreCriteria =
          user_request->user_ignore();

      // IgnoreCriteria from protobuf MessageDifferencer.
      MessageDifferencer::IgnoreCriteria* ignoreCriteria = nullptr;

      // loop the fields that set by user.
      if (user_ignoreCriteria.ignore_fields_list_size() != 0) {
        // 1) Check the IgnoreFlag set by user. The IgnoreFlag is a binary option.
        // ...FLAG_IGNORE or ...FLAG_COMPARE
        if (user_ignoreCriteria.flag() == differentialservice::IgnoreCriteria_IgnoreFlag_FLAG_IGNORE)
        {
          auto* tmp_criteria = new FieldIgnoreCriteria();
          // loop the fields and insert to the tmp_criteria
          for (int i = 0; i < user_ignoreCriteria.ignore_fields_list_size(); ++i) {
            tmp_criteria->add_featureField(user_ignoreCriteria.ignore_fields_list(i));
          }
          ignoreCriteria = tmp_criteria;
        }
        else
          // IgnoreCriteria_IgnoreFlag_FLAG_COMPARE
        {
          auto* tmp_criteria = new FieldCompareCriteria();
          for (int i = 0; i < user_ignoreCriteria.ignore_fields_list_size(); ++i) {
            tmp_criteria->add_featureField(user_ignoreCriteria.ignore_fields_list(i));
          }
          ignoreCriteria = tmp_criteria;
        }

        // Add the ignoreCriteria to the differencer if set.
        differencer.AddIgnoreCriteria(ignoreCriteria);
      }

      // If the regex is set.
      MessageDifferencer::IgnoreCriteria* criteria_regex = nullptr;

      if (!user_ignoreCriteria.regex().empty()) {
        auto* tmp_criteria =
            new RegexIgnoreCriteria(user_ignoreCriteria.regex());
        criteria_regex = tmp_criteria;
      }

      // Add the ignoreCriteria if set.
      if (criteria_regex != nullptr) {
        differencer.AddIgnoreCriteria(criteria_regex);
      }
    }

    /********************************************************
     *
     * Check the repeated field comparison methods (LIST or SET).
     *
     *******************************************************/

    // Don't show the more in report
    differencer.set_report_moves(false);

    // Check the repeated fields list.
    if (user_request->repeated_field_size()) {
      // Get the number of the fields.
      int num = user_request->repeated_field_size();
      for (int i = 0; i < num; ++i) {
        // Get the repeated field Tuple <flag, field_name>
        // flag is binary option ...FLAG_LIST or ...FLAG_SET
        const differentialservice::RepeatedFieldTuple& fieldTuple =
            user_request->repeated_field(i);
        if (fieldTuple.flag() ==
            differentialservice::RepeatedFieldTuple_TreatAsFlag_FLAG_LIST)
        {
          const std::string& field_name = fieldTuple.field_name();
          const FieldDescriptor* field_dsp =
              GetFieldDescriptor(*msg_base, field_name);

          // Tell the differencer treat the field as LIST
          differencer.TreatAsList(field_dsp);

        } else {  // RepeatedFieldTuple_TreatAsFlag_FLAG_LIST
          const std::string& field_name = fieldTuple.field_name();
          const FieldDescriptor* field_dsp =
              GetFieldDescriptor(*msg_base, field_name);

          // Tell the differencer treat the field as SET
          differencer.TreatAsSet(field_dsp);
        }
      }
    }

    /*******************************************************
     *
     * Check the repeated field comparison methods (MAP)
     *
     *******************************************************/
    // if set the map_comapre is set by user.
    if (user_request->map_compare_size() != 0) {
      int num = user_request->map_compare_size();
      for (int i = 0; i < num; ++i) {
        // Get the MapCompareTuple <name of repeated field, one/more sub field as map>
        const differentialservice::MapCompareTuple& tuple =
            user_request->map_compare(i);

        // Get the repeated filed descriptor
        const std::string& field_name = tuple.name_of_repeated_field();
        // get the field descriptor by user input message(*msg), and field name.
        // "GetFieldDescriptor" is a helper function declared at the top of file.
        const FieldDescriptor* field_dsp = GetFieldDescriptor(*msg, field_name);

        // Get the message descriptor of the repeated filed.
        const Descriptor* tmp_dsp = field_dsp->message_type();

        // User could select one/more sub-field as the Map field.
        // if the map key is a single sub-field.
        if (tuple.name_of_sub_field_size() == 1) {
          const std::string& key_field_name = tuple.name_of_sub_field(0);
          if (tmp_dsp->FindFieldByName(key_field_name) != nullptr) {
            const FieldDescriptor* key_field =
                tmp_dsp->FindFieldByName(key_field_name);
            differencer.TreatAsMap(field_dsp, key_field);
          } else {
            // ToDo: Add a error log in messageservice::result
            break;
          }
        }
        // If user set multiple sub-fields as the Map field.
          // Loop all map fields and add into the vector key_fields.
        else {

          auto* key_fields = new std::vector<const FieldDescriptor*>;
          int key_num = tuple.name_of_sub_field_size();
          for (int j = 0; j < key_num; ++j) {
            const std::string& key_field_name = tuple.name_of_sub_field(j);
            if (tmp_dsp->FindFieldByName(key_field_name) != nullptr) {
              const FieldDescriptor* key_field =
                  tmp_dsp->FindFieldByName(key_field_name);
              key_fields->push_back(key_field);
            } else {
              // ToDo: Add a error log in messageservice::result
              break;
            }
          }

          // Tell the differencer how to treat the map field.
          differencer.TreatAsMapWithMultipleFieldsAsKey(field_dsp, *key_fields);
        }
      }
    }

    /*******************************************************
     *
     *  Check the float point number comparison
     *
     *******************************************************/
    // if float_comparison is set
    if (user_request->has_fraction_margin()) {
      const differentialservice::FloatNumComparison& fraction_Margin =
          user_request->fraction_margin();

      double fraction = fraction_Margin.fraction();
      double margin = fraction_Margin.margin();

      auto* fieldComparator = new DefaultFieldComparator();

      // Set the float comparison as APPRROXIMATE.
      fieldComparator->set_float_comparison(
          DefaultFieldComparator::APPROXIMATE);
      fieldComparator->SetDefaultFractionAndMargin(fraction, margin);

      // Add the field comparator to the differencer.
      differencer.set_field_comparator(fieldComparator);
    }

    /*
     * Try~~
     */

//    const FieldDescriptor* specificField = descriptor->FindFieldByName("foo");
//
//    const Descriptor* specificField_type = specificField->message_type();
//    const FieldDescriptor* key_field_1 =
//        specificField_type->FindFieldByName("exam1");
//    const FieldDescriptor* key_field_2 =
//        specificField_type->FindFieldByName("exam2");
//
//    if (key_field_1 != nullptr && key_field_2 != nullptr) {
//      const MessageDifferencer::MapKeyComparator* Comparater =
//          new KeyComparater(key_field_1, key_field_2);
//
//      differ_obj.TreatAsMapUsingKeyComparator(specificField, Comparater);
//    } else {
//      // ToDo: Error message
//    }


    /*******************************************************
     *
     * Compare and output the result.
     *
     *******************************************************/
    // Output string for compare result.
    std::string differRes;
    differencer.ReportDifferencesToString(&differRes);
    bool diff_flag = differencer.Compare(*msg_base, *msg_test);

    // Set the compare result and return to the user.
    if (diff_flag) {
      resDiff->set_result("SAME");
    }
    else {
      resDiff->set_result(differRes);
    }

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