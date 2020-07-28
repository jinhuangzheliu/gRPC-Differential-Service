//
// Created by jinhuangzheliu on 7/15/20.
//

#include "client_util.h"


DiffRequest ClientUtil::WriteMsgToDiffRequest(const Message& msg_1,
                                              const Message& msg_2) {
  // Generate the differential request
  DiffRequest diff_request;

  // 1)Serialize Two message and write to the differential request.
  std::string serialized_first_string = msg_1.SerializeAsString();
  std::string serialized_second_string = msg_2.SerializeAsString();

  // write the serialize messages to differential request.
  diff_request.set_first_message(serialized_first_string);
  diff_request.set_second_message(serialized_second_string);

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
    std::string serialized_dependency = dependency_proto.SerializeAsString();

    // Add the serialized dependency to differential request.
    diff_request.add_file_descriptor_proto(serialized_dependency);
  }

  return diff_request;
}

void ClientUtil::IgnoreFields(DiffRequest* diff_request,
                              const std::vector<std::string>& field_list) {
  // Get the IgnoreCriteria that is defined by differential service.
  DifferentialService::IgnoreCriteria* ignore_criteria_ptr =
      diff_request->mutable_user_ignore();

  // Set the criteria flag (IGNORE)
  ignore_criteria_ptr->set_flag(
      DifferentialService::IgnoreCriteria_IgnoreFlag_FLAG_IGNORE);

  // Iterate and insert the IGNORE fields to the criteria.
  for (auto& i : field_list) {
    ignore_criteria_ptr->add_ignore_fields_list(i);
  }
}

void ClientUtil::CompareFields(DiffRequest* diff_request,
                               const std::vector<std::string>& field_list) {
  // Get the IgnoreCriteria that is defined by differential service.
  DifferentialService::IgnoreCriteria* ignore_criteria_ptr =
      diff_request->mutable_user_ignore();

  // Set the criteria flag (COMPARE)
  ignore_criteria_ptr->set_flag(
      DifferentialService::IgnoreCriteria_IgnoreFlag_FLAG_COMPARE);

  // Iterate and insert the COMPARE fields to the criteria.
  for (auto& i : field_list) {
    ignore_criteria_ptr->add_ignore_fields_list(i);
  }
}

void ClientUtil::RegexCriteria(DiffRequest* diff_request,
                               const std::string& regex) {
  // Get the IgnoreCriteria that is defined by differential service.
  DifferentialService::IgnoreCriteria* ignore_criteria_ptr =
      diff_request->mutable_user_ignore();

  // Set the regex
  ignore_criteria_ptr->set_regex(regex);
}

void ClientUtil::TreatRepeatedFieldAsListOrSet(DiffRequest* diff_request,
                                               const int flag,
                                               const std::string& field_name) {
  // Get the pointer of the Repeated Field Tuple, which defined in
  // differential_service.proto
  DifferentialService::RepeatedField* tuple_ptr =
      diff_request->add_repeated_field();

  // Set the treat as flag.
  if (flag == 0) {
    tuple_ptr->set_flag(
        DifferentialService::RepeatedField_TreatAsFlag_FLAG_LIST);
  } else if (flag == 1) {
    tuple_ptr->set_flag(
        DifferentialService::RepeatedField_TreatAsFlag_FLAG_SET);
  }
  // Set the field name.
  tuple_ptr->set_field_name(field_name);
}

void ClientUtil::TreatRepeatedFieldAsMap(DiffRequest* diff_request,
                                         const std::string& field_name,
                                         const std::vector<std::string>& sub_field_name) {
  // Get the pointer of the MapCompareTuple, which defined in
  // differntial_service.proto.
  DifferentialService::MapCompare* map_compare_ptr =
      diff_request->add_map_compare();

  // Set the repeated field name
  map_compare_ptr->set_repeated_field(field_name);

  // Set the map field (may multiple)
  for (auto& i : sub_field_name) {
    map_compare_ptr->add_key_field(i);
  }
}

void ClientUtil::SetFractionAndMargin(DiffRequest* diff_request,
                                      const double fraction,
                                      const double margin) {
  // get the pointer of Float Number Comparison
  DifferentialService::FloatNumComparison* fracAndMar_ptr =
      diff_request->mutable_float_num_comparison();
  fracAndMar_ptr->set_fraction(fraction);
  fracAndMar_ptr->set_margin(margin);
}