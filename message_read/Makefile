HOST_SYSTEM = $(shell uname | cut -f 1 -d_)
SYSTEM ?= $(HOST_SYSTEM)
CXX = g++
CPPFLAGS += `pkg-config --cflags protobuf grpc`
CXXFLAGS += -std=c++11
ifeq ($(SYSTEM),Darwin)
LDFLAGS += -L/usr/local/lib `pkg-config --libs protobuf grpc++`\
					 -pthread\
           -lgrpc++_reflection\
           -ldl
else
LDFLAGS += -L/usr/local/lib `pkg-config --libs protobuf grpc++`\
           -pthread\
           -Wl,--no-as-needed -lgrpc++_reflection -Wl,--as-needed\
           -ldl
endif
PROTOC = protoc
GRPC_CPP_PLUGIN = grpc_cpp_plugin
GRPC_CPP_PLUGIN_PATH ?= `which $(GRPC_CPP_PLUGIN)`

PROTOS_PATH = ../../protos

vpath %.proto $(PROTOS_PATH)

all: message_read_cpp

message_read_cpp: message_read.pb.o message_read.grpc.pb.o message_service.grpc.pb.o message_service.pb.o message_read.o
	$(CXX) $^ $(LDFLAGS) -o $@

#write_differtest_cpp: differ_test.pb.o write_differtest.o
#	$(CXX) $^ $(LDFLAGS) -o $@
#
#read_differtest_cpp: differ_test.pb.o read_differtest.o
#	$(CXX) $^ $(LDFLAGS) -o $@
#
#write_file_dsp_cpp: differ_test.pb.o write_file_descriptor_proto.o
#	$(CXX) $^ $(LDFLAGS) -o $@
#
#read_file_dsp_cpp: differ_test.pb.o read_file_descriptor_proto.o
#	$(CXX) $^ $(LDFLAGS) -o $@

%.grpc.pb.cc: %.proto
	$(PROTOC) -I $(PROTOS_PATH) --grpc_out=. --plugin=protoc-gen-grpc=$(GRPC_CPP_PLUGIN_PATH) $<

%.pb.cc: %.proto
	$(PROTOC) -I $(PROTOS_PATH) --cpp_out=. $<

clean:
	rm -f *.o *.pb.cc *.pb.h message_read_cpp