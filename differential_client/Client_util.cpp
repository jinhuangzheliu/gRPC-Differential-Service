//
// Created by jinhuangzheliu on 7/15/20.
//

#include "Client_util.h"


DiffMsgRequest Client_util::WriteMsgToDiffRequest(Message& msg_1, Message& msg_2)
/*
 * In this method, 1) the user input message will be serialized to string type of value
 *                 2) get the descriptor of user input message.
 *                 3) get the file descriptor proto and all its dependency then
 *                    serialized to string type of value.
 * all these object will be wrote into the differential request message and return.
 */
{
  // Generate the differential request
  DiffMsgRequest diff_request;

  // 1)Serialize Two message and write to the differential request.
  std::string msg_string_1 = msg_1.SerializeAsString();
  std::string msg_string_2 = msg_2.SerializeAsString();

  // write the serialize messages to differential request.
  diff_request.set_message_1(msg_string_1);
  diff_request.set_message_2(msg_string_2);

  // 2) Get the message Descriptor and write the name of descriptor to request.
  const Descriptor* descriptor = msg_1.GetDescriptor();
  diff_request.set_name_of_message_descriptor(descriptor->name());

  // 3) Get the file descriptor proto of the user message.
  const FileDescriptor* file_descriptor = descriptor->file();
  FileDescriptorProto file_descriptor_proto;
  file_descriptor->CopyTo(&file_descriptor_proto);

  // Serialized to string
  std::string string_file_dsp_proto;
  file_descriptor_proto.SerializeToString(&string_file_dsp_proto);

  // set the file descriptor proto and its dependency
  diff_request.add_file_descriptor_proto(string_file_dsp_proto);

  // Add the dependency of file descriptor
  // Because a .proto file may have multiple dependency so we need loop.
  for (int i = 0; i < file_descriptor->dependency_count(); i++) {
    // Get the dependency of a file descriptor.
    const FileDescriptor* dependency = file_descriptor->dependency(i);

    // Get the file descriptor proto.
    FileDescriptorProto dependency_proto;
    dependency->CopyTo(&dependency_proto);

    // Serialized field descriptor proto.
    std::string Serialized_dependency = dependency_proto.SerializeAsString();

    // Add the serialized dependency to differential request.
    diff_request.add_file_descriptor_proto(Serialized_dependency);
  }

  return diff_request;
}

void Client_util::IgnoreFields(DiffMsgRequest& diff_request,
                                 std::vector<std::string>& field_list)
{
  // Get the IgnoreCriteria that is defined by differential service.
  differentialservice::IgnoreCriteria* ignoreCriteria_ptr =
      diff_request.mutable_user_ignore();

  // Set the criteria flag (IGNORE)
  ignoreCriteria_ptr->set_flag(
      differentialservice::IgnoreCriteria_IgnoreFlag_FLAG_IGNORE);

  // Iterate and insert the IGNORE fields to the criteria.
  for (auto & i : field_list) {
    ignoreCriteria_ptr->add_ignore_fields_list(i);
  }
}

void Client_util::CompareFields(DiffMsgRequest& diff_request,
                                  std::vector<std::string>& field_list)
{
  // Get the IgnoreCriteria that is defined by differential service.
  differentialservice::IgnoreCriteria* ignoreCriteria_ptr =
      diff_request.mutable_user_ignore();

  // Set the criteria flag (COMPARE)
  ignoreCriteria_ptr->set_flag(
      differentialservice::IgnoreCriteria_IgnoreFlag_FLAG_COMPARE);

  // Iterate and insert the COMPARE fields to the criteria.
  for (auto & i : field_list) {
    ignoreCriteria_ptr->add_ignore_fields_list(i);
  }
}

void Client_util::RegexCriteria(DiffMsgRequest* diff_request,
                                  std::string& regex) {
//    log_message->mutable_user_ignore()->set_regex("^mes.*ress$");
  diff_request->mutable_user_ignore()->set_regex(regex);
}

void Client_util::TreatRepeatedFieldAsListOrSet(DiffMsgRequest& diff_request,
                                                  int flag, std::string& field_name)

{
  // Get the pointer of the Repeated Field Tuple, which defined in differential_service.proto
  differentialservice::RepeatedFieldTuple* tuple_ptr = diff_request.add_repeated_field();

  // Set the treat as flag.
  if (flag == 0){
    tuple_ptr->set_flag(differentialservice::RepeatedFieldTuple_TreatAsFlag_FLAG_LIST);
  }
  else if (flag == 1){
    tuple_ptr->set_flag(differentialservice::RepeatedFieldTuple_TreatAsFlag_FLAG_SET);
  }
  // Set the field name.
  tuple_ptr->set_field_name(field_name);
}

void Client_util::TreatRepeatedFieldAsMap(DiffMsgRequest& diff_request,
                                            std::string& field_name,
                                            std::vector<std::string>& sub_field_name)
{
  // Get the pointer of the MapCompareTuple, which defined in differntial_service.proto.
  differentialservice::MapCompareTuple* map_compare_ptr = diff_request.add_map_compare();

  // Set the repeated field name
  map_compare_ptr->set_name_of_repeated_field(field_name);

  // Set the map field (may multiple)
  for (auto & i : sub_field_name) {
    map_compare_ptr->add_name_of_sub_field(i);
  }
}


void Client_util::SetFractionAndMargin(DiffMsgRequest& diff_request,
                                         double fraction,
                                         double margin)
{
  differentialservice::FloatNumComparison* fracAndMar_ptr =
      diff_request.mutable_fraction_margin();
  fracAndMar_ptr->set_fraction(fraction);
  fracAndMar_ptr->set_margin(margin);
}