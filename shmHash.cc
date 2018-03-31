#include <stdio.h>
#include <stdlib.h>
#include <assert.h> // assert
#include <string.h> // memcpy
#include <pthread.h>// mutex, pthread
#include <unistd.h> // sleep
#include <sys/shm.h>// shm
#include <errno.h>  // errno
#include <limits.h> // LONG_MAX
#include <math.h>   // sqrt
#include "shmHash.h"


// hash function seed
static uint32_t dict_hash_function_seed = 5381;

static unsigned long s_digit_tbl[] = {
    1000000000UL,
    100000000UL,
    10000000,
    1000000,
    100000,
    10000,
    1000,
    100,
    10,
    1,
};

const int DIGIT_SIZE = sizeof(s_digit_tbl) / sizeof(s_digit_tbl[0]);

CShmHash::CShmHash()
{
    m_hash.size = HASH_BUCKET_SIZE;
    m_hash.mask = HASH_BUCKET_SIZE - 1;
    m_hash.used = 0;
    m_hash.collision = 0;

    memset(m_bucket, 0x0, sizeof(m_bucket));
    m_bucketSize = HASH_BUCKET_MAX_SIZE;
    m_bucketInitPrime = HASH_INIT_PRIME;

    if (InitHash() != SHM_OK)
    {
        printf("Init Hash error\n");
    }
}

CShmHash::~CShmHash()
{

}

int CShmHash::CreateShm(unsigned int size = SHM_SIZE)
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

int CShmHash::AttachShm(void)
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

int CShmHash::ReadShm(int uid, char* data, int len)
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
int CShmHash::WriteShm(int uid, const char* data, int len)
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
unsigned int CShmHash::HashKeyFunction(const void *key, int len) 
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







