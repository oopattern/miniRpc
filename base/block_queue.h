#ifndef __BLOCK_QUEUE_H
#define __BLOCK_QUEUE_H

#include <stdio.h>
#include <string>
#include <deque>
#include <string.h>  // strerror
#include <sys/time.h>// gettimeofday
#include <pthread.h> // mutex, cond
#include <assert.h>  // assert
#include <error.h>   // errno
#include "thread.h"

using namespace std;


template<typename T>
class CBlockQueue
{
public:
    CBlockQueue()
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
    
    ~CBlockQueue()
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

    // get size of queue
    size_t Size(void)
    {
        ::pthread_mutex_lock(&m_mutex);
        AssignHolder();

        size_t size = m_queue.size();
        
        UnassignHolder();
        ::pthread_mutex_unlock(&m_mutex);

        return size;
    }

    // put one obj
    void Put(const T& x)
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

    // take one obj, without timeout
    T Take(void)
    {
        T front;

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

    // take mutli obj, timeout with sec
    std::vector<T> TakeMutli(int num, int timeout)
    {
        assert(num > 0);

        std::vector<T> tbl;
        T front;

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

private:
    // cond operation
    void Notify(void)
    {
        // must lock before
        assert(m_holder > 0);
        
        if (0 != ::pthread_cond_signal(&m_cond))
        {
            printf("cond notify error:%s\n", strerror(errno));
        }
    }

    void Wait(void)
    {
        // must lock before
        assert(m_holder > 0);

        if (0 != ::pthread_cond_wait(&m_cond, &m_mutex))
        {
            printf("cond wait error:%s\n", strerror(errno));
        }
    }

    void WaitTimeout(int sec)
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
                //printf("cond wait time error:%s\n", strerror(errno));            
            }
        }       
    }

    // mutex operation
    void AssignHolder(void)
    {
        m_holder = CThread::Tid();
    }

    void UnassignHolder(void)
    {
        m_holder = 0;       
    }

private:
    pid_t               m_holder; // locking or not
    pthread_mutex_t     m_mutex;  // protect queue
    pthread_cond_t      m_cond;   // wait-notify  
    std::deque<T>       m_queue;  // share var, string temporary
};

#endif // end of __BLOCK_QUEUE_H

