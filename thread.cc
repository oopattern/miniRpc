#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <sys/syscall.h>
#include "thread.h"

#define gettid() syscall(SYS_gettid)


// just use for pthread operation, any other better method?
typedef struct TThreadData
{
    ThreadFunc func;

    TThreadData(const ThreadFunc& f) 
        : func(f)
    { }

    void RunInThread(void)
    {
        try
        {
            func();
        }
        catch (...)
        {
            printf("unknown exception caught in Thread\n");
            throw;
        }
    }
    
    // static for pthread_creat arg
    static void* StartThread(void* obj)
    {
        TThreadData* p = static_cast<TThreadData*>(obj);
        if (p != NULL)
        {
            p->RunInThread();
            delete p;
        }
        return NULL;
    }
} TThreadData;

CThread::CThread(const ThreadFunc& func)
{
    m_started = false;
    m_joined = false;
    m_pthreadId = -1;
    m_func = func;
}

CThread::~CThread()
{

}

int CThread::Start(void)
{
    m_started = true;
    // pthread_create arg must use static member function
    TThreadData* data = new(std::nothrow) TThreadData(m_func);
    if (0 != ::pthread_create(&m_pthreadId, NULL, &TThreadData::StartThread, data))
    {
        m_started = false;
        printf("pthread_create error\n");
        return -1;
    }
    return 0;
}

int CThread::Join(void)
{
    m_joined = true;
    return ::pthread_join(m_pthreadId, NULL);
}

pid_t CThread::Tid()
{
    return ::gettid();
}

CThreadPool::CThreadPool(int threadNum, const ThreadFunc& func)
{
    m_joined = false;
    m_pool.clear();

    for (int i = 0; i < threadNum; i++)
    {
        // all new should be check error ?
        // code not finish...
        CThread* p = new(std::nothrow) CThread(func);
        m_pool.push_back(p);
    }
}

CThreadPool::~CThreadPool()
{
    // should be take care of resource
    // code not finish...
    assert(true == m_joined);

    vector<CThread*>::iterator it;
    for (it = m_pool.begin(); it != m_pool.end(); ++it)
    {
        if (NULL == *it)
        {
            continue;
        }
        delete *it;
    }
    m_pool.clear();
}

int CThreadPool::StartAll(void)
{
    vector<CThread*>::const_iterator it;
    for (it = m_pool.cbegin(); it != m_pool.cend(); ++it)
    {
        if (NULL == *it)
        {
            continue;
        }
        (*it)->Start();
    }
}

int CThreadPool::JoinAll(void)
{
    vector<CThread*>::const_iterator it;
    for (it = m_pool.cbegin(); it != m_pool.cend(); ++it)
    {
        if (NULL == *it)
        {
            continue;
        }
        (*it)->Join();
    }
    m_joined = true;
}

