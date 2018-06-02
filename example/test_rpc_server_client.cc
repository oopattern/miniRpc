#include <stdio.h>
#include "../base/public.h"
#include "../rpc/rpc_service.h"
#include "../rpc/rpc_channel.h"


class CTestRpcNet
{
public:
    static void TestRpcMethod(CTcpConnection* conn_ptr);
    static void TestRpcClient(void);
    static void TestRpcServer(void);
};

void CTestRpcNet::TestRpcMethod(CTcpConnection* conn_ptr)
{
    // register rpc channel to stub
    // when client do rpc call, will process rcp channel callmethod actually
    //google::protobuf::RpcController rpc_cntl;
    //CRpcCntl rpc_cntl;
    CRpcChannel rpc_channel(conn_ptr);
    EchoRequest request;
    EchoResponse response;
    request.set_message("hello sakula");
    request.set_sid(6666);
    CEchoService_Stub stub(&rpc_channel);

    int32_t call_times = 1;
    printf("Client RPC call times=%d\n", call_times);
    printf("Client RPC start time: %s\n", CUtils::GetCurrentTime());
    for (int32_t i = 0; i < call_times; ++i)
    {
        stub.Echoxxx(NULL, &request, &response, NULL);    
        //printf("response, message: %s, rid:%d\n", response.message().c_str(), response.rid());
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
    client.SetConnectionCallback(std::bind(&TestRpcMethod, _1));
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

