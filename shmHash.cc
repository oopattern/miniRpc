#include <stdio.h>
#include <stdlib.h>
#include <assert.h> // assert
#include <string.h> // memcpy
#include <errno.h>  // errno
#include <limits.h> // LONG_MAX
#include <fcntl.h>  // open, O_RDWR
#include <sys/mman.h>// mmap
#include <sys/stat.h>// fstat
#include <math.h>   // sqrt
#include "shmHash.h"
#include "thread.h"

// Posix-mmap method, default shm way, compare with SystemV-shmget method
#define USE_MMAP    0
static const char* MMAP_NAME = "POSIX_MMAP";

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
    m_id = -1;
    m_ptr = NULL;
    m_isLock = false;
    m_isAttach = false;

    memset(m_bucket, 0x0, sizeof(m_bucket));
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

// shm create, Posix method
int CShmHash::PosixCreate(unsigned int size)
{
    // if shm exsist failed
    m_id = ::shm_open(MMAP_NAME, O_RDWR | O_CREAT | O_EXCL, SHM_USER_MODE);
    if (m_id < 0)
    {
        printf("shm_open error:%s\n", strerror(errno));
        return SHM_ERROR;
    }

    // fix size
    if (::ftruncate(m_id, size) < 0)
    {
        printf("ftruncate error:%s\n", strerror(errno));
        return SHM_ERROR;
    }

    // get shm addr
    m_ptr = ::mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, m_id, 0);
    if (MAP_FAILED == m_ptr)
    {
        printf("mmap error:%s\n", strerror(errno));
        return SHM_ERROR;
    }

    ::close(m_id);

    return SHM_OK;
}

// shm create, SystemV method
int CShmHash::SystemVCreate(unsigned int size)
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

int CShmHash::CreateShm(unsigned int size)
{
    if (size < (SHM_HEAD_SIZE + m_totalBucket * SHM_NODE_SIZE))
    {
        printf("shm size less than bucket node size\n");
        return SHM_ERROR;
    }

#if (USE_MMAP)
    return PosixCreate(size);
#else
    return SystemVCreate(size);
#endif
}

// attach shm, Posix method
int CShmHash::PosixAttach(void)
{
    struct stat shmStat;
    m_id = ::shm_open(MMAP_NAME, O_RDWR, SHM_USER_MODE);
    if (m_id < 0)
    {
        printf("shm_open error:%s\n", strerror(errno));
        return SHM_ERROR;
    }

    if (::fstat(m_id, &shmStat) < 0)
    {
        printf("fstat error:%s\n", strerror(errno));
        return SHM_ERROR;
    }

    m_ptr = ::mmap(NULL, shmStat.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, m_id, 0);
    if (MAP_FAILED == m_ptr)
    {
        printf("mmap error:%s\n", strerror(errno));
        return SHM_ERROR;
    }

    ::close(m_id);
    return SHM_OK;
}

// attach shm, SystemV method
int CShmHash::SystemVAttach(void)
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

int CShmHash::AttachShm(void)
{
    if (true == m_isAttach)
    {
        printf("tid=%d, process already attach before\n", CThread::Tid());
        return SHM_OK;
    }

#if (USE_MMAP)
    return PosixAttach();
#else
    return SystemVAttach();
#endif
}

void CShmHash::LockShm(void)
{
    TShmHead* p = (TShmHead*)m_ptr;
    ::pthread_mutex_lock(&p->mutex);
    m_isLock = true; // order can not be wrong!
}

void CShmHash::UnlockShm(void)
{
    m_isLock = false; // order can not be wrong!
    TShmHead* p = (TShmHead*)m_ptr;
    ::pthread_mutex_unlock(&p->mutex);
}

bool CShmHash::IsLockShm(void)
{
    return m_isLock;
}

// add   data: bCreat = true
// query data: bCreat = false
// return val: shm node pointer
char* CShmHash::GetNode(int uid, unsigned int hashKey, bool bCreat)
{
    char* dst = NULL;
    unsigned int primeBucketSize = 0;

    // check out lock shm for shm operation
    assert(IsLockShm());

    // mutli-order hash for add data or query data
    for (unsigned int i = 0; i < m_bucketSize; i++)
    {
        // get slot position
        unsigned int slot = hashKey & m_bucket[i];
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

int CShmHash::ReadShm(int uid, char* data, int len)
{
    assert((len > 0) && (len <= SHM_NODE_SIZE));
    
    if (m_id < 0)
    {
        return SHM_ERROR;
    }

    // get slot to query data
    // snprintf not strlen is very occpy TPS, so just pay attention to this,
    // try other way instead of snprintf, snprintf will more than 6 times!!!  
    char key[32];
    int keylen = ll2string(key, 32, (long)uid);
    unsigned int hashKey = CalcHashKey(key, keylen);

    LockShm();

    char* dst = GetNode(uid, hashKey, false);
    if (dst != NULL)
    {
        char* pVal = (char*)&(((TShmNode*)dst)->val);
        memcpy(data, pVal, len);
    }

    UnlockShm();

    return ((dst != NULL) ? SHM_OK : SHM_ERROR);
}

// add a value to shma, data struct only for TUser temporary
int CShmHash::WriteShm(int uid, const char* data, int len, bool bCreat)
{
    assert((len > 0) && (len <= SHM_NODE_SIZE));
    
    if (m_id < 0)
    {
        return SHM_ERROR;
    }

    // get slot to insert data
    char key[32];
    int keylen = ll2string(key, 32, uid);
    unsigned int hashKey = CalcHashKey(key, keylen);

    LockShm();

    // bCreat: true as add data, false as change data
    char* dst = GetNode(uid, hashKey, bCreat);
    if (dst != NULL)
    {
        ((TShmNode*)dst)->bUsed = true;
        ((TShmNode*)dst)->expireTime = 0;
        ((TShmNode*)dst)->key = uid;
        
        char* pVal = (char*)&(((TShmNode*)dst)->val);
        memcpy(pVal, data, len);
    }

    UnlockShm();

    return ((dst != NULL) ? SHM_OK : SHM_ERROR);
}

int CShmHash::ModifyShm(int uid, int chgVal)
{
    if (m_id < 0)
    {
        return SHM_ERROR;
    }

    char key[32];
    int keylen = ll2string(key, 32, uid);
    unsigned int hashKey = CalcHashKey(key, keylen);

    LockShm();

    char* dst = GetNode(uid, hashKey, false);
    if (dst != NULL)
    {
        ((TShmNode*)dst)->bUsed = true;
        ((TShmNode*)dst)->expireTime = 0;
        ((TShmNode*)dst)->key = uid;
        ((TShmNode*)dst)->val.money += chgVal;
    }

    UnlockShm();

    return ((dst != NULL) ? SHM_OK : SHM_ERROR);
}

int CShmHash::AtShm(void)
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

    m_isAttach = true;
    printf("tid=%d now attach shm=%p\n", CThread::Tid(), m_ptr);
    
    return SHM_OK;
}

bool CShmHash::IsPrime(unsigned int value)
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

int CShmHash::GenHashPrimes(void)
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

int CShmHash::InitHash(void)
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
unsigned int CShmHash::CalcHashKey(const void *key, int len) 
{
    /* 'm' and 'r' are mixing constants generated offline.
     They're not really 'magic', they just happen to work well.  */
    uint32_t seed = dict_hash_function_seed;
    const uint32_t m = 0x5bd1e995;
    const int r = 24;

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
unsigned int CShmHash::digits10(unsigned long long v) 
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
int CShmHash::ll2string(char* dst, size_t dstlen, long long svalue) 
{
    static const char digits[201] =
        "0001020304050607080910111213141516171819"
        "2021222324252627282930313233343536373839"
        "4041424344454647484950515253545556575859"
        "6061626364656667686970717273747576777879"
        "8081828384858687888990919293949596979899";
    int negative;
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
        int const i = (value % 100) * 2;
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
        int i = (uint32_t) value * 2;
        dst[next] = digits[i + 1];
        dst[next - 1] = digits[i];
    }

    /* Add sign. */
    if (negative) dst[0] = '-';
    return length;
}







