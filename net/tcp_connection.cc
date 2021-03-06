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
#define USE_RPC 0

CTcpConnection::CTcpConnection(CEventLoop* loop, int32_t connfd, CTcpServer* server)
    : m_loop(loop),
      m_channel(new CChannel(m_loop, connfd)),
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

int32_t CTcpConnection::RegisterCoroutine(int32_t coroutine_id, CRpcCoroutine* co)
{
    if ((coroutine_id <= 0) || (NULL == co))
    {
        printf("tcp connection coroutine invalid error\n");
        return ERROR;
    }

    assert(coroutine_id == co->GetId());
    m_coroutine_map[coroutine_id] = co;
    return OK;
}

int32_t CTcpConnection::DestroyCoroutine(int32_t coroutine_id)
{
    CoroutineMap::iterator it = m_coroutine_map.find(coroutine_id);
    if (it == m_coroutine_map.end())
    {
        printf("destroy coroutine id = %d error\n", coroutine_id);
        return ERROR;
    }

    CRpcCoroutine* co = it->second;
    assert((co != NULL) && (coroutine_id == co->GetId()));

    co->Release();
    delete co;
    m_coroutine_map.erase(coroutine_id);
    //printf("coroutine id = %d release successful\n", coroutine_id);

    return OK;    
}

int32_t CTcpConnection::TimeoutCoroutine(int32_t coroutine_id)
{
    CoroutineMap::iterator it = m_coroutine_map.find(coroutine_id);
    if (it == m_coroutine_map.end())
    {
        printf("timeout client can not find coroutine = %d\n", coroutine_id);
        return ERROR;
    }

    CRpcCoroutine* rpc_co = it->second;
    assert((rpc_co != NULL) && (coroutine_id == rpc_co->GetId()));

    TRpcCall rpc_call;
    rpc_call.recv_buf = NULL;
    rpc_call.recv_len = 0;
    rpc_call.is_timeout = true;

    rpc_co->SetRpcCall(rpc_call);
    rpc_co->Resume();

    DestroyCoroutine(coroutine_id);
}

void CTcpConnection::RpcClientMsg(const char* recv_buf, int32_t recv_len)
{
    if ((NULL == recv_buf) || (0 >= recv_len))
    {
        printf("tcp client msg error\n");
        return;
    }

    RpcMeta head_meta;
    if (OK != CPacketCodec::ParseHead(recv_buf, recv_len, &head_meta))
    {
        printf("client msg parse head error\n");
        return;
    }

    // find out coroutine id
    RpcResponseMeta* response_meta = head_meta.mutable_response();
    int32_t coroutine_id = response_meta->coroutine_id();

    CoroutineMap::iterator it = m_coroutine_map.find(coroutine_id);
    if (it == m_coroutine_map.end())
    {
        printf("rpc client msg can not find coroutine id=%d\n", coroutine_id);
        return;
    }

    CRpcCoroutine* rpc_co = it->second;
    assert((rpc_co != NULL) && (rpc_co->GetId() == coroutine_id));

    // set rpc call back
    TRpcCall rpc_call;
    rpc_call.recv_buf = recv_buf;
    rpc_call.recv_len = recv_len;
    rpc_call.is_timeout = false;

    // resume the coroutine
    rpc_co->SetRpcCall(rpc_call);
    rpc_co->Resume();

    // after finish coroutine, need to destroy it
    // TODO: timeout need to destroy it
    DestroyCoroutine(coroutine_id);
}

void CTcpConnection::RpcServerMsg(const char* recv_buf, int32_t recv_len)
{
    if (NULL == m_server)
    {
        printf("this is a client connection read callback, do not finish\n");
        return;
    }

    // parse packet head
    RpcMeta head_meta;
    if (OK != CPacketCodec::ParseHead(recv_buf, recv_len, &head_meta))
    {
        printf("rpc server msg parse head error\n");
        return;
    }
    
    // rpc_quest_meta to find rpc method service
    const RpcRequestMeta& request_meta = head_meta.request();
    int32_t coroutine_id = request_meta.coroutine_id();
    std::string service_name = request_meta.service_name();
    std::string method_name = request_meta.method_name();
    std::string full_name = service_name + ":" + method_name;

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

    // parse packet body
    if (OK != CPacketCodec::ParseBody(recv_buf, recv_len, req_base))
    {
        printf("rpc server msg parse body packet error\n");
        return;
    }

    // RPC call
    service->CallMethod(method, NULL, req_base, res_base, NULL);

    // build packet for response
    RpcMeta back_meta;
    RpcResponseMeta* response_meta = back_meta.mutable_response();
    response_meta->set_error_code(0);
    response_meta->set_coroutine_id(coroutine_id);

    char send_buf[PACKET_BUF_SIZE];
    int32_t send_len = PACKET_BUF_SIZE;
    if (OK != CPacketCodec::BuildPacket(&back_meta, res_base, send_buf, send_len))
    {
        printf("rpc server msg build packet error\n");
        return;
    }

    // delete msg
    delete req_base;
    delete res_base;
    
    // send response to client
    Send(send_buf, send_len);
}

void CTcpConnection::HandleRead(void)
{
    //printf("try to not read buf, check what will happen?\n");
    
    //printf("need to read socket\n");
    char buf[1024*100];
    int32_t nread = 0;

    nread = ::read(m_channel->Fd(), buf, sizeof(buf));    
    if (0 >= nread)
    {
        //printf("HandleRead happen error: %s\n", ::strerror(errno));
        HandleClose();
        return;
    }
    
    // handle all recv msg in loop
    m_rbuf->Append(buf, nread);
    while (m_rbuf->Remain() > 0)
    {
#if USE_RPC
        // check up complete packet
        int32_t packet_len = CPacketCodec::CheckPacket((char*)m_rbuf->Data(), m_rbuf->Remain());
        if (0 > packet_len)
        {
            m_rbuf->Clear();
            printf("tcp connection recv msg error, may need to close\n");
            HandleClose();
            return;
        }
        // packet not complete, may recv continue
        else if (0 == packet_len)
        {
            return;
        }

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
        int32_t packet_len = nread;
        if (m_message_callback)
        {
            m_message_callback(this, (char*)m_rbuf->Data(), packet_len);
        }
#endif
        
        // skip already read_len
        m_rbuf->Skip(packet_len);
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
        //printf("handle write nwrite = %d\n", nwrite);
    }
    else
    {
        printf("tcp connection write socket error: %s\n", ::strerror(errno));    
    }
}

void CTcpConnection::HandleClose(void)
{
    //printf("prepare to close socket\n");
    m_channel->DisableAll();
    m_channel->Remove();
    ::close(m_channel->Fd());
}

void CTcpConnection::ShutDown(void)
{
    //m_channel->DisableAll();
    //m_channel->Remove();
    ::shutdown(m_channel->Fd(), SHUT_WR);
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
        //printf("send remaining = %d\n", remaining);
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


