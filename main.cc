#include <stdio.h>
#include <vector>
#include "public.h"
#include "shmHash.h"
#include "thread.h"

using namespace std;


// read shm
void TestReadShm(bool always)
{
    int idx = 0;
    int uid = 2301;
    ValType user;
    CShmHash shm;
    
    printf("tid=%d doing TestReadShm\n", CThread::Tid());

    shm.AttachShm();
    do 
    {        
        uid = uid + idx;
        if (++idx >= 100)
        {
            idx = 0;
        }
        if (SHM_OK != shm.ReadShm(uid, (char*)&user, sizeof(user)))
        {
            printf("shm read can not find uid=%d\n", uid);
            break;
        }
    } while (always);
    
    printf("shm read done\n");
}

// always change shm data
void TestAlwaysWriteShm(void)
{
    int testUid = 2333;    
    ValType user;
    CShmHash shm;
    
    // first get user data
    shm.AttachShm();
    shm.ReadShm(testUid, (char*)&user, sizeof(user));
    while (1)
    {
        // increase user money
        user.money += 1;
        if (user.money >= (30 * MILLION))
        {
            user.money = 0;
            printf("pid:%d, uid:%d, too much money:%lld, clear it\n", ::getpid(), user.uid, user.money);
        }

        // just change shm data, not insert shm data 
        if (SHM_OK != shm.WriteShm(user.uid, (char*)&user, sizeof(user), false))
        {
            printf("uid:%d, shm write error\n", user.uid);
        }
    }
}

// test how many data can be insert into shm
void TestShmCapacity()
{
    ValType user;
    CShmHash shm;

    int i = 0;
    user.uid = 2300;
    user.money = 60000;
    snprintf(user.name, sizeof(user.name), "sakula");

    if (shm.CreateShm() != SHM_OK)
    {
        return;
    }

    while (1)
    {
        user.uid += 1;
        user.money += 2;
        
        // try to insert data into shm
        if (SHM_OK == shm.WriteShm(user.uid, (char*)&user, sizeof(user), true))
        {
            printf("uid:%d, money:%lld\n", user.uid, user.money);
            printf("shm write done\n");
        }
        // random query one record
        else 
        {
            // after read record, end the test
            TestReadShm(false);
            break;
        }
        usleep(5*1000);
    }
}

// work thread for read shm
void TestWorkerThread(void)
{
    // 4 pthread, full load of CPU
    const int THREAD_NUM = 4;    
    vector<CThread*> threadVec;
    for (int i = 0; i < THREAD_NUM; i++)
    {        
        CThread* p = new CThread(std::bind(TestReadShm, true));
        threadVec.push_back(p);
    }

    // run thread for read shm
    vector<CThread*>::iterator it;
    for (it = threadVec.begin(); it != threadVec.end(); ++it)
    {
        if (NULL == *it)
        {
            continue;
        }
        (*it)->Start();
    }
}

// test shm read TPS, while read shm, need other threads process shm, simulate full load of CPU
void TestReadShmTPS(void)
{
    // confirm data record in shm
    const int QUERY_TIME = 20 * MILLION;
    ValType user;
    CShmHash shm;
    char timeBuf[64];

    // other threads need to operate shm, simulate full load of CPU
    TestWorkerThread();

    printf("BenchMark QUERY_TIME=%s\n", ShowMagnitude(QUERY_TIME));
    GetCurrentTime(timeBuf, sizeof(timeBuf));
    printf("Query start time: %s\n", timeBuf);

    shm.AttachShm();

    int cnt = 0;
    while (cnt < QUERY_TIME)
    {
        int uid = 2333 + cnt % 1;
        if (SHM_OK != shm.ReadShm(uid, (char*)&user, sizeof(user)))
        {
            printf("shm read can not find uid=%d\n", uid);    
            return;
        }
        cnt++;
    }

    GetCurrentTime(timeBuf, sizeof(timeBuf));
    printf("Query   end time: %s\n", timeBuf);

    printf("uid:%d, money:%lld\n", user.uid, user.money);
    printf("shm read done\n");
}

int main(void)
{
    printf("hello world\n");    
    //TestShmCapacity();
    TestReadShmTPS();
    printf("shm test finish.\n");
    return 0;
}
