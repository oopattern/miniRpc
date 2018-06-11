#include <stdio.h>
#include <string>
#include "base/sort_merge.h"
#include "example/test_shm_hash.cc"
#include "example/test_shm_queue.cc"
#include "example/test_server_client.cc"
#include "example/test_rpc_server_client.cc"
#include "example/test_coroutine.cc"


using namespace std;

// 1) try tcpdump command, check out what is happen:
// # tcpdump -i lo tcp port 8888 -X -s 0 -v
// 2) try protoc command, to generate *.pb.cc and *.pb.h files
// # protoc --cpp_out=./ std_rpc_meta.proto, you can refer to CMakeLists.txt
// 3) try mount command to share file between windows and ubuntu
// # mount -t vboxsf Github /mnt/share


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



int main(void)
{
    printf("hello world\n");    
    //CTestNet::TestTcpServer();
    //CTestNet::TestTcpClient();
    CTestRpcNet::TestRpcServer();
    //CTestRpcNet::TestRpcClient();
    return 0;
}


