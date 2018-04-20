#include <stdio.h>
#include <stdlib.h>
#include <string.h>  // strerror
#include <sys/time.h>// gettimeofday
#include <pthread.h> // mutex, cond
#include <assert.h>  // assert
#include <error.h>   // errno
#include "thread.h"
#include "block_queue.h"

#ifndef ERROR
#define ERROR   -1
#endif

#ifndef OK
#define OK      0
#endif

CBlockQueue::CBlockQueue()
{
    // init pthread mutex, default attr
    if (0 != ::pthread_mutex_init(&m_mutex, NULL))
    {
        printf("mutex init error:%s\n", strerror(errno));
        ::exit(-1);
    }

    if (0 != ::pthread_cond_init(&m_cond, NULL))
    {
        printf("cond init error:%s\n", strerror(errno));
        ::exit(-1);
    }
}

CBlockQueue::~CBlockQueue()
{
    if (0 != ::pthread_cond_destroy(&m_cond))
    {
        printf("cond destroy error:%s\n", strerror(errno));
    }

    if (0 != ::pthread_mutex_destroy(&m_mutex))
    {
        printf("mutex destroy error:%s\n", strerror(errno));
    }
}

void CBlockQueue::AssignHolder()
{
    m_holder = CThread::Tid();
}

void CBlockQueue::UnassignHolder()
{
    m_holder = 0;
}

void CBlockQueue::Notify()
{
    // must lock before
    assert(m_holder > 0);
    
    if (0 != ::pthread_cond_signal(&m_cond))
    {
        printf("cond notify error:%s\n", strerror(errno));
    }
}

void CBlockQueue::Wait()
{
    // must lock before
    assert(m_holder > 0);

    if (0 != ::pthread_cond_wait(&m_cond, &m_mutex))
    {
        printf("cond wait error:%s\n", strerror(errno));
    }
}

void CBlockQueue::WaitTimeout(int sec)
{
    // must lock before
    assert(m_holder > 0);

    struct timespec tsp;
    struct timeval now;

    ::gettimeofday(&now, NULL);
    tsp.tv_sec = now.tv_sec;
    tsp.tv_nsec = now.tv_usec * 1000;
    tsp.tv_sec += sec;

    // wait timeout
    if (0 != ::pthread_cond_timedwait(&m_cond, &m_mutex, &tsp))
    {
        if (errno != ETIMEDOUT)
        {
            printf("cond wait time error:%s\n", strerror(errno));            
        }
    }
}

void CBlockQueue::Put(const std::string& x)
{
    // take care of order between assignholder and lock
    ::pthread_mutex_lock(&m_mutex);
    AssignHolder();

    m_queue.push_back(x);
    Notify();
    
    // take care of order between unassignholder and unlock
    UnassignHolder();
    ::pthread_mutex_unlock(&m_mutex);
}

std::vector<string> CBlockQueue::TakeMutli(int num, int timeout)
{
    assert(num > 0);

    std::vector<string> tbl;
    std::string front;

    // take care of order between assignholder and lock
    ::pthread_mutex_lock(&m_mutex);
    AssignHolder();

    if (m_queue.size() < num)
    {
        WaitTimeout(timeout);
    }

    if (m_queue.size() >= num)
    {
        for (int i = 0; i < num; i++)
        {
            front = m_queue.front();
            m_queue.pop_front();
            tbl.push_back(front);
        }
    }

    // take care of order between unassignholder and unlock
    UnassignHolder();
    ::pthread_mutex_unlock(&m_mutex);

    return tbl;
}

std::string CBlockQueue::Take()
{
    std::string front;

    // take care of order between assignholder and lock
    ::pthread_mutex_lock(&m_mutex);
    AssignHolder();

    while (m_queue.empty())
    {
        Wait();   
    }

    if (! m_queue.empty())
    {
        front = m_queue.front();
        m_queue.pop_front();
    }

    // take care of order between unassignholder and unlock
    UnassignHolder();
    ::pthread_mutex_unlock(&m_mutex);

    return front;
}

size_t CBlockQueue::Size()
{
    ::pthread_mutex_lock(&m_mutex);
    AssignHolder();

    size_t size = m_queue.size();
    
    UnassignHolder();
    ::pthread_mutex_unlock(&m_mutex);

    return size;
}
