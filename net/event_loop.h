#ifndef __EVENT_LOOP_H
#define __EVENT_LOOP_H

#include <stdio.h>
#include <stdint.h>
#include "net_common.h"

using namespace std;

class CChannel;
class CEpoller;
class CTimerQueue;

class CEventLoop
{
public:
    CEventLoop();
    ~CEventLoop();

    // loop operation
    void Loop(void);
    bool IsInLoopThread(void);

    // wakeup fd operation
    void HandleRead(void);
    void WakeUp(void);

    // socket channel operation
    void UpdateChannel(CChannel* channel);
    void RemoveChannel(CChannel* channel);

    // timer queue operation
    int32_t RunAfter(int32_t delay_ms, const TimerCallback& cb);
    int32_t RunEvery(int32_t interval_ms, const TimerCallback& cb);
    int32_t CancelTimer(int32_t timer_seq);

public:
    static int32_t CreateEventFd(void);

private:
    static const int32_t kEpollWaitTimeMs = 1000;
    
    CEpoller*    m_epoller;       
    bool         m_quit;
    CTimerQueue* m_timer_queue;
    ChannelList  m_active_channels;
    pid_t        m_thread_id;
    int32_t      m_wakeup_fd;
    CChannel*    m_wakeup_channel;
};

#endif // end of __EVENT_LOOP_H

