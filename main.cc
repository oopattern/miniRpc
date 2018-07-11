#include <stdio.h>
#include <string>
#include "base/sort_merge.h"
#include "example/test_shm_hash.cc"
#include "example/test_shm_queue.cc"
#include "example/test_server_client.cc"
#include "example/test_rpc_server_client.cc"
#include "example/test_http_server.cc"
#include "example/test_coroutine.cc"
#include "example/test_timer_queue.cc"


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
    sort.BucketSort();
    //sort.BitmapSort();
    //sort.BitmapQuery(37, line);
    //sort.BtreeSort();
    //sort.BtreeQuery(37, line);
    //sort.SearchCompare();
}

void TestShm(void)
{
    CTestHash::TestShmExpand();
    //CTestHash::TestShmCapacity();
    //CTestHash::TestShmUnlink();
    //CTestHash::TestReadShmTPS();
    //CTestHash::TestShmMutex(THREAD_NUM, QUERY_TIME);
    //CTestHash::TestThreadAbort();
    //CTestQueue::TestQueueCapacity();
    //CTestQueue::TestQueuePush();
    //CTestQueue::TestQueuePop();
    //CTestQueue::TestQueueTPS();    
    printf("shm test finish.\n");
}



int main(void)
{
    printf("hello world\n");    
    //TestShm();
    CTestHttp::TestHttpServer();
    //CTestNet::TestTcpServer();
    //CTestNet::TestTcpClient();
    //CTestRpcNet::TestRpcServer();
    //CTestRpcNet::TestRpcClient();
    //CTestTimer::TestTimer();
    //TestSortMerge();
    return 0;
}


