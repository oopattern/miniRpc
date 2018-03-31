#ifndef __SHM_HASH_H
#define __SHM_HASH_H

#include <stdio.h>

// shm macro
#define SHM_USER_MODE   0777
#define SHM_ERROR       -1
#define SHM_OK          0

// macro
typedef long long           money_t;
typedef unsigned long long  uint64_t;
typedef unsigned int        uint32_t;
typedef signed int          int32_t;
typedef unsigned short      uint16_t;
typedef signed short        int16_t;
typedef unsigned char       uint8_t;
typedef signed char         int8_t;

// hash table
typedef struct THashTbl {
    unsigned long size; // bucket number
    unsigned long mask; // bucket size - 1
    unsigned long used; // already used
    unsigned long collision; // collision number
} THashTbl;

// user info
typedef struct TUser
{
    int uid;
    char name[10]; // shm can not use string or pointer! delete will core
    money_t money;
} TUser;

// shm head info, mutex must be in shm head, for mutli process sync
typedef struct TShmHead
{
    pthread_mutex_t mutex;
} TShmHead;

// macro
const unsigned long HASH_BUCKET_MAX_SIZE = 10; // max prime count
const unsigned long HASH_INIT_PRIME = 1000; // produce prime under 1000

//const unsigned long HASH_BUCKET_SIZE = 6000;    // default hash bucket size
//const unsigned long HASH_COLLISION_SIZE = 100;  // default collision size, 1/10 with bucket size

// offset, use for search
// shm header: pthread_mutex_t 
// hash data / collision data: TUser or other protobuf data format
// | +-- shm header --+ | +-- hash data (bucket * data_size) --+ | +-- collision data (collision * data_size) --+ |
const unsigned long SHM_HEAD_SIZE = sizeof(TShmHead);
const unsigned long SHM_DATA_SIZE = sizeof(TUser);
const unsigned long SHM_DATA_POS = SHM_HEAD_SIZE;
const unsigned long SHM_COLLISION_POS = SHM_DATA_POS + SHM_DATA_SIZE * HASH_BUCKET_SIZE;


// operate shm
class CShmHash
{
public:
    int CreateShm(unsigned int size = SHM_SIZE);
    int AttachShm(void);
    int ReadShm(int uid, char* data, int len);
    int WriteShm(int uid, const char* data, int len);

private:
    int AtShm(void);

    // hash operation
    int InitHash(void);
    int GenHashPrimes(void);
    bool IsPrime(unsigned int value);
    unsigned int CalcHashKey(const void *key, int len);

    // digit operation
    unsigned int digits10(unsigned long long  v);
    int ll2string(char* dst, size_t dstlen, long long svalue);

private:
    static const int SHM_KEY = 0x20180325; // unique shm key
    static const int SHM_SIZE = 1024 * 1024; // max shm size, 1Mb
    
    // if exsist will fault, pay attention to user mode, 
    // otherwise will call permission deny
    static const int SHM_OPT = IPC_CREAT | IPC_EXCL | SHM_USER_MODE; 

    // shm identify
    int m_id; // inside id for shm
    void* m_ptr; // share memory addr
    
    // hash prime table
    THashTbl m_hash; // hash table
    uint32_t m_bucket[HASH_BUCKET_MAX_SIZE];
    uint32_t m_bucketSize;
    uint32_t m_bucketInitPrime;
};

#endif // __SHM_HASH_H
