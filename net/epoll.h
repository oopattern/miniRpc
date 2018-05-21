#ifndef __EPOLL_H
#define __EPOLL_H

#include <stdio.h>
#include <stdint.h>

class CChannel;
class ChannelList;
struct epoll_event;
typedef std::vector<struct epoll_event> EventList;

class CEpoller
{
public:
    CEpoller();

    void WaitEvent(int32_t timeout_ms, ChannelList& active_channels);

    void UpdateChannel(CChannel* channel);

private:
    static const int32_t kInitEventSize = 32;
    
    int32_t     m_epollfd;
    EventList   m_events;
};

#endif // end of __EPOLL_H
