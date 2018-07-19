#include <stdio.h>
#include "tcp_connection.h"
#include "http_server.h"


CHttpServer::CHttpServer(CEventLoop* loop, TEndPoint& addr)
    : m_server(loop, addr)
{
    m_server.SetMessageCallback(std::bind(&CHttpServer::OnMessage, this, _1, _2, _3));
}

CHttpServer::~CHttpServer()
{

}

void CHttpServer::Start(void)
{
    m_server.Start();
}

void CHttpServer::OnMessage(CTcpConnection* conn_ptr, char* buf, int32_t len)
{    
    std::string request = std::string(buf, len);
    std::string response;
    m_http_callback(request, response);
    conn_ptr->Send(response.c_str(), response.size());    
    // use strace found, when write to socket, shutdown socket will get SIGPIPE
    // so it will kill process by default, we can ignore the signal when start with main
    // the right time to shutdown the socket is no more writing.
    //conn_ptr->ShutDown();
}
