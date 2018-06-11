#include <stdio.h>
#include "../base/public.h"
#include "../rpc/rpc_service.h"
#include "../rpc/rpc_channel.h"
#include "../net/tcp_connection.h"
#include "../third_party/libco/co_routine.h"
#include "../third_party/libco/co_routine_inner.h"
#include <vector>

using std::vector;


class CTestRpcNet
{
public:
    static void TestRpcClient(void);
    static void TestRpcServer(void);
    static void TestGcRoutine(void);
    static void TestNoGcRoutine(void);
    static void TestCoroutine(CTcpConnection* conn_prt);
};

static int32_t s_loop_times = 0;
static std::vector<CRpcCoroutine*> s_no_gc_coroutine_vec;
static std::vector<stCoRoutine_t*> s_gc_coroutine_vec;
#define RPC_MAX_LOOP    10000


void* GcRoutine(void* arg)
{
    if (++s_loop_times >= RPC_MAX_LOOP)
    {
        printf("Client RPC loop = %d, end time: %s\n", RPC_MAX_LOOP, CUtils::GetCurrentTime());
    }
}

void CTestRpcNet::TestGcRoutine(void)
{
    for (int32_t i = 0; i < RPC_MAX_LOOP; ++i)
    {
        stCoRoutine_t* co = NULL;
        co_create(&co, NULL, GcRoutine, NULL);
        s_gc_coroutine_vec.push_back(co);
        co_resume(co);
    }   

    while (1)
    {
        if (s_loop_times >= RPC_MAX_LOOP)
        {
            printf("prepare_gc to delete coroutine size=%d\n", (int)s_gc_coroutine_vec.size());
            std::vector<stCoRoutine_t*>::iterator it;
            for (it = s_gc_coroutine_vec.begin(); it != s_gc_coroutine_vec.end(); ++it)
            {
                co_release(*it);
            }
            s_gc_coroutine_vec.clear();
        }
        ::sleep(5);
        printf("gc routine s_loop_times=%d\n", s_loop_times);
    }
}

void CTestRpcNet::TestNoGcRoutine(void)
{
    for (int32_t i = 0; i < RPC_MAX_LOOP; ++i)
    {
        CRpcCoroutine* coroutine = new CRpcCoroutine(GcRoutine, NULL);
        s_no_gc_coroutine_vec.push_back(coroutine);
        printf("malloc coroutine = %p, size = %d\n", coroutine, (int32_t)sizeof(*coroutine));
        coroutine->Resume();
    }

    while (1)
    {
        if (s_loop_times >= RPC_MAX_LOOP)
        {
            printf("prepare_no_gc to delete coroutine size=%d\n", (int)s_no_gc_coroutine_vec.size());
            std::vector<CRpcCoroutine*>::iterator it;
            for (it = s_no_gc_coroutine_vec.begin(); it != s_no_gc_coroutine_vec.end(); ++it)
            {
                (*it)->Release();
                printf("delete coroutine=%p\n", *it);
                delete *it;
            }
            s_no_gc_coroutine_vec.clear();
        }
        ::sleep(5);
        printf("no gc routine s_loop_times=%d\n", s_loop_times);
    }   
}

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
    //printf("response, message: %s, rid:%d\n", response.message().c_str(), response.rid());

    if (++s_loop_times >= RPC_MAX_LOOP)
    {
        printf("Client RPC loop = %d, end time: %s\n", RPC_MAX_LOOP, CUtils::GetCurrentTime());
    }
}

void CTestRpcNet::TestCoroutine(CTcpConnection* conn_ptr)
{
    printf("Client RPC start time: %s\n", CUtils::GetCurrentTime());
    for (int32_t i = 0; i < RPC_MAX_LOOP; ++i)
    {
        // build new coroutine, rpc call will execute in coroutine
        // rpc call will send, then yield, wait for recv, wake up by other coroutine
        CRpcCoroutine* coroutine = new CRpcCoroutine(Routine, conn_ptr);
        printf("malloc coroutine = %p, size = %d\n", coroutine, (int32_t)sizeof(*coroutine));
        conn_ptr->RegisterCoroutine(coroutine);
        coroutine->Resume();
    }    
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

