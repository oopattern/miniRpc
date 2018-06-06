#include <stdio.h>
#include <sys/socket.h> // socket, bind, connect, listen, accept, send, recv
#include <arpa/inet.h>  // htonl, ntohl
#include <unistd.h>     // read, close
#include <string.h>     // strerror
#include <errno.h>      // errno
#include "channel.h"
#include "event_loop.h"
#include "buffer.h"
#include "../rpc/std_rpc_meta.pb.h"
#include "../rpc/rpc_channel.h"
#include "../rpc/rpc_coroutine.h"
#include "tcp_server.h"
#include "packet_codec.h"
#include "tcp_connection.h"


// use RPC
#define USE_RPC 1


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

void CTcpConnection::GetRpcCall(TRpcCall& rpc_call)
{
    rpc_call = m_rpc_call;
}

int32_t CTcpConnection::CreateCoroutine(void* (*routine)(void*), void* arg)
{
    CRpcCoroutine* rpc_co = new CRpcCoroutine();

    int32_t co_id = rpc_co->Create(routine, arg);
    if (co_id < 0)
    {
        printf("tcp connection create coroutine error\n");
        return ERROR;
    }

    // register new coroutine
    m_rpc_call.rpc_co = rpc_co;
    return OK;
}

int32_t CTcpConnection::ResumeCoroutine(void)
{
    if (NULL == m_rpc_call.rpc_co)
    {
        printf("resume tcp connection no coroutine error\n");
        return ERROR;
    }

    m_rpc_call.rpc_co->Resume();
    return OK;
}

int32_t CTcpConnection::YieldCoroutine(void)
{
    if (NULL == m_rpc_call.rpc_co)
    {
        printf("yield tcp connection no coroutine error\n");
        return ERROR;
    }

    m_rpc_call.rpc_co->Yield();
    return OK;
}

void CTcpConnection::RpcClientMsg(const char* recv_buf, int32_t recv_len)
{
    if (NULL == m_rpc_call.rpc_co)
    {
        printf("tcp client no coroutine error\n");
        return;
    }

    // packet format: 
    const char* msg_buf = recv_buf + 5;
    int32_t msg_len = ::ntohl(*(int32_t*)(recv_buf + 1));
    assert(msg_len < recv_len);

    //printf("HandleRead client recv msg, need to resume coroutine\n");
    m_rpc_call.recv_buf = msg_buf;
    m_rpc_call.recv_len = msg_len;
    m_rpc_call.rpc_co->Resume();

    // TODO: code not finish, need to destroy coroutine
    m_rpc_call.rpc_co = NULL;
}

void CTcpConnection::RpcServerMsg(const char* recv_buf, int32_t recv_len)
{
    if (NULL == m_server)
    {
        printf("this is a client connection read callback, do not finish\n");
        return;
    }

    // analysis and parse packet
    const char* msg_buf = recv_buf + 5;
    int32_t msg_len = ::ntohl(*(int32_t*)(recv_buf + 1));
    assert(msg_len < recv_len);

    RpcMeta rpc_meta;
    std::string rpc_recv;
    rpc_recv.assign(msg_buf, msg_len);
    //rpc_meta.ParseFromArray(m_rbuf->Data(), m_rbuf->Remain());
    rpc_meta.ParseFromString(rpc_recv);

    // rpc_quest_meta to find rpc method service
    const RpcRequestMeta& request_meta = rpc_meta.request();
    std::string service_name = request_meta.service_name();
    std::string method_name = request_meta.method_name();
    std::string full_name = service_name + ":" + method_name;
    //printf("rpc service_name: %s, method_name: %s\n", service_name.c_str(), method_name.c_str());

    TMethodProperty method_property;
    if (OK != m_server->FindService(full_name, method_property))
    {
        printf("Rpc Server message callback find service error\n");
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
    //req_base->ParseFromArray(payload.c_str(), payload.size());
    req_base->ParseFromString(payload);

    // RPC call
    service->CallMethod(method, NULL, req_base, res_base, NULL);

    // build response to client
    std::string back_payload;
    res_base->SerializeToString(&back_payload);

    RpcMeta rpc_back_meta;
    RpcResponseMeta* response_meta = rpc_back_meta.mutable_response();
    response_meta->set_error_code(0);
    rpc_back_meta.set_payload(back_payload);

    char send_buf[PACKET_BUF_SIZE];
    int32_t send_len = PACKET_BUF_SIZE;
    CPacketCodec::BuildPacket(&rpc_back_meta, send_buf, send_len);

    // send response to client
    Send(send_buf, send_len);
}

void CTcpConnection::HandleRead(void)
{
    //printf("need to read socket\n");
    char buf[1024*100];
    int32_t nread = 0;

    nread = ::read(m_channel->Fd(), buf, sizeof(buf));    
    if (nread > 0)
    {
        // just for test 
        //buf[nread] = '\0';
        //printf("HandleRead recv content: %s\n", buf);

        m_rbuf->Append(buf, nread);
        
        // check up complete packet
        int32_t packet_len = CPacketCodec::CheckPacket((char*)m_rbuf->Data(), m_rbuf->Remain());
        if (0 > packet_len)
        {
            m_rbuf->Clear();
            printf("tcp connection recv msg error, may need to close\n");
            return;
        }
        // packet not complete, may recv continue
        else if (0 == packet_len)
        {
            return;
        }

#if USE_RPC 
        // server recv message from connection
        if (m_server != NULL)
        {
            RpcServerMsg((char*)m_rbuf->Data(), packet_len);
        }
        // client recv message from connection
        else 
        {
            RpcClientMsg((char*)m_rbuf->Data(), packet_len);           
        }
#else 
        if (m_message_callback)
        {
            m_message_callback(this, (char*)m_rbuf->Data(), packet_len);
        }
#endif
        
        // skip already read_len
        m_rbuf->Skip(packet_len);
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

int32_t CTcpConnection::RpcSendRecv(const char* send_buf, 
                                    int32_t send_len, 
                                    char* recv_buf, 
                                    int32_t recv_max_size, 
                                    int32_t timeout_ms)
{
    // send packet to server until finish or fail
    int32_t nsend = 0;
    while (nsend < send_len)
    {
        int32_t n = ::send(m_channel->Fd(), send_buf + nsend, send_len - nsend, 0);
        if (n < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            
            if ((errno != EAGAIN) && (errno != EWOULDBLOCK))
            {
                printf("rpc send error: %s\n", ::strerror(errno));
                return ERROR;
            }
        }
        else
        {
            nsend += n;            
        }
    }

    // recv packet from server until timeout or fail
    int32_t nrecv = -1;
    while (nrecv <= 0)
    {
        nrecv = ::recv(m_channel->Fd(), recv_buf, recv_max_size, 0);
        if (nrecv < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            if (errno != EAGAIN && errno != EWOULDBLOCK)
            {
                printf("rpc recv error: %s\n", ::strerror(errno));
                return ERROR;
            }
        }
        else if (0 == nrecv)
        {
            printf("tcp server may shutdown read 0 error\n");
            return ERROR;
        }
    }

    return nrecv;
}


