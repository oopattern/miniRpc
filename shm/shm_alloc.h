#ifndef __SHM_ALLOC_H
#define __SHM_ALLOC_H

#include <stdio.h>
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
    LOCK_MUTEX  = 1, // mutex lock
    LOCK_ATOMIC = 2  // gcc atomic, lock free
};

// mutex lock info
typedef struct _tMutexLock
{
    pthread_mutex_t     mutex;
    pthread_mutexattr_t attr;
} TMutex;



// public shm alloc and lock operation, use for shm SystemV or Posix mode
class CShmAlloc
{
public:
    // Posix mode
    // difference between detach and unlink:
    // detach just memory can not access any more, when process exit will detach shm
    // unlink will delete shm memory, but it need all process detach shm before
    static void* PosixCreate(const char* name, uint32_t size);
    static void* PosixAttach(const char* name);
    static int32_t PosixUnlink(const char* name);

    // SystemV mode
    static void* SystemVCreate(int32_t key, uint32_t size);
    static void* SystemVAttach(int32_t key);
    static int32_t SystemVUnlink(int32_t key);

    // lock operation
    static int32_t InitLock(TMutex* mlock);
    static void LockShm(TMutex* mlock);
    static void UnlockShm(TMutex* mlock);
};

#endif // __SHM_ALLOC_H
