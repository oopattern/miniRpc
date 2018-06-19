#include <stdio.h>
#include <stdint.h>
#include "net_common.h"


class CChannel;
class CEventLoop;


class CTimer
{
public:
    CTimer(int64_t when_ms, int32_t interval_ms, const TimerCallback& cb)
        : m_expiration(when_ms),
          m_interval(interval_ms),
          m_callback(cb),
          m_repeat(m_interval > 0)
    {
        m_sequence = ++m_num_create;   
    }
    ~CTimer() 
    {
    
    }

    int64_t Expiration(void) const
    {
        return m_expiration;
    }
    
    int32_t Sequence(void) const
    {
        return m_sequence;
    }

    void Restart(int64_t now_ms)
    {
        m_expiration = now_ms + m_interval;                
    }

    bool Repeat(void) const 
    {
        return m_repeat;
    }

    void Run(void)
    {        
        if (m_callback)
        {
            m_callback();        
        }
    }

private:    
    int64_t         m_expiration;
    int32_t         m_interval;
    TimerCallback   m_callback;
    bool            m_repeat;
    int32_t         m_sequence;
    
    static int32_t  m_num_create;    
};

// support the same expiration
// key: expiration, val: timer
typedef std::multimap<int64_t, CTimer*> TimerList;
// key: timer sequence, val: timer
typedef std::map<int32_t, CTimer*>      TimerSeq; 

class CTimerQueue
{
public:
    CTimerQueue(CEventLoop* loop);
    ~CTimerQueue();

    int32_t AddTimer(int64_t when_ms, int32_t interval_ms, const TimerCallback& cb);    
    int32_t CancelTimer(int32_t timer_seq);

public:
    static int32_t CreateTimerFd(void);
    static int32_t ReadTimerFd(int32_t timer_fd);
    static int32_t ResetTimerFd(int32_t timer_fd, int64_t when_ms);

private:
    void HandleRead(void);    
    
private:
    CEventLoop* m_loop;
    int32_t     m_timer_fd;
    CChannel*   m_timer_channel;
    TimerList   m_timer_list;
    TimerSeq    m_cancel_timer;
    bool        m_running_callback;
};

