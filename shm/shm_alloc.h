#ifndef __SHM_ALLOC_H
#define __SHM_ALLOC_H

#include <stdio.h>

// shm macro
#define SHM_USER_MODE   0777
#define SHM_ERROR       -1
#define SHM_OK          0

// mutex lock info
typedef struct _tMutexLock
{
    pthread_mutex_t     mutex;
    pthread_mutexattr_t attr;
} TMutex;


class CShmAlloc
{
public:
    static void* PosixCreate(const char* name, uint32_t size);
    static void* PosixAttach(const char* name);

    // lock operation
    static int32_t InitLock(TMutex* mlock);
    static void LockShm(TMutex* mlock);
    static void UnlockShm(TMutex* mlock);
};

#endif // __SHM_ALLOC_H
