#include <stdio.h>
#include <poll.h>   // POLLIN
#include <string.h> // strerror
#include <errno.h>  // errno
#include "channel.h"
#include "event_loop.h"


const int32_t CChannel::kNoneEvent = 0;
const int32_t CChannel::kReadEvent = POLLIN | POLLPRI;
const int32_t CChannel::kWriteEvent = POLLOUT;


CChannel::CChannel(CEventLoop* loop, int32_t fd)
    : m_fd(fd),
      m_events(0),
      m_ready_events(kNoneEvent),
      m_stat(kNew),
      m_loop(loop)
{

}

void CChannel::EnableRead(void)
{
    m_events |= kReadEvent;
    Update();
}

void CChannel::DisableRead(void)
{
    m_events &= ~kReadEvent;
    Update();
}

void CChannel::EnableWrite(void)
{
    m_events |= kWriteEvent;
    Update();
}

void CChannel::DisableWrite(void)
{
    m_events &= ~kWriteEvent;
    Update();
}

void CChannel::DisableAll(void)
{
    m_events = kNoneEvent;
    Update();
}

void CChannel::Update(void)
{
    m_loop->UpdateChannel(this);
}

void CChannel::Remove(void)
{
    m_loop->RemoveChannel(this);
}

void CChannel::HandleEvent(void)
{
    //printf("epoll ready event=%d happened\n", m_ready_events);   

    // registered read event
    if (m_ready_events & (POLLIN | POLLPRI))
    {
        printf("fd=%d epoll read event\n", m_fd);
        if (m_read_callback) 
        {
            m_read_callback();
        }
    }

    // registered write event
    if (m_ready_events & POLLOUT)
    {
        printf("fd=%d epoll write event\n", m_fd);
        if (m_write_callback)
        {
            m_write_callback();
        }
    }

    // registered error event
    if (m_ready_events & (POLLERR | POLLNVAL))
    {
        printf("fd=%d epoll error event: %s\n", m_fd, ::strerror(errno));
        if (m_error_callback)
        {
            m_error_callback();
        }
    }
}



