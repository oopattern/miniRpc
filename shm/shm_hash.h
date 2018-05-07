#ifndef __SHM_HASH_H
#define __SHM_HASH_H

#include <stdio.h>
#include <stdint.h> // uint32_t
#include <unistd.h> // sleep
#include <sys/shm.h>// shmget
#include "shm_alloc.h"

// define global
#define g_pShmHash      CShmHash::Instance()

#if 0
// hash table
typedef struct THashTbl {
    unsigned long size; // bucket number
    unsigned long mask; // bucket size - 1
    unsigned long used; // already used
    unsigned long collision; // collision number
} THashTbl;
#endif

// macro
typedef int32_t        	KeyType;
typedef long long       money_t;

// user info
typedef struct _tValType
{
    int32_t uid;
    char    name[10]; // shm can not use string or pointer! delete will core
    money_t money;
} ValType;

// operate shm
class CShmHash
{
private:
    // shm node data
    typedef struct _tShmNode
    {
        bool    bUsed; // true: already used
        int32_t expireTime; // expire time arrived, delete the data
        int32_t readAtomic; // atomic operation
        KeyType key;
        ValType val;
    } TShmNode;

    // shm head info, mutex must be in shm head, for mutli process sync
    typedef struct _tShmHead
    {   
        TMutex      mlock; // mutex lock
        int32_t     accessAtomic; // atomic operation
    } TShmHead;

public:
    static CShmHash* Instance(void);

    // create shm, use Posix mmap or SystemV shmget method, for shm server process
    int32_t CreateShm(uint32_t size = SHM_SIZE);
    // attach shm, for shm client process
    int32_t AttachShm(void);
    // uid: key
    int32_t ModifyShm(int32_t uid, int32_t chgVal);
    // uid: key, data: val
    int32_t ReadShm(int32_t uid, char* data, int32_t len);
    // bCreat: true mean add data, false mean change data
    int32_t WriteShm(int32_t uid, const char* data, int32_t len, bool bCreat); 
    // show shm status
    void ShowShm(void);

    // check out if pthread occur coredump, will happen dead lock?
    // just for test !!! absolutely can not use !!!
    int32_t AbortShm(int32_t uid, int32_t chgVal, int32_t target);

private:
    // SystemV method
    int32_t AtShm(void);
    int32_t SystemVCreate(uint32_t size);
    int32_t SystemVAttach(void);

    // lock operation
    int32_t InitLock(void);
    bool IsLockShm(void);
    void LockShm(void);
    void UnlockShm(void);
    void AtomicLockNode(TShmNode* p);
    void AtomicUnlockNode(TShmNode* p);

    // hash operation
    char* GetNode(int32_t uid, uint32_t hashKey, bool bCreat);
    int32_t InitHash(void);
    int32_t GenHashPrimes(void);
    bool IsPrime(uint32_t value);
    uint32_t CalcHashKey(const void *key, int32_t len);

    // digit operation
    uint32_t digits10(uint64_t v);
    int32_t ll2string(char* dst, size_t dstlen, int64_t svalue);

private:
    // singleton, can use template and pthread safe 
    // code not finish...
    static CShmHash* m_pInstance;
    CShmHash();
    ~CShmHash();

private:
    static const int32_t SHM_KEY = 0x20180325; // unique shm key
    static const int32_t SHM_SIZE = 1024 * 1024; // max shm size, 1Mb
    static const uint64_t HASH_BUCKET_MAX_SIZE = 10; // max prime count
    static const uint64_t HASH_INIT_PRIME = 1000; // produce prime under 1000

    // offset, use for search
    // shm header: pthread_mutex_t 
    // shm node  : node struct or protobuf data format
    // | +-- shm header --+ | +-- shm node (bucket * node_size) --+ |
    static const uint64_t SHM_HEAD_SIZE = sizeof(TShmHead);
    static const uint64_t SHM_NODE_SIZE = sizeof(TShmNode);
    
    // if exsist will fault, pay attention to user mode, 
    // otherwise delete shm will call permission deny
    static const int32_t SHM_OPT = IPC_CREAT | IPC_EXCL | SHM_USER_MODE; 

    // shm identify
    int32_t m_id; // inside id for shm
    void*   m_ptr; // share memory addr
    bool    m_isLock; // shm lock status
    bool    m_isAttach; // shm attach finish
    
    // hash prime table
    uint32_t m_bucketUsed;
    uint32_t m_totalBucket;
    uint32_t m_bucket[HASH_BUCKET_MAX_SIZE];
    uint32_t m_bucketSize;
    uint32_t m_bucketInitPrime;
};

#endif // __SHM_HASH_H
