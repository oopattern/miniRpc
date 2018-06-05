#ifndef __RPC_CHANNEL_H
#define __RPC_CHANNEL_H

#include <stdio.h>
#include "../net/net_common.h"


class CRpcCoroutine;
class CTcpConnection;

typedef struct TRpcCall
{
    CRpcCoroutine*  rpc_co;
    const char*     recv_buf;
    int32_t         recv_len;
    
    // default construct, rpc_co must init NULL
    TRpcCall() : rpc_co(NULL), recv_buf(NULL), recv_len(0) {}
} TRpcCall;

class CRpcChannel : public google::protobuf::RpcChannel
{
public:
    CRpcChannel(CTcpConnection* conn_ptr) 
        : m_connection(conn_ptr)
    {
        
    }

    virtual ~CRpcChannel() {}

    virtual void CallMethod(const google::protobuf::MethodDescriptor* method,
                            google::protobuf::RpcController* cntl,
                            const google::protobuf::Message* request,
                            google::protobuf::Message* response,
                            google::protobuf::Closure* done);

    int32_t RpcCall(const google::protobuf::Message* request,
                    google::protobuf::Message* response,
                    int32_t timeout_ms);

private:
    CTcpConnection* m_connection;
};


class CRpcCntl : public google::protobuf::RpcController
{
public:
    CRpcCntl() {}
    virtual ~CRpcCntl() {}
};

#endif


