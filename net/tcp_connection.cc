#include <stdio.h>
#include <unistd.h> // read, close
#include <string.h> // strerror
#include <errno.h>  // errno
#include "channel.h"
#include "event_loop.h"
#include "buffer.h"
#include "../rpc/std_rpc_meta.pb.h"
#include "tcp_server.h"
#include "tcp_connection.h"


CTcpConnection::CTcpConnection(CEventLoop* loop, int32_t connfd, CTcpServer* server)
    : m_loop(loop),
      m_channel(new CChannel(loop, connfd)),
      m_server(server),
      m_rbuf(new CBuffer),
      m_wbuf(new CBuffer)
{
    m_channel->SetReadCallback(std::bind(&CTcpConnection::HandleRead, this));
    m_channel->SetWriteCallback(std::bind(&CTcpConnection::HandleWrite, this));
    m_channel->SetCloseCallback(std::bind(&CTcpConnection::HandleClose, this));

    // enable socket read
    m_channel->EnableRead();
}

void CTcpConnection::RpcMsgCallback(void)
{
    if (NULL == m_server)
    {
        printf("this is a client connection read callback, do not finish\n");
        return;
    }

    RpcMeta rpc_meta;
    rpc_meta.ParseFromArray(m_rbuf->Data(), m_rbuf->Remain());

    // rpc_quest_meta to find rpc method service
    const RpcRequestMeta& request_meta = rpc_meta.request();
    std::string service_name = request_meta.service_name();
    std::string method_name = request_meta.method_name();
    std::string full_name = service_name + ":" + method_name;
    printf("rpc service: %s method: %s\n", service_name.c_str(), method_name.c_str());

    TMethodProperty method_property;
    if (OK != m_server->FindService(full_name, method_property))
    {
        printf("find service error\n");
        return;
    }

    // rpc_payload include request from client
    //google::protobuf::RpcController* cntl;
    google::protobuf::Service* service = method_property.service;
    const google::protobuf::MethodDescriptor* method = method_property.method; 

    // create request and response from method
    google::protobuf::Message* req_base = service->GetRequestPrototype(method).New();
    google::protobuf::Message* res_base = service->GetResponsePrototype(method).New();    
    std::string payload = rpc_meta.payload();
    req_base->ParseFromArray(payload.c_str(), payload.size());

    // RPC call
    service->CallMethod(method, NULL, req_base, res_base, NULL);

    // send response to client
    std::string back_payload;
    res_base->SerializeToString(&back_payload);

    RpcMeta rpc_back_meta;
    RpcResponseMeta* response_meta = rpc_back_meta.mutable_response();
    response_meta->set_error_code(0);
    rpc_back_meta.set_payload(back_payload);

    std::string client_msg;
    rpc_back_meta.SerializeToString(&client_msg);
    Send(client_msg.c_str(), client_msg.size());

    printf("finish rpc call\n");
}

void CTcpConnection::HandleRead(void)
{
    //printf("need to read socket\n");
    char buf[1024*100];
    int32_t nread = 0;

    nread = ::read(m_channel->Fd(), buf, sizeof(buf));    
    if (nread > 0)
    {
        m_rbuf->Append(buf, nread);

#if USE_RPC 
        RpcMsgCallback();
#else 
        if (m_message_callback)
        {
            m_message_callback(this, (char*)m_rbuf->Data(), m_rbuf->Remain());
        }
#endif
        
        m_rbuf->Skip(nread);
    }
    else if (0 == nread)
    {
        HandleClose();
    }
}

void CTcpConnection::HandleWrite(void)
{
    //printf("prepare to write socket\n");

    if (! m_channel->IsWriteable())
    {
        printf("tcp connection channel down, no more writing\n");
        return;
    }

    if (m_wbuf->Remain() <= 0)
    {
        printf("tcp connection channel nothing to write error\n");
        return;
    }

    int32_t nwrite = ::write(m_channel->Fd(), m_wbuf->Data(), m_wbuf->Remain());
    if (nwrite > 0)
    {
        m_wbuf->Skip(nwrite);

        // finish write content, disable epoll writeable
        if (m_wbuf->Remain() <= 0)
        {
            m_channel->DisableWrite();
        }
    }
    else
    {
        printf("tcp connection write socket error: %s\n", ::strerror(errno));    
    }
}

void CTcpConnection::HandleClose(void)
{
    printf("prepare to close socket\n");
    ::exit(-1);
}

void CTcpConnection::Send(const char* buf, int32_t len)
{
    int32_t nwrite = 0;
    int32_t remaining = len;   

    // nothing in output queue, try to write directly
    if (m_channel->IsWriteable() && (0 == m_wbuf->Remain()))
    {
        nwrite = ::write(m_channel->Fd(), buf, len);
        if (nwrite > 0)
        {
            remaining = len - nwrite;
            m_wbuf->Skip(nwrite);            
        }
        else
        {
            printf("tcp connection write error: %s\n", ::strerror(errno));
            return;
        }
    }

    // handle rest content to write
    if (remaining > 0)
    {
        m_wbuf->Append(buf+nwrite, remaining);
        if (! m_channel->IsWriteable())
        {
            m_channel->EnableWrite();
        }
    }
}


