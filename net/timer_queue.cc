#include <stdio.h>
#include <time.h>       // struct timespec
#include <unistd.h>     // read, close
#include <sys/timerfd.h>// timerfd_create
#include "channel.h"
#include "../base/public.h"
#include "timer_queue.h"


int32_t CTimer::m_num_create = 0;

CTimerQueue::CTimerQueue(CEventLoop* loop)
    : m_loop(loop),
      m_timer_fd(CTimerQueue::CreateTimerFd()),
      m_timer_channel(new CChannel(loop, m_timer_fd))
{
    if (m_timer_fd < 0)
    {
        printf("Timer queue init timerfd error\n");
        ::exit(-1);
    }
    m_timer_channel->SetReadCallback(std::bind(&CTimerQueue::HandleRead, this));
    m_timer_channel->EnableRead();
}

CTimerQueue::~CTimerQueue()
{
    m_timer_channel->DisableAll();
    m_timer_channel->Remove();
    ::close(m_timer_fd);

    TimerList::iterator it;
    for (it = m_timer_list.begin(); it != m_timer_list.end(); ++it)
    {
        delete it->second;
    }
    m_timer_list.clear();
}

int32_t CTimerQueue::AddTimer(int64_t when_ms, int32_t interval_ms, const TimerCallback& cb)
{
    // check if more early timer
    bool more_early = false;
    TimerList::iterator it = m_timer_list.begin();
    if ((it == m_timer_list.end()) || (when_ms < it->first))
    {
        more_early = true;
    }

    CTimer* timer = new CTimer(when_ms, interval_ms, cb);
    m_timer_list.insert(std::make_pair(when_ms, timer));

    if (true == more_early)
    {
        CTimerQueue::ResetTimerFd(m_timer_fd, when_ms);
    }       

    return timer->Sequence();
}

int32_t CTimerQueue::CancelTimer(int32_t timer_seq)
{
    TimerList::iterator it = m_timer_list.begin();
    
    while (it != m_timer_list.end())
    {
        CTimer* timer = it->second;
        if (timer_seq != timer->Sequence())
        {
            ++it;
        }
        else
        {
            // if func is running, can not delete at once,
            // because m_timer_list can not erase iterator twice
            if (kTimerFuncRunning != timer->GetFuncStatus())
            {
                printf("time queue cancel timer seq = %d\n", timer_seq);
                delete timer;
                it = m_timer_list.erase(it);
                return OK;
            }

            printf("timer func is running, need to delete later\n");
            timer->SetFuncStatus(kTimerFuncDelete);
            return OK;
        }
    }

    return ERROR;
}

void CTimerQueue::HandleRead(void)
{
    int64_t expiration = 0;
    int64_t now_ms = CUtils::NowMsec();
    ReadTimerFd(m_timer_fd);

    TimerList repeat_timer;
    repeat_timer.clear();

    TimerList::iterator it = m_timer_list.begin();
    while (it != m_timer_list.end())
    {
        // key: expiration, val: ctimer*
        expiration = it->first;
        CTimer* timer = it->second;
        
        // now time not reach expiration timeout
        if (now_ms < expiration)
        {
            break;
        }

        // run timeout callback
        timer->Run();
        it = m_timer_list.erase(it);

        // check if need to repeat
        if (!timer->Repeat() || (kTimerFuncDelete == timer->GetFuncStatus()))
        {
            delete timer;
        }
        else
        {
            timer->Restart(now_ms);
            repeat_timer.insert(std::make_pair(timer->Expiration(), timer));
        }
    }

    // resort timer queue
    TimerList::iterator re_it;
    for (re_it = repeat_timer.begin(); re_it != repeat_timer.end(); ++re_it)
    {
        m_timer_list.insert(*re_it);        
    }

    // reset next timer
    if (!m_timer_list.empty())
    {
        expiration = m_timer_list.begin()->first;
        ResetTimerFd(m_timer_fd, expiration);
    }
}

int32_t CTimerQueue::CreateTimerFd(void)
{    
    int32_t timer_fd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);    
    if (timer_fd < 0)
    {
        printf("create timerfd error: %s\n", ::strerror(errno));
        return ERROR;
    }

    return timer_fd;
}

int32_t CTimerQueue::ReadTimerFd(int32_t timer_fd)
{
    int64_t howmany;
    int32_t n = ::read(timer_fd, &howmany, sizeof(howmany));
    if (n != sizeof(howmany))
    {
        printf("read timerfd happen error\n");
    }
}

int32_t CTimerQueue::ResetTimerFd(int32_t timer_fd, int64_t when_ms)
{
    struct timespec delta;
    struct itimerspec new_val;
    struct itimerspec old_val;
    ::bzero(&delta, sizeof(delta));
    ::bzero(&new_val, sizeof(new_val));
    ::bzero(&old_val, sizeof(old_val));

    // how much time from now
    int64_t now_ms = CUtils::NowMsec();
    delta.tv_sec = (when_ms - now_ms) / 1000;
    delta.tv_nsec = ((when_ms - now_ms) % 1000) * 1000000L;
    new_val.it_value = delta;

    if (0 > ::timerfd_settime(timer_fd, 0, &new_val, &old_val))
    {
        printf("timerfd settime error: %s\n", ::strerror(errno));
        return ERROR;
    }

    return OK;
}


