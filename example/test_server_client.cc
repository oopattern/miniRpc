#include <stdio.h>
#include "../net/tcp_client.h"
#include "../net/tcp_server.h"
#include "../net/tcp_connection.h"
#include "../net/event_loop.h"



class CTestNet
{
public:
    static void ServerMessage(CTcpConnection* conn_ptr, char* buf, int32_t len);
    static void ClientMessage(CTcpConnection* conn_ptr, char* buf, int32_t len);
    static void TestFlowMessage(CTcpConnection* conn_ptr);
    static void TestSendMessage(CTcpConnection* conn_ptr);
    static void TestTcpClient(void);
    static void TestTcpServer(void);
};


void CTestNet::TestSendMessage(CTcpConnection* conn_ptr)
{
    char send_buf[64*1024];
    ::memset(send_buf, 'a', sizeof(send_buf));
    conn_ptr->Send(send_buf, sizeof(send_buf));
}

void CTestNet::TestFlowMessage(CTcpConnection* conn_ptr)
{
    CEventLoop* loop = conn_ptr->GetLoop();
    loop->RunEvery(2000, std::bind(&TestSendMessage, conn_ptr));
}

void CTestNet::ServerMessage(CTcpConnection* conn_ptr, char* buf, int32_t len)
{
    if (len > 0)
    {
        buf[len] = '\0';
        printf("server receive: %s\n", buf);
    }
}

void CTestNet::ClientMessage(CTcpConnection* conn_ptr, char* buf, int32_t len)
{
    if (len > 0)
    {
        buf[len] = '\0';
        printf("client receive: %s\n", buf);
    }
}

void CTestNet::TestTcpClient(void)
{
    TEndPoint server_addr;
    snprintf(server_addr.ip, sizeof(server_addr.ip), "127.0.0.1");
    server_addr.port = 8888;
    printf("tcp client start connect: %s:%d\n", server_addr.ip, server_addr.port);

    CEventLoop loop;
    CTcpClient client(&loop);
    client.SetConnectionCallback(std::bind(&TestFlowMessage, _1));
    client.SetMessageCallback(std::bind(&ClientMessage, _1, _2, _3));    
    client.Connect(server_addr);
    loop.Loop();
}

void CTestNet::TestTcpServer(void)
{
    TEndPoint  listen_addr;
    snprintf(listen_addr.ip, sizeof(listen_addr.ip), "127.0.0.1");
    listen_addr.port = 8888;
    printf("tcp server start listen: %s:%d\n", listen_addr.ip, listen_addr.port);
    
    CEventLoop loop;
    CTcpServer server(&loop, listen_addr);
    server.SetMessageCallback(std::bind(&ServerMessage, _1, _2, _3));
    server.Start();
    loop.Loop();
}

