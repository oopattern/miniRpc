#include <stdio.h>
#include <google/protobuf/descriptor.h> // MethodDescriptor
#include <google/protobuf/message.h>    // Message
#include "../third_party/libco/co_routine.h"
#include "../third_party/libco/co_routine_inner.h"
#include "../net/tcp_connection.h"
#include "std_rpc_meta.pb.h"
#include "rpc_channel.h"


void CRpcChannel::CallMethod(const google::protobuf::MethodDescriptor* method,
                             google::protobuf::RpcController* cntl,
                             const google::protobuf::Message* request,
                             google::protobuf::Message* response,
                             google::protobuf::Closure* done)
{
    std::string service_name = method->service()->name();
    std::string method_name = method->name();

    RpcMeta meta;
    RpcRequestMeta* request_meta = meta.mutable_request();
    request_meta->set_service_name(service_name);
    request_meta->set_method_name(method_name);
    
    std::string payload;
    request->SerializeToString(&payload);
    meta.set_payload(payload);

    std::string rpc_send;
    meta.SerializeToString(&rpc_send);

    m_connection->Send(rpc_send.c_str(), rpc_send.size());   

    // coroutine suspend and wait for recv message
    // TODO: code not finish... : should add timeout to check out
    m_connection->YieldCoroutine();

    // recv data in rpc_call
    TRpcCall rpc_call;
    m_connection->GetRpcCall(rpc_call);
    if ((NULL == rpc_call.recv_buf) || (0 >= rpc_call.recv_len))
    {
        printf("rpc client call method recv none error\n");
        return;
    }

    //printf("rpc client recv msg, coroutine resume\n");

#if 0
    // send to server and wait for reply until timeout ms     
    int32_t timeout_ms = 0;
    char recv_buf[1024*10];
    int32_t recv_len = m_connection->RpcSendRecv(rpc_send.c_str(), rpc_send.size(), recv_buf, sizeof(recv_buf), timeout_ms);
    if (recv_len <= 0)
    {
        printf("Rpc call method send or recv error\n");
        return;
    }
#endif    
    
    // analysis from response, finish rpc call
    RpcMeta back_meta;
    std::string rpc_recv; 
    rpc_recv.assign(rpc_call.recv_buf, rpc_call.recv_len);
    //back_meta.ParseFromArray(recv_buf, recv_len);
    back_meta.ParseFromString(rpc_recv);
    RpcResponseMeta* response_meta = back_meta.mutable_response();
    int32_t error_code = response_meta->error_code();
    if (0 == error_code)
    {
        std::string back_payload = back_meta.payload();
        //response->ParseFromArray(back_payload.c_str(), back_payload.size());
        response->ParseFromString(back_payload);
        //printf("client Rpc call success, welcome to finish\n");
    }
}
