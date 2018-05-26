#include <stdio.h>
#include "acceptor.h"
#include "event_loop.h"
#include "tcp_connection.h"
#include "tcp_server.h"


CTcpServer::CTcpServer(CEventLoop* loop, TEndPoint& listen_addr)
    : m_loop(loop),
      m_acceptor(new CAcceptor(loop, listen_addr))
{
    m_acceptor->SetNewConnectionCallback(std::bind(&CTcpServer::NewConnection, this, _1));
}

CTcpServer::~CTcpServer()
{
    // close connection
    // delete acceptor
}

void CTcpServer::Start(void)
{
    m_acceptor->Listen();   
}

void CTcpServer::NewConnection(int32_t connfd)
{
    // find pthread to handle this connection
    printf("build new connection\n");

    // build connection
    std::string name = std::to_string(connfd);
    CTcpConnection* conn_ptr = new CTcpConnection(m_loop, connfd);

    // attach message callback
    conn_ptr->SetMessageCallback(m_message_callback);
    
    m_connection_map[name] = conn_ptr;
}

int32_t CTcpServer::AddService(google::protobuf::Service* service)
{
    if (NULL == service)
    {
        printf("register service is NULL error\n");
        return ERROR;
    }

    const google::protobuf::ServiceDescriptor* sd = service->GetDescriptor();

    if (0 == sd->method_count())
    {
        printf("service: %s no methods error\n", sd->full_name());
        return ERROR;
    }

    for (int32_t i = 0; i < sd->method_count(); ++i)
    {
        // key: service_name : method_name
        const google::protobuf::MethodDescriptor* md = sd->method(i);
        std::string key = sd->full_name() + ":" + md->full_name();
        
        if (m_method_map.find(key) != m_method_map.end())
        {
            printf("service: %s method: %s already exists error\n", sd->full_name(), md->full_name());
            return ERROR;
        }

        // register service and method
        TMethodProperty mp;
        mp.service = service;
        mp.method = md;
        m_method_map[key] = mp;
        printf("Tcp Server register service: %s success\n", key.c_str());
    }

    return OK;
}


