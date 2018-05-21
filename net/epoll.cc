#include <stdio.h>
#include <assert.h>     // assert
#include <unistd.h>     // close
#include <string.h>     // strerror    
#include <strings.h>    // bzero
#include <sys/epoll.h>  // epoll
#include "net_common.h"
#include "channel.h"
#include "epoll.h"


CEpoller::CEpoller()
{
    m_epollfd = ::epoll_create(102400);
    m_events.resize(kInitEventSize);

    if (m_epollfd < 0)
    {
        printf("epoll create error: %s\n", ::strerror(errno));
        ::exit(-1);
    }
}

CEpoller::~CEpoller()
{
    ::close(m_epollfd);
}

void CEpoller::WaitEvent(int32_t timeout_ms, ChannelList& active_channels)
{
    int32_t num_events = ::epoll_wait(m_epollfd, &*(m_events.begin()), m_events.size(), timeout_ms);

    if (num_events <= 0)
    {
        if ((num_events < 0) && (errno != EINTR))
        {
            printf("epoll wait error: %s\n", ::strerror(errno));
        }
        return;
    }

    assert(num_events <= m_events.size());
    for (int32_t i = 0; i < num_events; ++i)
    {
        CChannel* channel = (CChannel*)m_events[i].data.ptr;
        channel->SetReadyEvent(m_events[i].events);
        active_channels.push_back(channel);
    }

    // resize epoll_event
    if (num_events == m_events.size())
    {
        m_events.resize(num_events*2);
    }
}

int32_t CEpoller::UpdateChannel(CChannel* channel)
{
    struct epoll_event event;
    ::bzero(&event, sizeof(event));

    int32_t fd = channel->Fd();      
    event.events = channel->Events();
    event.data.ptr = channel;

    // temporary just add 
    if (0 > ::epoll_ctl(m_epollfd, EPOLL_CTL_ADD, fd, &event))
    {
        printf("epoll ctl error: %s\n", ::strerror(errno));
        return ERROR;
    }

    return OK;
}
