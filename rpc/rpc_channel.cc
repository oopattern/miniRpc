#include <stdio.h>
#include <assert.h>
#include <google/protobuf/descriptor.h> // MethodDescriptor
#include <google/protobuf/message.h>    // Message
#include "../third_party/libco/co_routine.h"
#include "../third_party/libco/co_routine_inner.h"
#include "../net/tcp_connection.h"
#include "../net/packet_codec.h"
#include "../base/public.h"
#include "std_rpc_meta.pb.h"
#include "rpc_coroutine.h"
#include "rpc_channel.h"


void CRpcChannel::CallMethod(const google::protobuf::MethodDescriptor* method,
                             google::protobuf::RpcController* cntl,
                             const google::protobuf::Message* request,
                             google::protobuf::Message* response,
                             google::protobuf::Closure* done)
{
    std::string service_name = method->service()->name();
    std::string method_name = method->name();

    CRpcCoroutine* rpc_co = CRpcCoroutine::GetOwner();
    assert(rpc_co != NULL);

    RpcMeta meta;
    RpcRequestMeta* request_meta = meta.mutable_request();
    request_meta->set_service_name(service_name);
    request_meta->set_method_name(method_name);
    request_meta->set_coroutine_id(rpc_co->GetId());

    // build packet
    char send_buf[PACKET_BUF_SIZE];
    int32_t send_len = PACKET_BUF_SIZE;
    CPacketCodec::BuildPacket(&meta, request, send_buf, send_len);

    m_connection->Send(send_buf, send_len);   

    // coroutine suspend and wait for recv message
    // TODO: code not finish... : should add timeout to check out
    rpc_co->Yield();

    // recv data in rpc_call
    TRpcCall rpc_call;
    rpc_co->GetRpcCall(rpc_call);
    if ((NULL == rpc_call.recv_buf) || (0 >= rpc_call.recv_len))
    {
        printf("rpc client call method recv none error\n");
        return;
    }

    RpcMeta head_meta;
    if (OK != CPacketCodec::ParseHead(rpc_call.recv_buf, rpc_call.recv_len, &head_meta))
    {
        printf("client call method parse head error\n");
        return;
    }
    
    RpcResponseMeta* response_meta = head_meta.mutable_response();
    int32_t error_code = response_meta->error_code();
    if (0 == error_code)
    {
        if (OK != CPacketCodec::ParseBody(rpc_call.recv_buf, rpc_call.recv_len, response))
        {
            printf("client call method parse body error\n");
            return;
        }
    }

    printf("coroutine id = %d prepare to quit\n", rpc_co->GetId());
}
