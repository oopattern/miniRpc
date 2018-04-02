#include <stdio.h>
#include "public.h"
#include "shmHash.h"


// read shm once
void TestOnceReadShm()
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
            TestOnceReadShm();
            break;
        }
        usleep(5*1000);
    }
}

// test shm read TPS, while read shm, need other threads process shm, simulate full load of CPU
void TestReadShmTPS()
{
    // confirm data record in shm
    const int QUERY_TIME = 20 * MILLION;
    ValType user;
    CShmHash shm;
    char timeBuf[64];

    // other threads need to operate shm, simulate full load of CPU
    // code not finish...

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
