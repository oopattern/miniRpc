#include <stdio.h>
#include <string>
#include "sort_merge.h"
#include "test/test_shm_hash.cc"

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

int main(void)
{
    printf("hello world\n");    
    CTestHash::TestShmCapacity();
    //CTestHash::TestShmCapacity();
    //CTestHash::TestReadShmTPS();
    //CTestHash::TestShmMutex(THREAD_NUM, QUERY_TIME);
    //CTestHash::TestThreadAbort();
    //TestSortLargeFile();
    //TestSortMerge();
    printf("shm test finish.\n");
    return 0;
}
