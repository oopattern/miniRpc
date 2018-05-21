#include <stdio.h>
#include <string>
#include "sort_merge.h"
#include "net/tcp_server.h"
#include "net/event_loop.h"
#include "example/test_shm_hash.cc"
#include "example/test_shm_queue.cc"

using namespace std;


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

void TestMessage(CTcpConnection* conn_ptr, char* buf, int32_t len)
{
    if (len > 0)
    {
        buf[len] = '\0';
        printf("socket receive: %s\n", buf);
    }
}

void TestTcpServer(void)
{
    TEndPoint  listen_addr;
    snprintf(listen_addr.ip, sizeof(listen_addr.ip), "127.0.0.1");
    listen_addr.port = 8888;
    printf("tcp server start listen: %s:%d\n", listen_addr.ip, listen_addr.port);
    
    CEventLoop loop;
    CTcpServer server(&loop, listen_addr);
    server.SetMessageCallback(std::bind(&TestMessage, _1, _2, _3));
    server.Start();
    loop.Loop();
}

int main(void)
{
    printf("hello world\n");    
    TestTcpServer();
    return 0;
}


