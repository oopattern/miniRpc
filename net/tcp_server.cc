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
