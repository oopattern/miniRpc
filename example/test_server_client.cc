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
    static void ClientThread(void);
};


static int32_t s_client_thread = 3000;

void CTestNet::TestSendMessage(CTcpConnection* conn_ptr)
{
    char send_buf[32];
    ::memset(send_buf, 'a', sizeof(send_buf));
    conn_ptr->Send(send_buf, sizeof(send_buf));
}

void CTestNet::TestFlowMessage(CTcpConnection* conn_ptr)
{
    CEventLoop* loop = conn_ptr->GetLoop();
    loop->RunAfter(2000, std::bind(&TestSendMessage, conn_ptr));
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

void CTestNet::ClientThread(void)
{
    TEndPoint server_addr;
    snprintf(server_addr.ip, sizeof(server_addr.ip), "192.168.201.75");
    server_addr.port = 8888;
    printf("tcp client start connect: %s:%d\n", server_addr.ip, server_addr.port);

    CEventLoop loop;
    CTcpClient client(&loop);
    //client.SetConnectionCallback(std::bind(&TestFlowMessage, _1));
    //client.SetMessageCallback(std::bind(&ClientMessage, _1, _2, _3));    
    client.Connect(server_addr);

    loop.Loop();
}

void CTestNet::TestTcpClient(void)
{
    CEventLoop loop;

    CThreadPool pool(s_client_thread, std::bind(&CTestNet::ClientThread));
    pool.StartAll();
    
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

