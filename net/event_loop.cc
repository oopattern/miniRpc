#include <stdio.h>
#include "epoll.h"
#include "channel.h"
#include "event_loop.h"

CEventLoop::CEventLoop()
    : m_epoller(new CEpoller),
      m_quit(false)
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
