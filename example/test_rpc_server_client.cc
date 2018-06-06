#include <stdio.h>
#include "../base/public.h"
#include "../rpc/rpc_service.h"
#include "../rpc/rpc_channel.h"
#include "../net/tcp_connection.h"


class CTestRpcNet
{
public:
    static void TestRpcClient(void);
    static void TestRpcServer(void);
    static void TestCoroutine(CTcpConnection* conn_prt);
};

void* Routine(void* arg)
{
    // register rpc channel to stub
    // when client do rpc call, will process rcp channel callmethod actually
    // google::protobuf::RpcController rpc_cntl;
    // CRpcCntl rpc_cntl;

    CTcpConnection* conn_ptr = (CTcpConnection*)arg;

    CRpcChannel rpc_channel(conn_ptr);
    EchoRequest request;
    EchoResponse response;
    request.set_message("hello sakula");
    request.set_sid(6666);
    CEchoService_Stub stub(&rpc_channel);
    stub.Echoxxx(NULL, &request, &response, NULL);    
    printf("response, message: %s, rid:%d\n", response.message().c_str(), response.rid());
}

void CTestRpcNet::TestCoroutine(CTcpConnection* conn_ptr)
{
    int32_t loop_times = 1;

    printf("Client RPC start time: %s\n", CUtils::GetCurrentTime());
    for (int32_t i = 0; i < loop_times; ++i)
    {
        // build new coroutine, rpc call will execute in coroutine
        // rpc call will send, then yield, wait for recv, wake up by other coroutine
        conn_ptr->CreateCoroutine(Routine, conn_ptr);
        conn_ptr->ResumeCoroutine();
    }    
    printf("Client RPC   end time: %s\n", CUtils::GetCurrentTime());
}

void CTestRpcNet::TestRpcClient(void)
{
    TEndPoint server_addr;
    snprintf(server_addr.ip, sizeof(server_addr.ip), "127.0.0.1");
    server_addr.port = 8888;
    printf("RPC tcp client start connect: %s:%d\n", server_addr.ip, server_addr.port);

    CEventLoop loop;
    CTcpClient client(&loop);
    // when client connection finish, will do rpc call
    client.SetConnectionCallback(std::bind(&TestCoroutine, _1));
    client.Connect(server_addr);
    // loop forever
    loop.Loop();
}

void CTestRpcNet::TestRpcServer(void)
{
    TEndPoint listen_addr;
    snprintf(listen_addr.ip, sizeof(listen_addr.ip), "127.0.0.1");
    listen_addr.port = 8888;
    printf("RPC tcp server start listen: %s:%d\n", listen_addr.ip, listen_addr.port);

    CEventLoop loop;
    google::protobuf::Service* echo_service = new CEchoServiceImpl;    
    CTcpServer server(&loop, listen_addr);
    server.AddService(echo_service);
    server.Start();
    loop.Loop();
}

