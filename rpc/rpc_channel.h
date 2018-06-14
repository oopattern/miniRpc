#ifndef __RPC_CHANNEL_H
#define __RPC_CHANNEL_H

#include <stdio.h>
#include "../net/net_common.h"


typedef enum 
{
    RPC_SUC         = 0,
    RPC_TIMEOUT_ERR = -1,
    RPC_SEQ_ERR     = -2,
    RPC_SERVICE_NOT_FOUND = -3,
    RPC_OTHER_ERR   = -4,
} RpcResult;


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
    static const int32_t RPC_TIMEOUT_MS = 5000;
    CTcpConnection* m_connection;
};

class CRpcCntl : public google::protobuf::RpcController
{
public:
    CRpcCntl() 
    {
        m_error_code = 0;
    }
    virtual ~CRpcCntl() {}

    bool Failed(void) const 
    {
        return (m_error_code != 0);
    }

    void SetFailed(int32_t error_code)
    {
        printf("rpc call control happen error code = %d\n", error_code);
        m_error_code = error_code;
    }

    virtual void Reset()
    {
        
    }

    virtual std::string ErrorText() const
    {
        
    }

    virtual void StartCancel()
    {
    
    }

    virtual void SetFailed(const string& reason)
    {
    
    }

    virtual bool IsCanceled() const
    {
    
    }

    virtual void NotifyOnCancel(google::protobuf::Closure* callback)
    {
    
    }

private:
    int32_t m_error_code;
};

#endif


