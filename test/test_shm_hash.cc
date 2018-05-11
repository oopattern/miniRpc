#include <stdio.h>
#include <vector>
#include "../public.h"
#include "../thread.h"
#include "../shm/shm_hash.h"

using namespace std;


// 4 pthread(include main pthread), full load of CPU
const int THREAD_NUM = 3;    
static int s_threadIdx = 0;
static double s_threadTime[THREAD_NUM+1] = {0};
// query mangitude
const long long QUERY_TIME = 50 * MILLION;


class CTestHash
{
public:
    static void TestThreadAbort(void);
    static void TestReadShm(long long times);
    static void TestAbortShm(long long times);
    static void TestModifyShm(long long times);
    static void TestShmMutex(int threadNum, long long times);
    static void TestShmCapacity(void);
    static void TestReadShmTPS(void);
};

// check out if pthread coredump, will other process/thread will be dead lock?
void CTestHash::TestThreadAbort(void)
{
    int uid = 2333;
    ValType user;
    int cnt = 0;

    // first attach shm
    g_pShmHash->InitShm(POSIX, LOCK_ATOMIC, false);

    // main pthread clear user data to zero
    user.uid = uid;
    user.money = 0;
    g_pShmHash->WriteShm(uid, (char*)&user, sizeof(user), false);

    // main pthread read user data 
    g_pShmHash->ReadShm(uid, (char*)&user, sizeof(user));
    printf("origin : uid=%d, money=%lld, human money=%s\n", 
            user.uid, user.money, CUtils::ShowMagnitude(user.money));

    ::sleep(1);
    
    // normal work pthread, modify user data, increase chgVal 
    CThreadPool normal(THREAD_NUM-1, std::bind(TestModifyShm, QUERY_TIME));
    // abort work pthread, simulate abort when locking shm 
    CThreadPool dead(1, std::bind(TestAbortShm, QUERY_TIME));

    normal.StartAll();
    dead.StartAll();

    while (cnt < QUERY_TIME)
    {
        // read and modify, different from ReadShm and WriteShm
        g_pShmHash->ModifyShm(uid, 1);
        cnt++;
    }

    // wait for work pthread done
    normal.JoinAll();
    dead.JoinAll();
    
    // main pthread read user data 
    g_pShmHash->ReadShm(uid, (char*)&user, sizeof(user));

    printf("finally: uid=%d, money=%lld, human money=%s\n", 
            user.uid, user.money, CUtils::ShowMagnitude(user.money));
    printf("TestThreadAbort finish\n");
}

// read shm
// time > 0: loop times
// time = 0: loop once
// time < 0: loop forever
void CTestHash::TestReadShm(long long times)
{
    int idx = 0;
    int uid = 0;
    ValType user;
    
    long long st = CUtils::TimeInMilliseconds();

    g_pShmHash->InitShm(POSIX, LOCK_ATOMIC, false);   
    do 
    {
        uid = 2301 + idx;
        if (++idx >= 100)
        {
            idx = 0;
        }

        if (SHM_OK != g_pShmHash->ReadShm(uid, (char*)&user, sizeof(user)))
        {
            printf("shm read can not find uid=%d\n", uid);
            break;
        }
        
        if (times > 0)
        {
            times--;
        }
    } while (times != 0);
    
    long long end = CUtils::TimeInMilliseconds();

    // not safe, think how to put out the result to main thread
    // because of s_threadIdx is not be automic
    // code not finish...
    s_threadTime[s_threadIdx++] = end - st;
    printf("Work thread tid=%d use time=%lld(ms)\n", CThread::Tid(), end - st);
}

void CTestHash::TestAbortShm(long long times)
{
    int uid = 2333;
    int cnt = 0;
    ValType user;

    long long st = CUtils::TimeInMilliseconds();

    g_pShmHash->InitShm(POSIX, LOCK_ATOMIC, false);
    while (cnt < times)
    {
        // pthread will abort when operation reach times/3
        g_pShmHash->AbortShm(uid, 1, times / 2);
        cnt++;
    }

    long long end = CUtils::TimeInMilliseconds();
    printf("tid=%d use time=%lld(ms)\n", CThread::Tid(), end - st);

    g_pShmHash->ReadShm(uid, (char*)&user, sizeof(user));
    printf("Work thread tid=%d, uid=%d, money=%lld, modify shm finish\n", CThread::Tid(), user.uid, user.money);
}

void CTestHash::TestModifyShm(long long times)
{
    int uid = 2333;
    int cnt = 0;
    ValType user;

    long long st = CUtils::TimeInMilliseconds();

    g_pShmHash->InitShm(POSIX, LOCK_ATOMIC, false);
    while (cnt < times)
    {
        // read and modify, different from ReadShm and WriteShm
        g_pShmHash->ModifyShm(uid, 1);
        cnt++;
    }

    long long end = CUtils::TimeInMilliseconds();
    printf("tid=%d use time=%lld(ms)\n", CThread::Tid(), end - st);

    g_pShmHash->ReadShm(uid, (char*)&user, sizeof(user));
    printf("Work thread tid=%d, uid=%d, money=%lld, modify shm finish\n", CThread::Tid(), user.uid, user.money);
}

// test shm mutex
void CTestHash::TestShmMutex(int threadNum, long long times)
{
    int uid = 2333;
    ValType user;
    int cnt = 0;

    // first attach shm
    g_pShmHash->InitShm(POSIX, LOCK_ATOMIC, false);

    // main pthread read user data 
    g_pShmHash->ReadShm(uid, (char*)&user, sizeof(user));
    printf("origin : uid=%d, money=%lld, human money=%s\n", 
            user.uid, user.money, CUtils::ShowMagnitude(user.money));
    
    // main pthread clear user data to zero
    user.uid = uid;
    user.money = 0;
    g_pShmHash->WriteShm(uid, (char*)&user, sizeof(user), false);

    // work pthread modify user data, increase chgVal 
    CThreadPool pool(threadNum, std::bind(TestModifyShm, times));
    pool.StartAll();

    while (cnt < times)
    {
        // read and modify, different from ReadShm and WriteShm
        g_pShmHash->ModifyShm(uid, 1);
        cnt++;
    }

    // wait for work pthread done
    pool.JoinAll();
    
    // main pthread read user data 
    g_pShmHash->ReadShm(uid, (char*)&user, sizeof(user));

    printf("finally: uid=%d, money=%lld, human money=%s\n", 
            user.uid, user.money, CUtils::ShowMagnitude(user.money));
    printf("TestShmMutex finish\n");
}

// test how many data can be insert into shm
void CTestHash::TestShmCapacity()
{
    ValType user;

    int i = 0;
    user.uid = 2300;
    user.money = 60000;
    snprintf(user.name, sizeof(user.name), "sakula");

    if (g_pShmHash->InitShm(POSIX, LOCK_ATOMIC, true) != SHM_OK)
    {
        return;
    }

    while (1)
    {
        user.uid += 1;
        user.money += 2;
        
        // try to insert data into shm
        if (SHM_OK == g_pShmHash->WriteShm(user.uid, (char*)&user, sizeof(user), true))
        {
            printf("uid:%d, money:%lld\n", user.uid, user.money);
            printf("shm write done\n");
        }
        // random query one record
        else 
        {
            // after read record, end the test
            TestReadShm(0);
            g_pShmHash->ShowShm();
            break;
        }
    }
}


// test shm read TPS, while read shm, need other threads process shm, simulate full load of CPU
void CTestHash::TestReadShmTPS(void)
{
    // confirm data record in shm
    ValType user;

    printf("Query start time: %s\n", CUtils::GetCurrentTime());

    printf("Main thread tid=%d\n", CThread::Tid());
    printf("BenchMark QUERY_TIME=%s\n", CUtils::ShowMagnitude(QUERY_TIME));

    // for the same process, attach only init once
    // think of how to sync main pthread and work pthread, use condition-variable ???
    // temporary just use sleep
    g_pShmHash->InitShm(POSIX, LOCK_ATOMIC, false);
    ::sleep(1);

    // other threads need to operate shm, simulate full load of CPU
    CThreadPool pool(THREAD_NUM, std::bind(TestReadShm, QUERY_TIME));
    pool.StartAll();

    long long st = CUtils::TimeInMilliseconds();
    int cnt = 0;

    while (cnt < QUERY_TIME)
    {
        int uid = 2333 + cnt % 100;
        if (SHM_OK != g_pShmHash->ReadShm(uid, (char*)&user, sizeof(user)))
        {
            printf("shm read can not find uid=%d\n", uid);    
            return;
        }
        cnt++;
    }

    long long end = CUtils::TimeInMilliseconds();
    printf("Main thread tid=%d use time=%lld(ms)\n", CThread::Tid(), end - st);

    // wait pthread done, need something pthread sync
    // temporary use sleep to ensure work pthread finish the job
    pool.JoinAll();

    // calc full load TPS
    double allUseTime = end - st;    
    for (int i = 0; i < THREAD_NUM; i++)
    {
        allUseTime = allUseTime + s_threadTime[i];
    }
    allUseTime = allUseTime / (THREAD_NUM + 1);
    allUseTime = allUseTime / 1000.0;
    long long tps = QUERY_TIME * (THREAD_NUM+1) / allUseTime;
    printf("%d CPU full load, (all core)TPS=%s\n", THREAD_NUM + 1, CUtils::ShowMagnitude(tps));
    printf("%d CPU full load, (per core)TPS=%s\n", THREAD_NUM + 1, CUtils::ShowMagnitude(tps/(THREAD_NUM+1)));

    printf("Query   end time: %s\n", CUtils::GetCurrentTime());

    while (1)
    {
        printf("TestReadShmTPS finish, wait to Ctrl+C \n");
        ::sleep(5);
    }
}
