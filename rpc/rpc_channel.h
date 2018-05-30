#ifndef __RPC_CHANNEL_H
#define __RPC_CHANNEL_H

#include <stdio.h>
#include "../net/net_common.h"


class CTcpConnection;

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


