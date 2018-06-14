#include <stdio.h>
#include <stdint.h>
#include "net_common.h"


class CChannel;
class CEventLoop;


typedef enum 
{
    kTimerFuncStop = 0,
    kTimerFuncRunning = 1,
    kTimerFuncDelete = 2,
} TimerFuncStat;

class CTimer
{
public:
    CTimer(int64_t when_ms, int32_t interval_ms, const TimerCallback& cb)
        : m_expiration(when_ms),
          m_interval(interval_ms),
          m_callback(cb),
          m_repeat(m_interval > 0)
    {
        m_func_stat = kTimerFuncStop;
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

    TimerFuncStat GetFuncStatus(void) const
    {
        return m_func_stat;
    }

    void SetFuncStatus(TimerFuncStat stat)
    {
        m_func_stat = stat;
    }

    void Run(void)
    {        
        // exclude kTimerFuncDelete stat
        if (kTimerFuncDelete != GetFuncStatus())
        {
            SetFuncStatus(kTimerFuncRunning);
        }
        
        m_callback();

        // exclude kTimerFuncDelete stat
        if (kTimerFuncDelete != GetFuncStatus())
        {
            SetFuncStatus(kTimerFuncStop);
        }
    }

private:    
    int64_t         m_expiration;
    int32_t         m_interval;
    TimerCallback   m_callback;
    bool            m_repeat;
    TimerFuncStat   m_func_stat;
    int32_t         m_sequence;
    
    static int32_t  m_num_create;    
};

typedef std::map<int64_t, CTimer*> TimerList;

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
};

