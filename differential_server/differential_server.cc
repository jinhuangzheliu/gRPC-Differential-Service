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

using DifferentialService::ServerDifferential;
using DifferentialService::DiffRequest;
using DifferentialService::DiffResponse;
using DifferentialService::MsgReply;
using DifferentialService::MsgRequest;

// Declare a unordered_set printer function for debug propose.
void SetPrinter(std::unordered_set<std::string> const &s){
    std::copy(s.begin(), s.end(), std::ostream_iterator<std::string>(std::cout, " "));
 }

// A helper function for split string type value implemented by GetFieldDescriptor() function.
std::vector<std::string> StringSplit(const std::string& string) {
  std::vector<std::string> res;
  std::stringstream check1(string);
  std::string intermediate;
  while (getline(check1, intermediate, '.')) {
    res.push_back(intermediate);
  }
  return res;
}

// Get the field descriptor for a string type of field name.(TestEmployee.fullname)
const FieldDescriptor* GetFieldDescriptor(const Message& message, const std::string& field_name) {
  // Split the field name by "." ("TestEmployee.fullname" => ["TestEmployee", "fullname"])
  std::vector<std::string> field_path = StringSplit(field_name);
  const Descriptor* descriptor = message.GetDescriptor();
  const FieldDescriptor* field = nullptr;

  // Loop the field name until the last one.
  for (int i = 0; i < field_path.size(); ++i) {
    field = descriptor->FindFieldByName(field_path[i]);
    descriptor = field->message_type();
  }
  return field;
}

/*
 *   Implement the IgnoreCriteria to ignore the specific fields
 */
class IgnoreFieldImpl : public MessageDifferencer::IgnoreCriteria {
 public:
  IgnoreFieldImpl() { ignore_fields = new std::unordered_set<std::string>; }

  // Add the field name into the ignore_fields
  void AddField(const std::string& field){
    ignore_fields->insert(field);
  }

  bool IsIgnored(const Message& msg1, const Message& msg2, const FieldDescriptor* field,
                 const std::vector<MessageDifferencer::SpecificField>& parent_fields) override {
    // Check if the field name is in the ignore_fields
    if (ignore_fields->find(field->full_name()) == ignore_fields->end()) {
      // If not in ignore_fields.
      return false;
    } else {
      return true;
    }
  }

 private:
  std::unordered_set<std::string>* ignore_fields;
};

/*
 *   Implement the IgnoreCriteria to compare the specific fields
 */
class CompareFieldImpl : public MessageDifferencer::IgnoreCriteria {
 public:
  CompareFieldImpl() {
    compare_fields = new std::unordered_set<std::string>;
  }

  // Add the field name into the compare_fields.
  void AddField(const std::string& field) {
    compare_fields->insert(field);
  }

  bool IsIgnored(const Message& msg1, const Message& msg2, const FieldDescriptor* field,
                 const std::vector<MessageDifferencer::SpecificField>& parent_fields) override {
    // Check if the field name is in the compare_field.
    if (compare_fields->find(field->full_name()) == compare_fields->end()) {
      // if not in the comapre_field ignroe.
      return true;
    } else {
      return false;
    }
  }

 private:
  std::unordered_set<std::string>* compare_fields;
};


/*
 *   Implement the IgnoreCriteria to ignore field by regular expression.
 */
class RegexIgnoreCriteria : public MessageDifferencer::IgnoreCriteria {
 public:
  explicit RegexIgnoreCriteria(const std::string& regex) { str_ptr = regex; }
  bool IsIgnored(const Message& msg1, const Message& msg2, const FieldDescriptor* field,
                 const std::vector<MessageDifferencer::SpecificField>& parent_fields) override {
    std::regex feature(str_ptr);
    if (std::regex_match(field->full_name(), feature)) {
      return true;
    } else {
      return false;
    }
  }

 private:
  std::string str_ptr;
};

/*
 *  Implement the MapKeyComparator to compare two repeated field without same
 *  index key field.

 As the following example, we want to compare the field ExamScore
 by Key [first_exam] in the first field and Key [second_exam] in the seconde field

 [Message definition]:
 message ExamScore{
    string first_exam = 1;
    int32 first_score = 2;
    string second_exam = 3;
    int32 seconde_score = 4;
}

 message TestEmployee{
 repeated ExamScore exam_score = 1;
 }


[Content of First Message]:
first_exam = "Mid-term"; <-------- first key
first_score = 98;
second_exam = "Final";
seconde_score = 89;


[Content of Second Message]:
first_exam = "Final";
first_score = 92;
second_exam = "Mid-term"; <------ second key
seconde_score = 91;

 */
class KeyComparatorImpl : public MessageDifferencer::MapKeyComparator {
 public:
  /*
   * Passing the [key] field for the first compare message and second [key] field for the second message
   */
  KeyComparatorImpl(const FieldDescriptor* fst_kfield, const FieldDescriptor* scd_kfield) {
    first_key_field = fst_kfield;
    second_key_field = scd_kfield;
  }

  typedef MessageDifferencer::SpecificField SpecificField;

  bool IsMatch(const Message& message1, const Message& message2,
               const std::vector<SpecificField>& parent_fields) const override {
    // Get the reflection of the two message.
    const Reflection* first_msg_reflection = message1.GetReflection();
    const Reflection* second_msg_reflection = message2.GetReflection();

    // If the type of two key field is different return false
    if (first_key_field->cpp_type() != second_key_field->cpp_type()) {
      return false;
    }

    // Get the Cpp type of the field descriptor.
    FieldDescriptor::CppType key_field_type = first_key_field->cpp_type();

    // 1) compare the key field first and set the default as true.
    bool key_match = true;
    switch (key_field_type) {
      case FieldDescriptor::CPPTYPE_STRING: {
        std::string fst_field = first_msg_reflection->GetString(message1, first_key_field);
        std::string sed_field = second_msg_reflection->GetString(message2, second_key_field);
        if (fst_field != sed_field) {
          key_match = false;
        }
        break;
      }
      case FieldDescriptor::CPPTYPE_INT32: {
        std::int32_t fst_field = first_msg_reflection->GetInt32(message1, first_key_field);
        std::int32_t sed_field = second_msg_reflection->GetInt32(message2, second_key_field);
        if (fst_field != sed_field) {
          key_match = false;
        }
        break;
      }
      case FieldDescriptor::CPPTYPE_INT64: {
        std::int64_t fst_field = first_msg_reflection->GetInt64(message1, first_key_field);
        std::int64_t sed_field = second_msg_reflection->GetInt64(message2, second_key_field);
        if (fst_field != sed_field) {
          key_match = false;
        }
        break;
      }
      case FieldDescriptor::CPPTYPE_UINT32: {
        std::uint32_t fst_field = first_msg_reflection->GetUInt32(message1, first_key_field);
        std::uint32_t sed_field = second_msg_reflection->GetUInt32(message2, second_key_field);
        if (fst_field != sed_field) {
          key_match = false;
        }
        break;
      }
      case FieldDescriptor::CPPTYPE_UINT64: {
        std::uint64_t fst_field = first_msg_reflection->GetUInt64(message1, first_key_field);
        std::uint64_t sed_field = second_msg_reflection->GetUInt64(message2, second_key_field);
        if (fst_field != sed_field) {
          key_match = false;
        }
        break;
      }
      case FieldDescriptor::CPPTYPE_DOUBLE: {
        double fst_field = first_msg_reflection->GetDouble(message1, first_key_field);
        double sed_field = second_msg_reflection->GetDouble(message2, second_key_field);
        if (fst_field != sed_field) {
          key_match = false;
        }
        break;
      }
      case FieldDescriptor::CPPTYPE_FLOAT: {
        float fst_field = first_msg_reflection->GetFloat(message1, first_key_field);
        float sed_field = second_msg_reflection->GetFloat(message2, second_key_field);
        if (fst_field != sed_field) {
          key_match = false;
        }
        break;
      }
      case FieldDescriptor::CPPTYPE_BOOL: {
        bool fst_field = first_msg_reflection->GetBool(message1, first_key_field);
        bool sed_field = second_msg_reflection->GetBool(message2, second_key_field);
        if (fst_field != sed_field) {
          key_match = false;
        }
        break;
      }
      case FieldDescriptor::CPPTYPE_ENUM:
        break;
      case FieldDescriptor::CPPTYPE_MESSAGE: {
        const Message& first_sub_msg = first_msg_reflection->GetMessage(message1, first_key_field);
        const Message& second_sub_msg = second_msg_reflection->GetMessage(message2, second_key_field);

        MessageDifferencer differencer;
        if (!differencer.Compare(first_sub_msg, second_sub_msg)) {
          key_match = false;
        }
      }
    }

    // If the key field don't match return false.
    if (!key_match){
      return false;
    }

    // 2) remove the key field from two messages
    // Because the message1 and message2 is "const Message" type so we need to create a new message
    // as "Message" type and use the message reflection to remove the key field from this new message.
    const Descriptor* descriptor = message1.GetDescriptor();

    // Create two new "not const" Message
    Message* new_msg_1 = message1.New();
    Message* new_msg_2 = message2.New();


    // Copy the value from the const message.
    new_msg_1->ParseFromString(message1.SerializeAsString());
    new_msg_2->ParseFromString(message2.SerializeAsString());

    std::cout << new_msg_1->DebugString() << std::endl;
    std::cout << new_msg_2->DebugString() << std::endl;


    // Get the reflection of these two messages
    const Reflection* new_reflection_1 = new_msg_1->GetReflection();
    const Reflection* new_reflection_2 = new_msg_1->GetReflection();


    // remove the key field from the new msg so the fields in the new message is [value] fields.
    new_reflection_1->ClearField(new_msg_1, first_key_field);
    new_reflection_2->ClearField(new_msg_2, second_key_field);

    std::cout << new_msg_1->DebugString() << std::endl;
    std::cout << new_msg_2->DebugString() << std::endl;


    // Compare the new msg.
    auto* differencer = new MessageDifferencer();
    if( !differencer->Compare(*new_msg_1, *new_msg_2) )
    {
      return false;
    }
    return true;
  }

 private:
  const FieldDescriptor* first_key_field;
  const FieldDescriptor* second_key_field;
};

class MessageServiceImpl final : public ServerDifferential::Service
{
  Status CompareInputMessages(ServerContext* context, const DiffRequest* diff_request, DiffResponse* diff_response) override {
    /*
     * 0) Check the size of the input proto message.
     */
    size_t proto_max_size = 4194304;
    size_t size_of_request_msg = diff_request->ByteSizeLong();

    if (size_of_request_msg > proto_max_size){
      diff_response->set_error("The size of request message larger than max(4194304).");
      return Status::CANCELLED;
    }


    /****************************************************************************
     *
     * 1) Dynamically re-generate the compared two messages.
     *
     ****************************************************************************/
    // Reading the file descriptorProto and their dependency and saving into the descritor pool.
    FileDescriptorProto file_descriptor_proto;
    // The first element is the file descriptor proto.
    file_descriptor_proto.ParseFromString(diff_request->file_descriptor_proto(0));

    // Use descriptor pool to get all the dependency of the file descriptor proto.
    DescriptorPool descriptor_pool;
    for (int i = 1; i < diff_request->file_descriptor_proto_size(); i++) {
      FileDescriptorProto dependency_Proto;
      dependency_Proto.ParseFromString(diff_request->file_descriptor_proto(i));
      descriptor_pool.BuildFile(dependency_Proto);
    }

    // Build the file descriptor by the file descriptor and descriptor pool.
    const FileDescriptor* file_descriptor = descriptor_pool.BuildFile(file_descriptor_proto);

    // Read the name of the message descriptor
    std::string name_of_message_descriptor = diff_request->name_of_message();

    // Using the name of the message descriptor to generate the message descriptor
    const Descriptor* descriptor = file_descriptor->FindMessageTypeByName(name_of_message_descriptor);

    // Get the the abstract Message instance by Message factory
    google::protobuf::DynamicMessageFactory factory(&descriptor_pool);
    const Message* msg = factory.GetPrototype(descriptor);

    // Parer the base and test message from the log
    Message* msg_base = msg->New();
    msg_base->ParseFromString(diff_request->first_message());
//    std::cout << "Base Message: \n" << msg_base->DebugString() << std::endl;

    Message* msg_test = msg->New();
    msg_test->ParseFromString(diff_request->second_message());
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
    if (diff_request->has_user_ignore()) {
      const DifferentialService::IgnoreCriteria& user_ignoreCriteria = diff_request->user_ignore();

      // IgnoreCriteria from protobuf MessageDifferencer.
      MessageDifferencer::IgnoreCriteria* ignoreCriteria = nullptr;

      // loop the fields that set by user.
      if (user_ignoreCriteria.ignore_fields_list_size() != 0) {
        // 1) Check the IgnoreFlag set by user. The IgnoreFlag is a binary option.
        // ...FLAG_IGNORE or ...FLAG_COMPARE
        if (user_ignoreCriteria.flag() == DifferentialService::IgnoreCriteria_IgnoreFlag_FLAG_IGNORE){
          auto* tmp_criteria = new IgnoreFieldImpl();
          // loop the fields and insert to the tmp_criteria
          for (int i = 0; i < user_ignoreCriteria.ignore_fields_list_size(); ++i) {
            tmp_criteria->AddField(user_ignoreCriteria.ignore_fields_list(i));
          }
          ignoreCriteria = tmp_criteria;
        }
        else if (user_ignoreCriteria.flag() == DifferentialService::IgnoreCriteria_IgnoreFlag_FLAG_COMPARE){
          auto* tmp_criteria = new CompareFieldImpl();

          for (int i = 0; i < user_ignoreCriteria.ignore_fields_list_size(); ++i) {
            tmp_criteria->AddField(user_ignoreCriteria.ignore_fields_list(i));
          }
          ignoreCriteria = tmp_criteria;
        }
        else {
          // HANDLE exception if the IgnoreFlag set as INVALID. Actually, this will not happened in the normal situation.
          diff_response->set_error("ERROR: The IgnoreCriteria_IgnoreFlag is set by \"INVALID\".");
        }

        // Add the ignoreCriteria to the differencer if set.
        differencer.AddIgnoreCriteria(ignoreCriteria);
      }

      // If the regex is set.
      MessageDifferencer::IgnoreCriteria* criteria_regex = nullptr;

      if (!user_ignoreCriteria.regex().empty()) {
        auto* tmp_criteria = new RegexIgnoreCriteria(user_ignoreCriteria.regex());
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
    if (diff_request->repeated_field_size()) {

      // Get the number of the fields.
      int num = diff_request->repeated_field_size();
      for (int i = 0; i < num; ++i) {

        // Get the repeated field Tuple <flag, field_name> flag is binary option ...FLAG_LIST or ...FLAG_SET
        const DifferentialService::RepeatedField& repeated_field_tuple = diff_request->repeated_field(i);

        if (repeated_field_tuple.flag() == DifferentialService::RepeatedField_TreatAsFlag_DEFAULT_LIST){
          // Get the field name
          const std::string& field_name = repeated_field_tuple.field_name();
          // Get the field descriptor by name.

          // [ATTENTION] "GetFieldDescriptor" is a helper function declared at the top of file.

          const FieldDescriptor* field_dsp = GetFieldDescriptor(*msg_base, field_name);

          // Tell the differencer treat the field as LIST
          differencer.TreatAsList(field_dsp);

        } else {  // RepeatedFieldTuple_TreatAsFlag_FLAG_LIST
          // Get the field name
          const std::string& field_name = repeated_field_tuple.field_name();
          // Get the field descriptor by name.
          const FieldDescriptor* field_dsp = GetFieldDescriptor(*msg_base, field_name);

          // Tell the differencer treat the field as SET
          differencer.TreatAsSet(field_dsp);
        }
      }
    }

    /*******************************************************
     *
     * Check the repeated field will be treated as a map for diffing purposes.
     *
     *******************************************************/
    // if set the map_compare is set by user.
    if (diff_request->map_compare_size() != 0) {
      int num = diff_request->map_compare_size();
      for (int i = 0; i < num; ++i) {
        // Get the MapCompareTuple <name of repeated field, one/more sub field as map>
        const DifferentialService::MapCompare& tuple = diff_request->map_compare(i);

        // Get the repeated filed name
        const std::string& field_name = tuple.repeated_field();
        // get the field descriptor by user input message(*msg), and field name.

        const FieldDescriptor* field_dsp = GetFieldDescriptor(*msg, field_name);

        // Get the message descriptor of the repeated filed.
        const Descriptor* tmp_dsp = field_dsp->message_type();

        // User could select one/more sub-field as the Map field.
        // if the map key is a single sub-field.
        if (tuple.key_field_size() == 1) {
          const std::string& key_field_name = tuple.key_field(0);

          if (tmp_dsp->FindFieldByName(key_field_name) != nullptr) {
            const FieldDescriptor* key_field = tmp_dsp->FindFieldByName(key_field_name);

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

          for (int j = 0; j < tuple.key_field_size(); ++j) {
            const std::string& key_field_name = tuple.key_field(j);

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



    /*************************************************************
     *
     * Check the repeated field be treated as a map if [key] field don't appear at the same index.
     *
     *************************************************************/

    if (diff_request->has_map_compare_not_same_index()){
      const DifferentialService::MapCompareNotSameIndex& map_not_same_index = diff_request->map_compare_not_same_index();

      // 1) Get the repeated field name. (TestEmployee.exam_score)
      const std::string& field_name = map_not_same_index.repeated_field();
      // Get the field descriptor for the repeated field.
      const FieldDescriptor* field_descriptor = GetFieldDescriptor(*msg, field_name);

      // Check the field descriptor is not nullptr.
      if (field_descriptor != nullptr){

        // 2) Get the first key field name.(TestEmployee.exam_score.first_exam)
        const std::string& first_key_field_name = map_not_same_index.first_key_field();
        // Get the field descriptor for the first key field.
        const FieldDescriptor* first_key_field_descriptor = GetFieldDescriptor(*msg, first_key_field_name);


        // 3) Get the second key field name.(TestEmployee.exam_score.second_exam)
        const std::string& second_key_field_name = map_not_same_index.second_key_field();
        // Get the field descriptor for the second key field.
        const FieldDescriptor* second_key_field_descriptor = GetFieldDescriptor(*msg, second_key_field_name);

        // 4) Check the key field is not nullptr.
        if (first_key_field_descriptor != nullptr && second_key_field_descriptor != nullptr){
          const MessageDifferencer::MapKeyComparator* map_key_comparator = new KeyComparatorImpl(first_key_field_descriptor, second_key_field_descriptor);

          // Add the comparator in to the differencer.
          differencer.TreatAsMapUsingKeyComparator(field_descriptor, map_key_comparator);
        }
      }
    }

    /*******************************************************
     *
     *  Check the float point number comparison
     *
     *******************************************************/
    // if float_comparison is set
    if (diff_request->has_float_num_comparison()) {
      const DifferentialService::FloatNumComparison& fraction_Margin =
          diff_request->float_num_comparison();

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


    /*******************************************************
     *
     * Compare and output the result.
     *
     *******************************************************/
    // Output string for compare result.
    std::string diff_result;
    differencer.ReportDifferencesToString(&diff_result);
    bool diff_flag = differencer.Compare(*msg_base, *msg_test);

    // Set the compare result and return to the user.
    if (diff_flag) {
      diff_response->set_result("SAME");
    }
    else {
      diff_response->set_result(diff_result);
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