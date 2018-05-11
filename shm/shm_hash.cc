#include <stdio.h>
#include <stdlib.h>
#include <assert.h> // assert
#include <string.h> // memcpy
#include <errno.h>  // errno
#include <limits.h> // LONG_MAX
#include <fcntl.h>  // open, O_RDWR
#include <math.h>   // sqrt
#include "shm_hash.h"
#include "../thread.h"

// Posix-mmap method, default shm way, compare with SystemV-shmget method
static const uint8_t g_allocType = POSIX;
// read-write concurrent sync way, default atomic, compare with mutex lock
// for mutli-thread in a process, atomic may be the best way, TPS will be much higher
// for mutli-process, mutex or atomic may be little difference
static const uint8_t g_lockType = LOCK_MUTEX;

static const int32_t ATOMIC_COUNT = 2;
static const int64_t ATOMIC_TRY_LIMIT = 200000000L;
static const char*   MMAP_HASH = "POSIX_MMAP_HASH";

// hash function seed
static uint32_t dict_hash_function_seed = 5381;

// init singleton
CShmHash* CShmHash::m_pInstance = NULL;

CShmHash* CShmHash::Instance(void)
{
    if (NULL == m_pInstance)
    {
        m_pInstance = new CShmHash();
    }
    return m_pInstance;
}

CShmHash::CShmHash()
{
    m_ptr = NULL;
    m_isLock = false;
    m_isAttach = false;
    m_allocType = POSIX;
    m_lockType = LOCK_MUTEX;

    memset(m_bucket, 0x0, sizeof(m_bucket));
    m_bucketUsed = 0;
    m_totalBucket = 0;
    m_bucketSize = HASH_BUCKET_MAX_SIZE;
    m_bucketInitPrime = HASH_INIT_PRIME;

    if (InitHash() != SHM_OK)
    {
        printf("Init Hash error\n");
        exit(-1);
    }
}

CShmHash::~CShmHash()
{

}

void CShmHash::ShowShm(void)
{
    printf("- - - - - - - - - - - - - - - HASH_STATUS_START - - - - - - - - - - - - - - - \n");
    printf("hash total bucket: %d\n", m_totalBucket);
    printf("hash  used bucket: %d\n", m_bucketUsed);
    printf("hash      percent: %.2f%%\n", (double)m_bucketUsed*100.0/m_totalBucket);
    for (int32_t i = 0; i < HASH_BUCKET_MAX_SIZE; i++)
    {
        printf("hash bucket:%d prime:%d\n", i+1, m_bucket[i]);
    }
    printf("- - - - - - - - - - - - - - - HASH_STATUS_END - - - - - - - - - - - - - - - - \n");
}

int32_t CShmHash::InitShm(uint8_t allocType, uint8_t lockType, bool bCreat, uint32_t size)
{
    // just for test
    m_allocType = g_allocType;
    m_lockType = g_lockType;
    //m_allocType = allocType;
    //m_lockType = lockType;

    if (false == bCreat)
    {
        return AttachShm();
    }
    return CreateShm(size);
}

int32_t CShmHash::CreateShm(uint32_t size)
{
    if (size < (SHM_HEAD_SIZE + m_totalBucket * SHM_NODE_SIZE))
    {
        printf("shm size less than bucket node size\n");
        return SHM_ERROR;
    }

    // Posix mode
    if (POSIX == m_allocType)
    {
        m_ptr = CShmAlloc::PosixCreate(MMAP_HASH, size);
    }
    // SystemV mode
    else
    {
        m_ptr = CShmAlloc::SystemVCreate(SHM_KEY, size);
    }

    if (NULL == m_ptr)
    {
        printf("Create shm alloc mode=%d error\n", m_allocType);
        return SHM_ERROR;
    }
    
    // shm init lock, for process, just init once
    if (InitLock() != SHM_OK)
    {
        m_isAttach = false;
        return SHM_ERROR;
    }

    m_isAttach = true;
    return SHM_OK;
}

int32_t CShmHash::AttachShm(void)
{
    if (true == m_isAttach)
    {
        printf("tid=%d, process already attach before\n", CThread::Tid());
        return SHM_OK;
    }

    if (POSIX == m_allocType)
    {
        m_ptr = CShmAlloc::PosixAttach(MMAP_HASH);
    }
    else
    {
        m_ptr = CShmAlloc::SystemVAttach(SHM_KEY);
    }

    if (NULL == m_ptr)
    {
        printf("Attach shm alloc mode=%d error\n", m_allocType);
        return SHM_ERROR;
    }

    // shm init lock, for process, just init once
    if (SHM_OK != InitLock())
    {
        printf("Attach shm init lock error\n");
        m_isAttach = false;
        return SHM_ERROR;
    }

    printf("tid=%d now attach shm=%p\n", CThread::Tid(), m_ptr);
    m_isAttach = true;
    return SHM_OK;
}

int32_t CShmHash::InitLock(void)
{
    if (LOCK_MUTEX == m_lockType)
    {
        TShmHead* p = (TShmHead*)m_ptr;
        return CShmAlloc::InitLock(&p->mlock);
    }
    else
    {
        printf("InitLock use atomic...\n");
    }    
    return SHM_OK;
}

void CShmHash::Lock(void)
{
    // just for mutex lock
    if (LOCK_MUTEX == m_lockType)
    {
        TShmHead* p = (TShmHead*)m_ptr;    
        CShmAlloc::LockShm(&p->mlock);
        m_isLock = true; // order can not be wrong!
    }
    // atomic not care about it
    else
    {
        m_isLock = true;
    }
}

void CShmHash::Unlock(void)
{
    // just for mutex lock
    if (LOCK_MUTEX == m_lockType)
    {
        m_isLock = false; // order can not be wrong!
        TShmHead* p = (TShmHead*)m_ptr;
        CShmAlloc::UnlockShm(&p->mlock);
    }
    // atomic not care about it
    else
    {
        m_isLock = false;
    }
}

bool CShmHash::IsLockShm(void)
{
    // if it use atomic, just return true
    return ((LOCK_MUTEX == m_lockType) ? m_isLock : true);
}

// just use for atomic way
void CShmHash::AtomicLockNode(TShmNode* p)
{
    if (m_lockType != LOCK_ATOMIC)
    {
        return;
    }

    int64_t tmp = 0;
    static int64_t s_max_loop = 0;

    // replace_val: -1
    // expected_val: 0
    while (! ::__sync_bool_compare_and_swap(&p->access, 0, -1))
    {
        // just calc max loop times will try to get atomic
        if (tmp > s_max_loop)
        {
            s_max_loop = tmp;
        }

        // use CAS, process will try to get atomic until finish
        //printf("AtomicLockNode tid=%d, read_atomic=%d, tmp=%ld, s_max_loop=%ld\n", CThread::Tid(), p->access, tmp, s_max_loop);
        
        tmp++;
        // if pthread atomic coredump suddenly, shm atomic var will not change
        // so we should check out by ourself, or will turn to be dead cycle...
        if (tmp >= ATOMIC_TRY_LIMIT)
        {
            // why access should be set to -1, not other value ?
            ::__sync_lock_test_and_set(&p->access, -1);
            printf("[Fatal Error] tid=%d AtomicLockNode shm dead, try to release atomic\n", CThread::Tid());
            break;
        }        
    }
}

void CShmHash::AtomicUnlockNode(TShmNode* p)
{
    if (m_lockType != LOCK_ATOMIC)
    {
        return;
    }
    ::__atomic_fetch_add(&p->access, 1, __ATOMIC_RELEASE);
}

// add   data: bCreat = true
// query data: bCreat = false
// return val: shm node pointer
char* CShmHash::GetNode(int32_t uid, uint32_t hashKey, bool bCreat)
{
    char* dst = NULL;
    uint32_t primeBucketSize = 0;

    // check out lock shm for shm operation
    assert(IsLockShm());

    // mutli-order hash for add data or query data
    for (uint32_t i = 0; i < m_bucketSize; i++)
    {
        // get slot position
        uint32_t slot = hashKey % m_bucket[i];
        dst = (char*)m_ptr + SHM_HEAD_SIZE + SHM_NODE_SIZE * (slot + primeBucketSize);
        
        // add data
        if ((true == bCreat) 
            && (false == ((TShmNode*)dst)->bUsed))
        {
            printf("insert data success slot=%d, bucket=%d, bucketSize=%d\n", 
                    slot, i+1, m_bucket[i]);
            return dst;
        }
        // query data
        else if ((false == bCreat) 
                && (true == ((TShmNode*)dst)->bUsed) 
                && (uid == ((TShmNode*)dst)->key)) 
        {
            return dst;
        }

        // next prime bucket
        primeBucketSize = primeBucketSize + m_bucket[i];
    }

    printf("uid:%d bCreat:%d GetNode error\n", uid, bCreat);
    return NULL;
}

int32_t CShmHash::ReadShm(int32_t uid, char* data, int32_t len)
{
    assert((len > 0) && (len <= SHM_NODE_SIZE));
    
    if (NULL == m_ptr)
    {
        printf("shm hash init error\n");
        return SHM_ERROR;
    }

    // get slot to query data
    // snprintf not strlen is very occpy TPS, so just pay attention to this,
    // try other way instead of snprintf, snprintf will more than 6 times!!!  
    char key[32];
    int32_t keylen = ll2string(key, 32, uid);
    uint32_t hashKey = CalcHashKey(key, keylen);

    Lock();

    char* dst = GetNode(uid, hashKey, false);
    if (dst != NULL)
    {
        // protect access shm node
        AtomicLockNode((TShmNode*)dst);

        char* pVal = (char*)&(((TShmNode*)dst)->val);
        memcpy(data, pVal, len);

        // release access shm node
        AtomicUnlockNode((TShmNode*)dst);
    }
    else
    {
        printf("ReadShm occur error\n");
        exit(-1);
    }

    Unlock();

    return ((dst != NULL) ? SHM_OK : SHM_ERROR);
}

// add a value to shma, data struct only for TUser temporary
int32_t CShmHash::WriteShm(int32_t uid, const char* data, int32_t len, bool bCreat)
{
    assert((len > 0) && (len <= SHM_NODE_SIZE));
    
    if (NULL == m_ptr)
    {
        printf("shm hash init error\n");
        return SHM_ERROR;
    }

    // get slot to insert data
    char key[32];
    int32_t keylen = ll2string(key, 32, uid);
    uint32_t hashKey = CalcHashKey(key, keylen);

    Lock();

    // bCreat: true as add data, false as change data
    char* dst = GetNode(uid, hashKey, bCreat);
    if (dst != NULL)
    {        
        // protect access shm node
        AtomicLockNode((TShmNode*)dst);

        // take care !!!
        // between AtomicLockNode and AtomicUnlockNode, if occur abort,
        // other thread or process will turn to be dead lock
        // because atomic var in shm release abnormal
        // need think about how to solve this problem ?

        ((TShmNode*)dst)->bUsed = true;
        ((TShmNode*)dst)->expireTime = 0;
        ((TShmNode*)dst)->key = uid;
        
        char* pVal = (char*)&(((TShmNode*)dst)->val);
        memcpy(pVal, data, len);

        if (true == bCreat)
        {
            m_bucketUsed++;
        }

        // release access shm node
        AtomicUnlockNode((TShmNode*)dst);
    }
    else
    {
        printf("WriteShm occur error\n");
        exit(-1);
    }

    Unlock();

    return ((dst != NULL) ? SHM_OK : SHM_ERROR);
}

// just for test !!! absolutely can not use !!!
int32_t CShmHash::AbortShm(int32_t uid, int32_t chgVal, int32_t target)
{
    static int32_t s_init = 0;

    if (NULL == m_ptr)
    {
        printf("shm hash init error\n");
        return SHM_ERROR;
    }

    char key[32];
    int32_t keylen = ll2string(key, 32, uid);
    uint32_t hashKey = CalcHashKey(key, keylen);

    Lock();

    char* dst = GetNode(uid, hashKey, false);
    if (dst != NULL)
    {
        // protect access shm node
        AtomicLockNode((TShmNode*)dst);

        ((TShmNode*)dst)->bUsed = true;
        ((TShmNode*)dst)->expireTime = 0;
        ((TShmNode*)dst)->key = uid;
        ((TShmNode*)dst)->val.money += chgVal;

        // simulate abort situation
        // can not use ::exit()/::abort(), because will terminate all thread within process
        s_init++;
        if (s_init >= target)
        {
            printf("tid=%d ready to occur abort, check if dead lock\n", CThread::Tid());
            ::pthread_exit(NULL);
            return SHM_ERROR;
        }

        // release access shm node
        AtomicUnlockNode((TShmNode*)dst);
    }
    else
    {
        printf("Modify occur error\n");
        exit(-1);
    }

    Unlock();

    return ((dst != NULL) ? SHM_OK : SHM_ERROR);
}

int32_t CShmHash::ModifyShm(int32_t uid, int32_t chgVal)
{
    if (NULL == m_ptr)
    {
        printf("shm hash init error\n");
        return SHM_ERROR;
    }

    char key[32];
    int32_t keylen = ll2string(key, 32, uid);
    uint32_t hashKey = CalcHashKey(key, keylen);

    Lock();

    char* dst = GetNode(uid, hashKey, false);
    if (dst != NULL)
    {
        // protect access shm node
        AtomicLockNode((TShmNode*)dst);

        // take care !!!
        // between AtomicLockNode and AtomicUnlockNode, if occur abort,
        // other thread or process will turn to be dead lock
        // because atomic var in shm release abnormal
        // need think about how to solve this problem ?

        ((TShmNode*)dst)->bUsed = true;
        ((TShmNode*)dst)->expireTime = 0;
        ((TShmNode*)dst)->key = uid;
        ((TShmNode*)dst)->val.money += chgVal;

        // release access shm node
        AtomicUnlockNode((TShmNode*)dst);
    }
    else
    {
        printf("Modify occur error\n");
        exit(-1);
    }

    Unlock();

    return ((dst != NULL) ? SHM_OK : SHM_ERROR);
}

bool CShmHash::IsPrime(uint32_t value)
{
    uint32_t i = 0;
    uint32_t square = (uint32_t)sqrt(value);    

    for (i = 2; i <= square; i++)
    {
        if (0 == (value % i))
        {
            return false;
        }
    }

    return true;
}

int32_t CShmHash::GenHashPrimes(void)
{
    uint32_t val = 0;
    uint32_t cnt = 0;

    for (val = m_bucketInitPrime; val > 1; val--)
    {
        if (IsPrime(val))
        {
            m_bucket[cnt] = val;
            m_totalBucket = m_totalBucket + m_bucket[cnt];
            cnt++;        
        }

        if (cnt == m_bucketSize)
        {
            return SHM_OK;
        }
    }

    return SHM_ERROR;
}

int32_t CShmHash::InitHash(void)
{
    return GenHashPrimes();
}

/* MurmurHash2, by Austin Appleby
 * Note - This code makes a few assumptions about how your machine behaves -
 * 1. We can read a 4-byte value from any address without crashing
 * 2. sizeof(int) == 4
 *
 * And it has a few limitations -
 *
 * 1. It will not work incrementally.
 * 2. It will not produce the same results on little-endian and big-endian
 *    machines.
 */
uint32_t CShmHash::CalcHashKey(const void *key, int32_t len) 
{
    /* 'm' and 'r' are mixing constants generated offline.
     They're not really 'magic', they just happen to work well.  */
    uint32_t seed = dict_hash_function_seed;
    const uint32_t m = 0x5bd1e995;
    const int32_t r = 24;

    /* Initialize the hash to a 'random' value */
    uint32_t h = seed ^ len;

    /* Mix 4 bytes at a time into the hash */
    const unsigned char *data = (const unsigned char *)key;

    while (len >= 4) 
    {
        uint32_t k = *(uint32_t*)data;

        k *= m;
        k ^= k >> r;
        k *= m;

        h *= m;
        h ^= k;

        data += 4;
        len -= 4;
    }

    /* Handle the last few bytes of the input array  */
    switch (len) 
    {
        case 3: h ^= data[2] << 16;     break;
        case 2: h ^= data[1] << 8;      break;
        case 1: h ^= data[0]; h *= m;   break;
    };

    /* Do a few final mixes of the hash to ensure the last few
     * bytes are well-incorporated. */
    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;

    return (unsigned int)h;
}

/* Return the number of digits of 'v' when converted to string in radix 10.
 * See ll2string() for more information. */
uint32_t CShmHash::digits10(uint64_t v) 
{
    if (v < 10) return 1;
    if (v < 100) return 2;
    if (v < 1000) return 3;
    if (v < 1000000000000UL) 
    {
        if (v < 100000000UL) 
        {
            if (v < 1000000) 
            {
                if (v < 10000) return 4;
                return 5 + (v >= 100000);
            }
            return 7 + (v >= 10000000UL);
        }
        if (v < 10000000000UL) 
        {
            return 9 + (v >= 1000000000UL);
        }
        return 11 + (v >= 100000000000UL);
    }
    return 12 + digits10(v / 1000000000000UL);
}

/* Convert a long long into a string. Returns the number of
 * characters needed to represent the number.
 * If the buffer is not big enough to store the string, 0 is returned.
 *
 * Based on the following article (that apparently does not provide a
 * novel approach but only publicizes an already used technique):
 *
 * https://www.facebook.com/notes/facebook-engineering/three-optimization-tips-for-c/10151361643253920
 *
 * Modified in order to handle signed integers since the original code was
 * designed for unsigned integers. */
int32_t CShmHash::ll2string(char* dst, size_t dstlen, int64_t svalue) 
{
    static const char digits[201] =
        "0001020304050607080910111213141516171819"
        "2021222324252627282930313233343536373839"
        "4041424344454647484950515253545556575859"
        "6061626364656667686970717273747576777879"
        "8081828384858687888990919293949596979899";
    int32_t negative;
    unsigned long long value;

    /* The main loop works with 64bit unsigned integers for simplicity, so
     * we convert the number here and remember if it is negative. */
    if (svalue < 0) 
    {
        if (svalue != LLONG_MIN) 
        {
            value = -svalue;
        } 
        else 
        {
            value = ((unsigned long long) LLONG_MAX)+1;
        }
        negative = 1;
    } 
    else 
    {
        value = svalue;
        negative = 0;
    }

    /* Check length. */
    uint32_t const length = digits10(value)+negative;
    if (length >= dstlen) return 0;

    /* Null term. */
    uint32_t next = length;
    dst[next] = '\0';
    next--;
    while (value >= 100) 
    {
        int32_t const i = (value % 100) * 2;
        value /= 100;
        dst[next] = digits[i + 1];
        dst[next - 1] = digits[i];
        next -= 2;
    }

    /* Handle last 1-2 digits. */
    if (value < 10) 
    {
        dst[next] = '0' + (uint32_t) value;
    } 
    else 
    {
        int32_t i = (uint32_t) value * 2;
        dst[next] = digits[i + 1];
        dst[next - 1] = digits[i];
    }

    /* Add sign. */
    if (negative) dst[0] = '-';
    return length;
}

