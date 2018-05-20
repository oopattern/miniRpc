#include <stdio.h>


const int32_t CChannel::kNoneEvent = 0;
const int32_t CChannel::kReadEvent = POLLIN | POLLPRI;
const int32_t CChannel::kWriteEvent = POLLOUT;


CChannel::CChannel(CEventLoop* loop, int32_t fd)
    : m_fd(fd),
      m_events(0),
      m_ready_events(kNoneEvent),
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

void CChannel::HandleEvent(void)
{
    printf("epoll event happened\n");   
}

void CChannel::Update(void)
{
    m_loop->UpdateChannel(this);
}


