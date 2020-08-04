#include <google/protobuf/compiler/importer.h>
#include <google/protobuf/descriptor.h>
#include <google/protobuf/descriptor.pb.h>
#include <google/protobuf/dynamic_message.h>
#include <google/protobuf/message.h>
#include <grpcpp/grpcpp.h>
#include <time.h>
#include <bits/stdc++.h>
#include <stdio.h>
#include <math.h>


#include <fstream>
#include <iostream>
#include <memory>
#include <string>

#include "client_util.h"
#include "differential_client_lib/differential_service.grpc.pb.h"
#include "differential_client_lib/differential_test.grpc.pb.h"
#include "differential_service_client.h"

// using namespace form the differential test .proto file.
using google::protobuf::Message;
using google::protobuf::MessageFactory;
using google::protobuf::Reflection;
using google::protobuf::Descriptor;
using google::protobuf::DescriptorPool;
using google::protobuf::DescriptorProto;
using google::protobuf::FieldDescriptor;
using google::protobuf::FileDescriptor;
using google::protobuf::FileDescriptorProto;

using google::protobuf::Message;
using google::protobuf::MessageFactory;
using google::protobuf::Reflection;


using DifferentialTest::Company;
using DifferentialTest::EducationInfo;
using DifferentialTest::DependentInfo;
using DifferentialTest::ExamScore;
using DifferentialTest::TestEmployee;



int main(int argc, char* argv[]) {
  GOOGLE_PROTOBUF_VERIFY_VERSION;

  // Establish the gRPC channel with port number.
  std::string target_str;
  target_str = "0.0.0.0:50053";

  // Initial the service client instance.
  DifferentialServiceClient service_client;


  /*
   *  Write two message for testing.
   *  The .proto file of testing message is located in ../protos/differential_test.proto
   */

  for (int j = 1; j <= 1; ++j) {
    std::clock_t start, end;

    double num = pow(10.0, j);
    int num_1 = (int)num;
    std::cout << num_1 << std::endl;
    TestEmployee message_first;
    TestEmployee message_second;

    start = clock();


    for (int i = 0; i < num_1; ++i) {

      int suffix = i;
      std::string name_1 = "A";
      name_1 = name_1 + "_" + std::to_string(suffix);

      std::string name_2 = "B";
      name_2 = name_2 + "_" + std::to_string(suffix);

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

    DiffRequest diff_request = ClientUtil::WriteMsgToDiffRequest(message_first, message_second);


    // Receive the differential response.
    DiffResponse diff_response;

    // Request the differential service.
    StatusCode service_response_status = service_client.CompareInputMessages(
        diff_request, &diff_response, target_str);



    end = clock();

    if (service_response_status == StatusCode::OK){
      std::cout << "Differential Result: \n"
                << diff_response.result() << std::endl;
    }

    double time_taken = double(end - start) / double(CLOCKS_PER_SEC);
    std::cout << "Time taken is : " << std::fixed << time_taken << std::setprecision(5);
    std::cout << " sec \n"
              << "####################" << std::endl;


  }
  google::protobuf::ShutdownProtobufLibrary();
  return 0;
}
