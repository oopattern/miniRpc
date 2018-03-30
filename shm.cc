#include <stdio.h>
#include <stdlib.h>
#include <assert.h> // assert
#include <string.h> // memcpy
#include <pthread.h>// mutex, pthread
#include <unistd.h> // sleep
#include <sys/shm.h>// shm
#include <errno.h>  // errno
#include <string>
#include "hash.h"

using namespace std;

typedef long long money_t;

// shm macro
#define SHM_MODE    0600
#define SHM_ERROR   -1
#define SHM_OK      0

// user info
typedef struct TUser
{
    int uid;
    char name[10]; // can not use string! delete will core
    money_t money;
} TUser;

// shm head info, mutex must be in shm head, for mutli process sync
typedef struct TShmHead
{
    pthread_mutex_t mutex;
} TShmHead;

// offset, use for search
const unsigned long SHM_HEAD_SIZE = sizeof(TShmHead);
const unsigned long SHM_DATA_SIZE = sizeof(TUser);
const unsigned long SHM_DATA_POS = SHM_HEAD_SIZE;
const unsigned long SHM_COLLISION_POS = SHM_DATA_POS + SHM_DATA_SIZE * HASH_BUCKET_SIZE;

// operate shm
class CShm
{
public:
    CShm() 
    {
        m_hash.size = HASH_BUCKET_SIZE;
        m_hash.mask = HASH_BUCKET_SIZE - 1;
        m_hash.used = 0;
        m_hash.collision = 0;
    }
    ~CShm() {}
    
    int CreateShm(unsigned int size = SHM_SIZE)
    {
        m_id = ::shmget(SHM_KEY, size, SHM_OPT);
        if (m_id < 0)
        {
            printf("shmget error:%s\n", strerror(errno));
            return SHM_ERROR;
        }
        printf("create shmid=%d\n", m_id);
        return AtShm();
    }

    int AttachShm(void)
    {
        m_id = ::shmget(SHM_KEY, 0, 0);
        if (m_id < 0)
        {
            printf("shmget error:%s\n", strerror(errno));
            return SHM_ERROR;
        }
        printf("attach shmid=%d\n", m_id);
        return AtShm();
    }

    int ReadShm(int uid, char* data, int len)
    {
        assert((len > 0) && (len <= SHM_DATA_SIZE));
        
        if (m_id < 0)
        {
            return SHM_ERROR;
        }

        // get slot to query data
        // snprintf not strlen is very occpy TPS, so just pay attention to this,
        // try other way instead of snprintf, snprintf will more than 6 times!!!  
        char key[32];
        int keylen = ll2string(key, 32, (long)uid);
        unsigned int h = HashKeyFunction(key, keylen);
        unsigned int slot = h & m_hash.mask;
        char* dst = NULL;

        // mutex for sync        
        TShmHead* p = (TShmHead*)m_ptr;
        ::pthread_mutex_lock(&p->mutex);

        // handle collision
        int success = 0;
        dst = (char*)m_ptr + SHM_DATA_POS + (slot * SHM_DATA_SIZE);
        if (uid == ((TUser*)dst)->uid)
        {
            success = 1;
            memcpy(data, dst, len);
        }
        // collision
        else
        {
            int i = 0;
            while (i < HASH_COLLISION_SIZE)
            {
                dst = (char*)m_ptr + SHM_COLLISION_POS + (i * SHM_DATA_SIZE);
                if (uid == ((TUser*)dst)->uid)
                {
                    success = 1;
                    memcpy(data, dst, len);
                    break;
                }
                i++;
            }
        }        

        ::pthread_mutex_unlock(&p->mutex);

        return ((1 == success) ? SHM_OK : SHM_ERROR);
    }

    // add a value to shma, data struct only for TUser temporary
    int WriteShm(int uid, const char* data, int len)
    {
        assert((len > 0) && (len <= SHM_DATA_SIZE));
        
        if (m_id < 0)
        {
            return SHM_ERROR;
        }

        if (m_hash.collision >= HASH_COLLISION_SIZE)
        {
            return SHM_ERROR;
        }

        // get slot to insert data
        char key[32];
        snprintf(key, sizeof(key), "%d", uid);
        unsigned int h = HashKeyFunction(key, strlen(key));
        unsigned int slot = h & m_hash.mask;
        char* dst = NULL;

        // mutex for sync
        TShmHead* p = (TShmHead*)m_ptr;
        ::pthread_mutex_lock(&p->mutex);

        // handle collision
        int success = 0;
        int collision = 0;
        dst = (char*)m_ptr + SHM_DATA_POS + (slot * SHM_DATA_SIZE);
        
        // normal
        if (0 == ((TUser*)dst)->uid)
        {
            success = 1;
            memcpy(dst, data, len);
            printf("insert data success slot=%d, bucket size=%lu\n", slot, HASH_BUCKET_SIZE);
        }
        // collision
        else
        {
            int i = 0;
            while (i < HASH_COLLISION_SIZE)
            {
                dst = (char*)m_ptr + SHM_COLLISION_POS + (i * SHM_DATA_SIZE);
                if (0 == ((TUser*)dst)->uid)
                {
                    collision = 1;
                    success = 1;
                    memcpy(dst, data, len);
                    break;
                }            
                i++;
            }
            printf("----------------------collision=%lu, max collision=%lu-------------------\n", 
                    m_hash.collision, HASH_COLLISION_SIZE);
        }

        if (1 == success)
        {
            m_hash.used += 1;
        }
        if (1 == collision)
        {
            m_hash.collision += 1;
        }

        printf("hash used=%lu, collision=%lu, max collision=%lu\n", 
            m_hash.used, m_hash.collision, HASH_COLLISION_SIZE);
        ::pthread_mutex_unlock(&p->mutex);

        return ((success == 1) ? SHM_OK : SHM_ERROR);
    }
private:
    static const int SHM_KEY = 0x20180325; // unique shm key
    static const int SHM_SIZE = 1024 * 1024; // max shm size, 1Mb
    
    // if exsist will fault, pay attention to user mode, 
    // otherwise will call permission deny
    static const int SHM_OPT = IPC_CREAT | IPC_EXCL | 0777; 

    int AtShm(void)
    {
        if (m_id < 0)
        {
            return SHM_ERROR;
        }
        
        m_ptr = ::shmat(m_id, 0, 0);
        if (m_ptr == (void*)-1)
        {
            printf("shmat error:%s\n", strerror(errno));
            return SHM_ERROR;
        }
        
        TShmHead* p = (TShmHead*)m_ptr;
        if (::pthread_mutex_init(&p->mutex, NULL) != 0)
        {
            printf("mutex init error:%s\n", strerror(errno));
            return SHM_ERROR;
        }

        return SHM_OK;
    }

    int m_id; // inside id for shm
    void* m_ptr; // share memory addr
    THashTbl m_hash; // hash table
};

void TestReadShm()
{
    int uid = 2333;
    TUser user;
    CShm shm;
    
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
    TUser user;
    CShm shm;

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
        if (SHM_OK == shm.WriteShm(user.uid, (char*)&user, sizeof(user)))
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
        usleep(100*1000);
    }
}

void TestBenchReadShm()
{
    // confirm data record in shm
    const int QUERY_TIME = 20000000;
    TUser user;
    CShm shm;
    char timeBuf[64];

    shm.AttachShm();

    printf("BenchMark lidi QUERY_TIME=%d\n", QUERY_TIME);

    GetCurrentTime(timeBuf, sizeof(timeBuf));
    printf("Query start time: %s\n", timeBuf);

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
