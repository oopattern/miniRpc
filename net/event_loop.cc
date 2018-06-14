#include <stdio.h>
#include "epoll.h"
#include "channel.h"
#include "timer_queue.h"
#include "../base/public.h"
#include "event_loop.h"

CEventLoop::CEventLoop()
    : m_epoller(new CEpoller),
      m_quit(false),
      m_timer_queue(new CTimerQueue(this))
{

}

CEventLoop::~CEventLoop()
{

}

void CEventLoop::Loop(void)
{
    m_quit = false;

    while (!m_quit)
    {
        m_active_channels.clear();
        m_epoller->WaitEvent(kEpollWaitTimeMs, m_active_channels);

        ChannelList::iterator it;
        for (it = m_active_channels.begin(); it != m_active_channels.end(); ++it)
        {
            CChannel* channel = *it;
            channel->HandleEvent();
        }
    }
}

void CEventLoop::UpdateChannel(CChannel* channel)
{
    m_epoller->UpdateChannel(channel);   
}

void CEventLoop::RemoveChannel(CChannel* channel)
{
    m_epoller->RemoveChannel(channel);
}

int32_t CEventLoop::RunAfter(int32_t delay_ms, const TimerCallback& cb)
{
    int64_t when_ms = CUtils::NowMsec() + delay_ms;
    return m_timer_queue->AddTimer(when_ms, 0, cb);
}

int32_t CEventLoop::RunEvery(int32_t interval_ms, const TimerCallback& cb)
{
    int64_t when_ms = CUtils::NowMsec() + interval_ms;
    return m_timer_queue->AddTimer(when_ms, interval_ms, cb);
}

int32_t CEventLoop::CancelTimer(int32_t timer_seq)
{
    return m_timer_queue->CancelTimer(timer_seq);
}

