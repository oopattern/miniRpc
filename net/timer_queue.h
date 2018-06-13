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
    CTimer(int32_t when, int32_t interval, const TimerCallback& cb)
        : m_expiration(when),
          m_interval(interval),
          m_callback(cb),
          m_repeat(m_interval > 0)
    {
        m_func_stat = kTimerFuncStop;
        m_sequence = ++m_num_create;   
    }
    ~CTimer() 
    {
    
    }

    int32_t Expiration(void) const
    {
        return m_expiration;
    }
    
    int32_t Sequence(void) const
    {
        return m_sequence;
    }

    void Restart(int32_t now)
    {
        m_expiration = now + m_interval;                
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
    int32_t         m_expiration;
    int32_t         m_interval;
    TimerCallback   m_callback;
    bool            m_repeat;
    TimerFuncStat   m_func_stat;
    int32_t         m_sequence;
    
    static int32_t  m_num_create;    
};

typedef std::map<int32_t, CTimer*> TimerList;

class CTimerQueue
{
public:
    CTimerQueue(CEventLoop* loop);
    ~CTimerQueue();

    int32_t AddTimer(int32_t when, int32_t interval, const TimerCallback& cb);    
    int32_t CancelTimer(int32_t timer_seq);

public:
    static int32_t CreateTimerFd(void);
    static int32_t ReadTimerFd(int32_t timer_fd);
    static int32_t ResetTimerFd(int32_t timer_fd, int32_t when);

private:
    void HandleRead(void);    
    
private:
    CEventLoop* m_loop;
    int32_t     m_timer_fd;
    CChannel*   m_timer_channel;
    TimerList   m_timer_list;
};

