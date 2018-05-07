#include <stdio.h>
#include <stdint.h>  // int32_t
#include <fcntl.h>   // open, O_RDWR
#include <string.h>  // memcpy
#include <errno.h>   // errno
#include <pthread.h> // mutex, pthread
#include <sys/mman.h>// mmap
#include <unistd.h>  // sleep, ftruncate, close
#include <sys/stat.h>// fstat
#include "shm_alloc.h"


int32_t CShmAlloc::InitLock(TMutex* mlock)
{
    printf("InitLock use mutex...\n");

    if (NULL == mlock)
    {
        printf("mutex lock init error\n");
        return SHM_ERROR;
    }

    // mutex and mutex_attr should be in shm
    // init pthread_mutexattr
    if (0 != ::pthread_mutexattr_init(&mlock->attr))
    {
        printf("mutex_attr init error:%s\n", strerror(errno));
        return SHM_ERROR;
    }

    // set process share attribute, so mutile-process can sync 
    if (0 != ::pthread_mutexattr_setpshared(&mlock->attr, PTHREAD_PROCESS_SHARED))
    {
        printf("pthread_mutexattr_setshared error:%s\n", strerror(errno));
        return SHM_ERROR;
    }

    // set mutex robust attribute, if pthread coredump during locking, 
    // other pthread can recover mutex, otherwise other process/thread will be dead lock
    if (0 != ::pthread_mutexattr_setrobust(&mlock->attr, PTHREAD_MUTEX_ROBUST))
    {
        printf("pthread_mutexattr_setrobust error:%s\n", strerror(errno));
        return SHM_ERROR;
    }

    // init pthread mutex with mutex_attr
    if (0 != ::pthread_mutex_init(&mlock->mutex, &mlock->attr))
    {
        printf("mutex init error:%s\n", strerror(errno));
        return SHM_ERROR;
    }

    return SHM_OK;
}

void CShmAlloc::LockShm(TMutex* mlock)
{
    if (NULL == mlock)
    {
        printf("lock shm mlock is NULL error\n");
        return;
    }
    
    int32_t ret = ::pthread_mutex_lock(&mlock->mutex);
    if (EOWNERDEAD == ret)
    {        
        ::pthread_mutex_consistent(&mlock->mutex);
        printf("[Fatal Error] pthread coredump when locking, try to release the mutex lock\n");
    }
}

void CShmAlloc::UnlockShm(TMutex* mlock)
{
    if (NULL == mlock)
    {
        printf("unlock shm mlock is NULL error\n");
        return;
    }
    ::pthread_mutex_unlock(&mlock->mutex);
}

// shm create, Posix method
void* CShmAlloc::PosixCreate(const char* name, uint32_t size)
{
    // if shm exsist failed
    int32_t id = ::shm_open(name, O_RDWR | O_CREAT | O_EXCL, SHM_USER_MODE);
    if (id < 0)
    {
        printf("shm_open error:%s\n", strerror(errno));
        return NULL;
    }

    // fix size
    if (::ftruncate(id, size) < 0)
    {
        printf("ftruncate error:%s\n", strerror(errno));
        return NULL;
    }

    // get shm addr
    void* ptr = ::mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, id, 0);
    if (MAP_FAILED == ptr)
    {
        printf("mmap error:%s\n", strerror(errno));
        return NULL;
    }

    // clear zero of the region
    memset(ptr, 0x0, size);

    // above if fail, will not close fd
    // code not finish...
    ::close(id);

    return ptr;
}

// attach shm, Posix method
void* CShmAlloc::PosixAttach(const char* name)
{
    struct stat shmStat;
    int32_t id = ::shm_open(name, O_RDWR, SHM_USER_MODE);
    if (id < 0)
    {
        printf("shm_open error:%s\n", strerror(errno));
        return NULL;
    }

    if (::fstat(id, &shmStat) < 0)
    {
        printf("fstat error:%s\n", strerror(errno));
        return NULL;
    }

    void* ptr = ::mmap(NULL, shmStat.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, id, 0);
    if (MAP_FAILED == ptr)
    {
        printf("mmap error:%s\n", strerror(errno));
        return NULL;
    }

    // above if fail, will not close fd
    // code not finish...
    ::close(id);    

    return ptr;
}
