// Generated by the gRPC C++ plugin.
// If you make any local change, they will be lost.
// source: differential_service.proto

#include "differential_service.pb.h"
#include "differential_service.grpc.pb.h"

#include <functional>
#include <grpcpp/impl/codegen/async_stream.h>
#include <grpcpp/impl/codegen/async_unary_call.h>
#include <grpcpp/impl/codegen/channel_interface.h>
#include <grpcpp/impl/codegen/client_unary_call.h>
#include <grpcpp/impl/codegen/client_callback.h>
#include <grpcpp/impl/codegen/message_allocator.h>
#include <grpcpp/impl/codegen/method_handler.h>
#include <grpcpp/impl/codegen/rpc_service_method.h>
#include <grpcpp/impl/codegen/server_callback.h>
#include <grpcpp/impl/codegen/server_callback_handlers.h>
#include <grpcpp/impl/codegen/server_context.h>
#include <grpcpp/impl/codegen/service_type.h>
#include <grpcpp/impl/codegen/sync_stream.h>
namespace DifferentialService {

static const char* ServerDifferential_method_names[] = {
  "/DifferentialService.ServerDifferential/GetConnect",
  "/DifferentialService.ServerDifferential/CompareInputMessages",
};

std::unique_ptr< ServerDifferential::Stub> ServerDifferential::NewStub(const std::shared_ptr< ::grpc::ChannelInterface>& channel, const ::grpc::StubOptions& options) {
  (void)options;
  std::unique_ptr< ServerDifferential::Stub> stub(new ServerDifferential::Stub(channel));
  return stub;
}

ServerDifferential::Stub::Stub(const std::shared_ptr< ::grpc::ChannelInterface>& channel)
  : channel_(channel), rpcmethod_GetConnect_(ServerDifferential_method_names[0], ::grpc::internal::RpcMethod::NORMAL_RPC, channel)
  , rpcmethod_CompareInputMessages_(ServerDifferential_method_names[1], ::grpc::internal::RpcMethod::NORMAL_RPC, channel)
  {}

::grpc::Status ServerDifferential::Stub::GetConnect(::grpc::ClientContext* context, const ::DifferentialService::MsgRequest& request, ::DifferentialService::MsgReply* response) {
  return ::grpc::internal::BlockingUnaryCall(channel_.get(), rpcmethod_GetConnect_, context, request, response);
}

void ServerDifferential::Stub::experimental_async::GetConnect(::grpc::ClientContext* context, const ::DifferentialService::MsgRequest* request, ::DifferentialService::MsgReply* response, std::function<void(::grpc::Status)> f) {
  ::grpc_impl::internal::CallbackUnaryCall(stub_->channel_.get(), stub_->rpcmethod_GetConnect_, context, request, response, std::move(f));
}

void ServerDifferential::Stub::experimental_async::GetConnect(::grpc::ClientContext* context, const ::grpc::ByteBuffer* request, ::DifferentialService::MsgReply* response, std::function<void(::grpc::Status)> f) {
  ::grpc_impl::internal::CallbackUnaryCall(stub_->channel_.get(), stub_->rpcmethod_GetConnect_, context, request, response, std::move(f));
}

void ServerDifferential::Stub::experimental_async::GetConnect(::grpc::ClientContext* context, const ::DifferentialService::MsgRequest* request, ::DifferentialService::MsgReply* response, ::grpc::experimental::ClientUnaryReactor* reactor) {
  ::grpc_impl::internal::ClientCallbackUnaryFactory::Create(stub_->channel_.get(), stub_->rpcmethod_GetConnect_, context, request, response, reactor);
}

void ServerDifferential::Stub::experimental_async::GetConnect(::grpc::ClientContext* context, const ::grpc::ByteBuffer* request, ::DifferentialService::MsgReply* response, ::grpc::experimental::ClientUnaryReactor* reactor) {
  ::grpc_impl::internal::ClientCallbackUnaryFactory::Create(stub_->channel_.get(), stub_->rpcmethod_GetConnect_, context, request, response, reactor);
}

::grpc::ClientAsyncResponseReader< ::DifferentialService::MsgReply>* ServerDifferential::Stub::AsyncGetConnectRaw(::grpc::ClientContext* context, const ::DifferentialService::MsgRequest& request, ::grpc::CompletionQueue* cq) {
  return ::grpc_impl::internal::ClientAsyncResponseReaderFactory< ::DifferentialService::MsgReply>::Create(channel_.get(), cq, rpcmethod_GetConnect_, context, request, true);
}

::grpc::ClientAsyncResponseReader< ::DifferentialService::MsgReply>* ServerDifferential::Stub::PrepareAsyncGetConnectRaw(::grpc::ClientContext* context, const ::DifferentialService::MsgRequest& request, ::grpc::CompletionQueue* cq) {
  return ::grpc_impl::internal::ClientAsyncResponseReaderFactory< ::DifferentialService::MsgReply>::Create(channel_.get(), cq, rpcmethod_GetConnect_, context, request, false);
}

::grpc::Status ServerDifferential::Stub::CompareInputMessages(::grpc::ClientContext* context, const ::DifferentialService::DiffRequest& request, ::DifferentialService::DiffResponse* response) {
  return ::grpc::internal::BlockingUnaryCall(channel_.get(), rpcmethod_CompareInputMessages_, context, request, response);
}

void ServerDifferential::Stub::experimental_async::CompareInputMessages(::grpc::ClientContext* context, const ::DifferentialService::DiffRequest* request, ::DifferentialService::DiffResponse* response, std::function<void(::grpc::Status)> f) {
  ::grpc_impl::internal::CallbackUnaryCall(stub_->channel_.get(), stub_->rpcmethod_CompareInputMessages_, context, request, response, std::move(f));
}

void ServerDifferential::Stub::experimental_async::CompareInputMessages(::grpc::ClientContext* context, const ::grpc::ByteBuffer* request, ::DifferentialService::DiffResponse* response, std::function<void(::grpc::Status)> f) {
  ::grpc_impl::internal::CallbackUnaryCall(stub_->channel_.get(), stub_->rpcmethod_CompareInputMessages_, context, request, response, std::move(f));
}

void ServerDifferential::Stub::experimental_async::CompareInputMessages(::grpc::ClientContext* context, const ::DifferentialService::DiffRequest* request, ::DifferentialService::DiffResponse* response, ::grpc::experimental::ClientUnaryReactor* reactor) {
  ::grpc_impl::internal::ClientCallbackUnaryFactory::Create(stub_->channel_.get(), stub_->rpcmethod_CompareInputMessages_, context, request, response, reactor);
}

void ServerDifferential::Stub::experimental_async::CompareInputMessages(::grpc::ClientContext* context, const ::grpc::ByteBuffer* request, ::DifferentialService::DiffResponse* response, ::grpc::experimental::ClientUnaryReactor* reactor) {
  ::grpc_impl::internal::ClientCallbackUnaryFactory::Create(stub_->channel_.get(), stub_->rpcmethod_CompareInputMessages_, context, request, response, reactor);
}

::grpc::ClientAsyncResponseReader< ::DifferentialService::DiffResponse>* ServerDifferential::Stub::AsyncCompareInputMessagesRaw(::grpc::ClientContext* context, const ::DifferentialService::DiffRequest& request, ::grpc::CompletionQueue* cq) {
  return ::grpc_impl::internal::ClientAsyncResponseReaderFactory< ::DifferentialService::DiffResponse>::Create(channel_.get(), cq, rpcmethod_CompareInputMessages_, context, request, true);
}

::grpc::ClientAsyncResponseReader< ::DifferentialService::DiffResponse>* ServerDifferential::Stub::PrepareAsyncCompareInputMessagesRaw(::grpc::ClientContext* context, const ::DifferentialService::DiffRequest& request, ::grpc::CompletionQueue* cq) {
  return ::grpc_impl::internal::ClientAsyncResponseReaderFactory< ::DifferentialService::DiffResponse>::Create(channel_.get(), cq, rpcmethod_CompareInputMessages_, context, request, false);
}

ServerDifferential::Service::Service() {
  AddMethod(new ::grpc::internal::RpcServiceMethod(
      ServerDifferential_method_names[0],
      ::grpc::internal::RpcMethod::NORMAL_RPC,
      new ::grpc::internal::RpcMethodHandler< ServerDifferential::Service, ::DifferentialService::MsgRequest, ::DifferentialService::MsgReply>(
          [](ServerDifferential::Service* service,
             ::grpc_impl::ServerContext* ctx,
             const ::DifferentialService::MsgRequest* req,
             ::DifferentialService::MsgReply* resp) {
               return service->GetConnect(ctx, req, resp);
             }, this)));
  AddMethod(new ::grpc::internal::RpcServiceMethod(
      ServerDifferential_method_names[1],
      ::grpc::internal::RpcMethod::NORMAL_RPC,
      new ::grpc::internal::RpcMethodHandler< ServerDifferential::Service, ::DifferentialService::DiffRequest, ::DifferentialService::DiffResponse>(
          [](ServerDifferential::Service* service,
             ::grpc_impl::ServerContext* ctx,
             const ::DifferentialService::DiffRequest* req,
             ::DifferentialService::DiffResponse* resp) {
               return service->CompareInputMessages(ctx, req, resp);
             }, this)));
}

ServerDifferential::Service::~Service() {
}

::grpc::Status ServerDifferential::Service::GetConnect(::grpc::ServerContext* context, const ::DifferentialService::MsgRequest* request, ::DifferentialService::MsgReply* response) {
  (void) context;
  (void) request;
  (void) response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}

::grpc::Status ServerDifferential::Service::CompareInputMessages(::grpc::ServerContext* context, const ::DifferentialService::DiffRequest* request, ::DifferentialService::DiffResponse* response) {
  (void) context;
  (void) request;
  (void) response;
  return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "");
}


}  // namespace DifferentialService

