#include <stdio.h>
#include "epoll.h"
#include "channel.h"
#include "timer_queue.h"
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

int32_t CEventLoop::RunAfter(int32_t delay, const TimerCallback& cb)
{
    int32_t when = ::time(NULL) + delay;
    return m_timer_queue->AddTimer(when, 0, cb);
}

int32_t CEventLoop::RunEvery(int32_t interval, const TimerCallback& cb)
{
    int32_t when = ::time(NULL) + interval;
    return m_timer_queue->AddTimer(when, interval, cb);
}

int32_t CEventLoop::CancelTimer(int32_t timer_seq)
{
    return m_timer_queue->CancelTimer(timer_seq);
}

