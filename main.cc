#include <stdio.h>
#include <vector>
#include "public.h"
#include "shmHash.h"
#include "thread.h"

using namespace std;

// 4 pthread(include main pthread), full load of CPU
const int THREAD_NUM = 3;    
static double s_threadTime[THREAD_NUM] = {0};
// query mangitude
const long long QUERY_TIME = 50 * MILLION;

static void TestReadShm(int threadSeq, long long times);
static void TestModifyShm(int threadSeq, long long times);
static void TestAlwaysWriteShm(void);
static void TestShmMutex(void);
static void TestShmCapacity();
static void TestWorkerThread(const ThreadFunc& cb);
static void TestReadShmTPS(void);

// read shm
// time > 0: loop times
// time = 0: loop once
// time < 0: loop forever
void TestReadShm(int threadSeq, long long times)
{
    int idx = 0;
    int uid = 0;
    ValType user;
    CShmHash shm;
    
    long long st = TimeInMilliseconds();

    shm.AttachShm();   
    do 
    {
        uid = 2301 + idx;
        if (++idx >= 100)
        {
            idx = 0;
        }

        if (SHM_OK != shm.ReadShm(uid, (char*)&user, sizeof(user)))
        {
            printf("shm read can not find uid=%d\n", uid);
            break;
        }
        
        if (times > 0)
        {
            times--;
        }
    } while (times != 0);
    
    long long end = TimeInMilliseconds();
    if (threadSeq < THREAD_NUM)
    {
        s_threadTime[threadSeq] = end - st;
    }
    printf("Work thread tid=%d use time=%lld(ms)\n", CThread::Tid(), end - st);
}

void TestModifyShm(int threadSeq, long long times)
{
    int cnt = 0;
    CShmHash shm;
    ValType user;

    shm.AttachShm();
    while (cnt < times)
    {
        // read and modify, different from ReadShm and WriteShm
        shm.ModifyShm(2333, 1);
        cnt++;
    }
    printf("Work thread modify shm finish\n");
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

// test shm mutex
void TestShmMutex(void)
{
    int uid = 2333;
    CShmHash shm;
    ValType user;

    // main pthread clear user data to zero
    user.uid = uid;
    user.money = 0;
    shm.AttachShm();
    shm.WriteShm(uid, (char*)&user, sizeof(user), false);

    // work pthread modify user data, increase chgVal 
    for (int i = 0; i < THREAD_NUM; i++)
    {
        TestWorkerThread(std::bind(TestModifyShm, i, QUERY_TIME));
    }

    // wait for work pthread done
    ::sleep(30);
    
    // main pthread read user data 
    shm.ReadShm(uid, (char*)&user, sizeof(user));

    printf("uid=%d, money=%lld, human money=%s\n", 
            user.uid, user.money, ShowMagnitude(user.money));
    printf("TestShmMutex finish\n");
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
            TestReadShm(0, 0);
            break;
        }
        usleep(5*1000);
    }
}

// work thread for read shm
void TestWorkerThread(const ThreadFunc& cb)
{
    // where is delete? will due to memory leak?
    // code not finish...
    CThread* p = new CThread(cb);
    if (p != NULL)
    {
        p->Start();
    }

    /*
    vector<CThread*> threadVec;
    for (int i = 0; i < threadNum; i++)
    {        
        // true means always read shm
        CThread* p = new CThread(cb);
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
    }*/
}

// test shm read TPS, while read shm, need other threads process shm, simulate full load of CPU
void TestReadShmTPS(void)
{
    // confirm data record in shm
    ValType user;
    CShmHash shm;
    //char timeBuf[64];
    //GetCurrentTime(timeBuf, sizeof(timeBuf));
    //printf("Query start time: %s\n", timeBuf);

    printf("Main thread tid=%d\n", CThread::Tid());
    printf("BenchMark QUERY_TIME=%s\n", ShowMagnitude(QUERY_TIME));

    shm.AttachShm();

    // other threads need to operate shm, simulate full load of CPU
    for (int i = 0; i < THREAD_NUM; i++)
    {
        TestWorkerThread(std::bind(TestReadShm, i, QUERY_TIME));
    }

    long long st = TimeInMilliseconds();

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

    long long end = TimeInMilliseconds();
    printf("Main thread tid=%d use time=%lld(ms)\n", CThread::Tid(), end - st);

    // wait pthread done, need something pthread sync
    // code not finish...
    // temporary use sleep to ensure work pthread finish the job
    ::sleep(3);

    // calc full load TPS
    double allUseTime = end - st;    
    for (int i = 0; i < THREAD_NUM; i++)
    {
        allUseTime = allUseTime + s_threadTime[i];
    }
    allUseTime = allUseTime / (THREAD_NUM + 1);
    allUseTime = allUseTime / 1000.0;
    long long tps = QUERY_TIME * (THREAD_NUM+1) / allUseTime;
    printf("%d CPU full load, (all core)TPS=%s\n", THREAD_NUM + 1, ShowMagnitude(tps));
    printf("%d CPU full load, (per core)TPS=%s\n", THREAD_NUM + 1, ShowMagnitude(tps/(THREAD_NUM+1)));

    while (1)
    {
        printf("TestReadShmTPS finish, wait to Ctrl+C \n");
        ::sleep(5);
    }
}

int main(void)
{
    printf("hello world\n");    
    //TestShmCapacity();
    //TestReadShmTPS();
    TestShmMutex();
    printf("shm test finish.\n");
    return 0;
}
