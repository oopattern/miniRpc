#include <stdio.h>

CTcpServer::CTcpServer(CEventLoop* loop, TEndPoint listen_addr)
    : m_acceptor(new CAcceptor(loop, listen_addr))
{

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
