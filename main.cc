#include <stdio.h>
#include "public.h"
#include "shmHash.h"


void TestReadShm()
{
    int uid = 2333;
    ValType user;
    CShmHash shm;
    
    shm.AttachShm();
    if (SHM_OK == shm.ReadShm(uid, (char*)&user, sizeof(user)))
    {
        printf("uid:%d, name:%s, money:%lld\n", user.uid, user.name, user.money);
    }
    else
    {
        printf("shm read can not find uid=%d\n", uid);
    }
    printf("shm read done\n");
}

void TestMutliWriteShm()
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
            TestReadShm();
            break;
        }
        usleep(5*1000);
    }
}

void TestBenchReadShm()
{
    // confirm data record in shm
    const int QUERY_TIME = 20 * MILLION;
    ValType user;
    CShmHash shm;
    char timeBuf[64];

    printf("BenchMark QUERY_TIME=%s\n", ShowMagnitude(QUERY_TIME));
    GetCurrentTime(timeBuf, sizeof(timeBuf));
    printf("Query start time: %s\n", timeBuf);

    shm.AttachShm();

    int cnt = 0;
    while (cnt < QUERY_TIME)
    {
        int uid = 2301 + cnt % 100;
        if (SHM_OK != shm.ReadShm(uid, (char*)&user, sizeof(user)))
        {
            printf("shm read can not find uid=%d\n", uid);    
            return;
        }
        cnt++;
    }

    GetCurrentTime(timeBuf, sizeof(timeBuf));
    printf("Query   end time: %s\n", timeBuf);

    printf("shm read done\n");
}

int main(void)
{
    printf("hello world\n");
    //TestMutliWriteShm();
    TestBenchReadShm();
    printf("shm test finish.\n");
    return 0;
}
