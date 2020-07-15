//
// Created by jinhuangzheliu on 7/15/20.
//

#ifndef DIFFERENTIAL_SERVICE_CLIENT_UTIL_H
#define DIFFERENTIAL_SERVICE_CLIENT_UTIL_H

#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/message.h>
#include <grpcpp/grpcpp.h>

#include <iostream>
#include <memory>
#include <string>

#include "differential_client_lib/differential_service.grpc.pb.h"
#include "differential_client_lib/differential_test.grpc.pb.h"

using google::protobuf::Descriptor;
using google::protobuf::DescriptorPool;
using google::protobuf::DescriptorProto;
using google::protobuf::FieldDescriptor;
using google::protobuf::FileDescriptor;
using google::protobuf::FileDescriptorProto;
using google::protobuf::Message;

// using namespace from the differential service .proto file
using differentialservice::DiffMsgRequest;

// using namespace form the differential test .proto file.
using differential_test::company;
using differential_test::dependent_info;
using differential_test::education_info;
using differential_test::employee;
using differential_test::field_set;

/*
 *  This class contains various utilities to use the Differential Service.
 */

class Client_util {
 public:
  /*
   * Write the input messages to the request of differential service.
   * [Input Args]: 1) protobuf message from user.
   *               2) protobuf message from user.
   * [Return]: differential request message.
   */
  static DiffMsgRequest WriteMsgToDiffRequest(Message& msg_1, Message& msg_2);

  /*
   * This method is used to ignore one / more fields during the differential service.
   * [Input Args]: 1) the object of differential request message.
   *                        2) a string type vector saving the name of the field.
   * [Return]: void
   */
  static void IgnoreFields(DiffMsgRequest& diff_request,
                           std::vector<std::string>& field_list);

  /*
   * This method is used to only compare one / more fields during the differential service.
   * [Input Args]: 1) the object of differential request message.
   *               2) a string type vector saving the name of the field.
   * [Return]: void
   */

  static void CompareFields(DiffMsgRequest& diff_request,
                            std::vector<std::string>& field_list);

  /*
   * This method is used to ignore some specific fields by regular expression.
   * [Input Args]: 1) the object of differential request message.
   *               2) a string type regular expression.
   * [Return]: void
   */
  static void RegexCriteria(DiffMsgRequest* diff_request, std::string& regex);

  /*
   * This method is used to set the repeated field as list-based of set-based in
   * messages comparison [Input Args]: 1) the object of differential request
   * message. 2) a integer type flag (0/1) when flag passed by 0, treat repeated
   * field as list based comparison. when flag passed by 1, treat repeated field
   * as set based comparison. 3) the name of the repeated field. (string type)
   * [Return]: void
   */
  static void TreatRepeatedFieldAsListOrSet(DiffMsgRequest& diff_request,
                                            int flag, std::string& field_name);

  /*
   * This method is used to set the repeated field as Map-Value based in
   * messages comparison [Input Args]: 1) the object of differential request
   * message. 2) the repeated field name(string type). 3) a string type vector
   * saving the name of the sub-fields as the Map fields. [Return]: void
   */
  static void TreatRepeatedFieldAsMap(DiffMsgRequest& diff_request,
                                      std::string& field_name,
                                      std::vector<std::string>& sub_field_name);

  /*
   * This method is used to set the fraction and margin to compare the double /
   * float number [Input Args]: 1) the object of differential request message.
   *               2) fraction (double)
   *               3) maring(double)
   * [Return]: void
   */
  static void SetFractionAndMargin(DiffMsgRequest& diff_request,
                                   double fraction, double margin);
};

#endif  // DIFFERENTIAL_SERVICE_CLIENT_UTIL_H
