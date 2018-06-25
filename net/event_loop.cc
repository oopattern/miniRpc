#include <stdio.h>
#include <sys/eventfd.h>
#include <unistd.h>
#include "epoll.h"
#include "channel.h"
#include "timer_queue.h"
#include "../base/public.h"
#include "../base/thread.h"
#include "event_loop.h"

CEventLoop::CEventLoop()
    : m_epoller(new CEpoller),
      m_quit(false),
      m_timer_queue(new CTimerQueue(this)),
      m_thread_id(CThread::Tid()),
      m_wakeup_fd(CreateEventFd()),
      m_wakeup_channel(new CChannel(this, m_wakeup_fd))
{
    if (m_wakeup_fd < 0)
    {
        printf("create wakeup fd error\n");
        ::exit(-1);
    }
    
    m_wakeup_channel->SetReadCallback(std::bind(&CEventLoop::HandleRead, this));
    m_wakeup_channel->EnableRead();
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

bool CEventLoop::IsInLoopThread(void)
{
    return m_thread_id == CThread::Tid();
}

int32_t CEventLoop::CreateEventFd(void)
{
    int32_t event_fd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (event_fd < 0)
    {
        printf("create event fd error:%s\n", ::strerror(errno));
        return ERROR;
    }

    return event_fd;
}

void CEventLoop::HandleRead(void)
{
    uint64_t u64 = 0;
    int32_t n = ::read(m_wakeup_fd, &u64, sizeof(u64));
    if (n != sizeof(u64))
    {
        printf("wakeup fd read error:%s\n", ::strerror(errno));
    }
}

void CEventLoop::WakeUp(void)
{
    uint64_t u64 = 1;
    int32_t n = ::write(m_wakeup_fd, &u64, sizeof(u64));
    if (n != sizeof(u64))
    {
        printf("wakeup fd write error:%s\n", ::strerror(errno));
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

