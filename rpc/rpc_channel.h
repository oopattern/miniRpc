#ifndef __RPC_CHANNEL_H
#define __RPC_CHANNEL_H

#include <stdio.h>
#include "../net/net_common.h"


class CTcpClient;

class CRpcChannel : public google::protobuf::RpcChannel
{
public:
    CRpcChannel(CTcpClient* client) 
        : m_client(client)
    {
        
    }
    virtual ~CRpcChannel() {}

    virtual void CallMethod(const google::protobuf::MethodDescriptor* method,
                            google::protobuf::RpcController* cntl,
                            const google::protobuf::Message* request,
                            google::protobuf::Message* response,
                            google::protobuf::Closure* done);
private:
    CTcpClient* m_client;        
};


class CRpcCntl : public google::protobuf::RpcController
{
public:
    CRpcCntl() {}
    virtual ~CRpcCntl() {}
};

#endif


