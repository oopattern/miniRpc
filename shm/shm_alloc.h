#ifndef __SHM_ALLOC_H
#define __SHM_ALLOC_H

#include <stdio.h>
#include <assert.h>
#include <pthread.h>

// shm macro
#define SHM_USER_MODE   0777
#define SHM_ERROR       -1
#define SHM_OK          0

// alloc type for shm
enum AllocType 
{
    POSIX = 1, // posix way, use mmap
    SVIPC = 2  // systemV way, use shmget
};

// lock type for shm, mutex lock or atomic
enum LockType
{
    LOCK_MUTEX = 1, // mutex lock
    LOCK_ATOMIC = 2   // gcc atomic, lock free
};

// mutex lock info
typedef struct _tMutexLock
{
    pthread_mutex_t     mutex;
    pthread_mutexattr_t attr;
} TMutex;

// thread safe singleton
template<typename T>
class Singleton
{
public:
    // init just run once in mutli pthread
    static T& Instance(void)
    {
        ::pthread_once(&m_ponce, &Singleton::Init);
        assert(m_value != NULL);
        return *m_value;
    }

private:
    Singleton();
    ~Singleton();

    static void Init(void)
    {
        m_value = new T();
    }

private:
    static pthread_once_t   m_ponce;
    static T*               m_value;
};

// init static variable
template<typename T> 
pthread_once_t Singleton<T>::m_ponce = PTHREAD_ONCE_INIT;

template<typename T>
T* Singleton<T>::m_value = NULL;

// public shm alloc and lock operation, use for shm SystemV or Posix mode
class CShmAlloc
{
public:
    // Posix mode
    static void* PosixCreate(const char* name, uint32_t size);
    static void* PosixAttach(const char* name);

    // SystemV mode
    static void* SystemVCreate(int32_t key, uint32_t size);
    static void* SystemVAttach(int32_t key);

    // lock operation
    static int32_t InitLock(TMutex* mlock);
    static void LockShm(TMutex* mlock);
    static void UnlockShm(TMutex* mlock);
};

#endif // __SHM_ALLOC_H
