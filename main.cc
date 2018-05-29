#include <stdio.h>
#include <string>
#include "base/sort_merge.h"
#include "net/tcp_client.h"
#include "net/tcp_server.h"
#include "net/event_loop.h"
#include "rpc/rpc_service.h"
#include "example/test_shm_hash.cc"
#include "example/test_shm_queue.cc"
#include "rpc/rpc_channel.h"


using namespace std;

// 1) try tcpdump command, check out what is happen:
// # tcpdump -i lo tcp port 8888 -X -s 0 -v
// 2) try protoc command, to generate *.pb.cc and *.pb.h files
// # protoc --cpp_out=./ std_rpc_meta.proto, you can refer to CMakeLists.txt


void TestSortLargeFile(void)
{
    CSortMerge sort;
    sort.InitLargeFile();
}

void TestSortMerge(void)
{
    CSortMerge sort;
    std::string line;
    //sort.GeneralSplit();
    //sort.GeneralMerge();
    //sort.BucketSort();
    //sort.BitmapSort();
    //sort.BitmapQuery(37, line);
    //sort.BtreeSort();
    //sort.BtreeQuery(37, line);
    sort.SearchCompare();
}

void TestShm(void)
{
    //CTestHash::TestShmCapacity();
    //CTestHash::TestReadShmTPS();
    //CTestHash::TestShmMutex(THREAD_NUM, QUERY_TIME);
    //CTestHash::TestThreadAbort();
    CTestQueue::TestQueueCapacity();
    //CTestQueue::TestQueuePush();
    //CTestQueue::TestQueuePop();
    //CTestQueue::TestQueueTPS();    
    printf("shm test finish.\n");
}

void ServerMessage(CTcpConnection* conn_ptr, char* buf, int32_t len)
{
    if (len > 0)
    {
        buf[len] = '\0';
        printf("server receive: %s\n", buf);
    }
}

void ClientMessage(CTcpConnection* conn_ptr, char* buf, int32_t len)
{
    if (len > 0)
    {
        buf[len] = '\0';
        printf("client receive: %s\n", buf);
    }
}

void TestTcpClient(void)
{
    TEndPoint server_addr;
    snprintf(server_addr.ip, sizeof(server_addr.ip), "127.0.0.1");
    server_addr.port = 8888;
    printf("tcp client start connect: %s:%d\n", server_addr.ip, server_addr.port);

    CEventLoop loop;
    CTcpClient client(&loop);
    client.SetMessageCallback(std::bind(&ClientMessage, _1, _2, _3));
    client.Connect(server_addr);
    loop.Loop();
}

void TestTcpServer(void)
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

void TestRpcClient(void)
{
    TEndPoint server_addr;
    snprintf(server_addr.ip, sizeof(server_addr.ip), "127.0.0.1");
    server_addr.port = 8888;
    printf("RPC tcp client start connect: %s:%d\n", server_addr.ip, server_addr.port);

    CEventLoop loop;
    CTcpClient client(&loop);
    client.Connect(server_addr);

    // register rpc channel to stub
    // when client do rpc call, will process rcp channel callmethod actually
    //google::protobuf::RpcController rpc_cntl;
    //CRpcCntl rpc_cntl;
    CRpcChannel rpc_channel(&client);
    EchoRequest request;
    EchoResponse response;
    CEchoService_Stub stub(&rpc_channel);
    stub.Echoxxx(NULL, &request, &response, NULL);

    // loop forever
    loop.Loop();
}

void TestRpcServer(void)
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

int main(void)
{
    printf("hello world\n");    
    //TestTcpServer();
    //TestTcpClient();
    //TestRpcServer();
    TestRpcClient();
    return 0;
}


