#include <stdio.h>
#include <assert.h>
#include <google/protobuf/descriptor.h> // MethodDescriptor
#include <google/protobuf/message.h>    // Message
#include "../third_party/libco/co_routine.h"
#include "../third_party/libco/co_routine_inner.h"
#include "../net/tcp_connection.h"
#include "../net/packet_codec.h"
#include "../net/event_loop.h"
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
    assert((method != NULL) && (cntl != NULL) && (request != NULL) && (response != NULL));    
    CRpcCntl* rpc_cntl = (CRpcCntl*)cntl;

    std::string service_name = method->service()->name();
    std::string method_name = method->name();

    CRpcCoroutine* rpc_co = CRpcCoroutine::GetOwner();
    assert(rpc_co != NULL);
    int32_t coroutine_id = rpc_co->GetId();

    RpcMeta meta;
    RpcRequestMeta* request_meta = meta.mutable_request();
    request_meta->set_service_name(service_name);
    request_meta->set_method_name(method_name);
    request_meta->set_coroutine_id(coroutine_id);

    // build packet
    char send_buf[PACKET_BUF_SIZE];
    int32_t send_len = PACKET_BUF_SIZE;
    if (OK != CPacketCodec::BuildPacket(&meta, request, send_buf, send_len))
    {
        rpc_cntl->SetFailed(RPC_OTHER_ERR);
        printf("client call method build packet error\n");
        return;
    }

    m_connection->Send(send_buf, send_len);   
    CEventLoop* loop = m_connection->GetLoop();
    loop->RunAfter(RPC_TIMEOUT_MS, std::bind(&CTcpConnection::TimeoutCoroutine, m_connection, coroutine_id));

    // coroutine suspend and wait for recv message
    // TODO: code not finish... : should add timeout to check out
    rpc_co->Yield();

    // recv data in rpc_call
    TRpcCall rpc_call;
    rpc_co->GetRpcCall(rpc_call);

    // check out timeout first
    if (true == rpc_call.is_timeout)
    {
        rpc_cntl->SetFailed(RPC_TIMEOUT_ERR);
        printf("rpc client call method timeout error\n");
        return;
    }

    // next check out other thing
    if ((NULL == rpc_call.recv_buf) || (0 >= rpc_call.recv_len))
    {
        rpc_cntl->SetFailed(RPC_OTHER_ERR);
        printf("rpc client call method recv none error\n");
        return;
    }

    RpcMeta head_meta;
    if (OK != CPacketCodec::ParseHead(rpc_call.recv_buf, rpc_call.recv_len, &head_meta))
    {
        rpc_cntl->SetFailed(RPC_OTHER_ERR);
        printf("client call method parse head error\n");
        return;
    }
    
    RpcResponseMeta* response_meta = head_meta.mutable_response();
    int32_t error_code = response_meta->error_code();
    if (0 != error_code)
    {
        rpc_cntl->SetFailed(RPC_OTHER_ERR);
        printf("rpc response with error code = %d\n", error_code);
        return;
    }
    
    if (OK != CPacketCodec::ParseBody(rpc_call.recv_buf, rpc_call.recv_len, response))
    {
        rpc_cntl->SetFailed(RPC_OTHER_ERR);
        printf("client call method parse body error\n");
        return;
    }

    //printf("coroutine id = %d prepare to quit\n", coroutine_id);
}
