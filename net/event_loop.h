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

    void Loop(void);

    void UpdateChannel(CChannel* channel);
    void RemoveChannel(CChannel* channel);

    int32_t RunAfter(int32_t delay, const TimerCallback& cb);
    int32_t RunEvery(int32_t interval, const TimerCallback& cb);
    int32_t CancelTimer(int32_t timer_seq);

private:
    static const int32_t kEpollWaitTimeMs = 1000;
    
    CEpoller*    m_epoller;       
    bool         m_quit;
    CTimerQueue* m_timer_queue;
    ChannelList  m_active_channels;
};

#endif // end of __EVENT_LOOP_H

