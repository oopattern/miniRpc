#ifndef __SHM_HASH_H
#define __SHM_HASH_H

#include <stdio.h>
#include <stdint.h> // uint32_t
#include <unistd.h> // sleep
#include <pthread.h>// mutex, pthread
#include <sys/shm.h>// shm

// define global
#define g_pShmHash      CShmHash::Instance()

// shm macro
#define SHM_USER_MODE   0777
#define SHM_ERROR       -1
#define SHM_OK          0

// macro
typedef int             KeyType;
typedef long long       money_t;

#if 0
// hash table
typedef struct THashTbl {
    unsigned long size; // bucket number
    unsigned long mask; // bucket size - 1
    unsigned long used; // already used
    unsigned long collision; // collision number
} THashTbl;
#endif

// user info
typedef struct _tValType
{
    int uid;
    char name[10]; // shm can not use string or pointer! delete will core
    money_t money;
} ValType;

// shm node data
typedef struct _tShmNode
{
    bool    bUsed; // true: already used
    int     expireTime; // expire time arrived, delete the data
    KeyType key;
    ValType val;
} TShmNode;

// shm head info, mutex must be in shm head, for mutli process sync
typedef struct _tShmHead
{
    pthread_mutex_t mutex;
} TShmHead;

// macro
const unsigned long HASH_BUCKET_MAX_SIZE = 10; // max prime count
const unsigned long HASH_INIT_PRIME = 1000; // produce prime under 1000

// offset, use for search
// shm header: pthread_mutex_t 
// shm node  : node struct or protobuf data format
// | +-- shm header --+ | +-- shm node (bucket * node_size) --+ |
const unsigned long SHM_HEAD_SIZE = sizeof(TShmHead);
const unsigned long SHM_NODE_SIZE = sizeof(TShmNode);


// operate shm
class CShmHash
{
public:
    static CShmHash* Instance(void);

    // create shm, use Posix mmap or SystemV shmget method, for shm server process
    int CreateShm(unsigned int size = SHM_SIZE);
    // attach shm, for shm client process
    int AttachShm(void);
    // uid: key
    int ModifyShm(int uid, int chgVal);
    // uid: key, data: val
    int ReadShm(int uid, char* data, int len);
    // bCreat: true mean add data, false mean change data
    int WriteShm(int uid, const char* data, int len, bool bCreat); 

private:
    // SystemV method
    int AtShm(void);
    int SystemVCreate(unsigned int size);
    int SystemVAttach(void);

    // Posix method
    int PosixCreate(unsigned int size);
    int PosixAttach(void);

    // lock operation
    void LockShm(void);
    void UnlockShm(void);
    bool IsLockShm(void);

    // hash operation
    char* GetNode(int uid, unsigned int hashKey, bool bCreat);
    int InitHash(void);
    int GenHashPrimes(void);
    bool IsPrime(unsigned int value);
    unsigned int CalcHashKey(const void *key, int len);

    // digit operation
    unsigned int digits10(unsigned long long  v);
    int ll2string(char* dst, size_t dstlen, long long svalue);

private:
    // singleton, can use template and pthread safe 
    // code not finish...
    static CShmHash* m_pInstance;
    CShmHash();
    ~CShmHash();

private:
    static const int SHM_KEY = 0x20180325; // unique shm key
    static const int SHM_SIZE = 1024 * 1024; // max shm size, 1Mb
    
    // if exsist will fault, pay attention to user mode, 
    // otherwise delete shm will call permission deny
    static const int SHM_OPT = IPC_CREAT | IPC_EXCL | SHM_USER_MODE; 

    // shm identify
    int     m_id; // inside id for shm
    void*   m_ptr; // share memory addr
    bool    m_isLock; // shm lock status
    bool    m_isAttach; // shm attach finish
    
    // hash prime table
    uint32_t m_totalBucket;
    uint32_t m_bucket[HASH_BUCKET_MAX_SIZE];
    uint32_t m_bucketSize;
    uint32_t m_bucketInitPrime;
};

#endif // __SHM_HASH_H
