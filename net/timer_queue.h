#include <stdio.h>
#include <stdint.h>
#include "net_common.h"


class CChannel;
class CEventLoop;


class CTimer
{
public:
    CTimer(int32_t when, const TimerCallback& cb)
        : m_expiration(when),
          m_callback(cb),
          m_repeat(false)
    {
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

    bool Repeat(void) const 
    {
        return m_repeat;
    }

    void Run(void)
    {
        m_callback();
    }

private:    
    int32_t         m_expiration;
    TimerCallback   m_callback;
    bool            m_repeat;
    int32_t         m_sequence;
    
    static int32_t  m_num_create;    
};

typedef std::map<int32_t, CTimer*> TimerList;

class CTimerQueue
{
public:
    CTimerQueue(CEventLoop* loop);
    ~CTimerQueue();

    int32_t AddTimer(int32_t when, const TimerCallback& cb);    
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
