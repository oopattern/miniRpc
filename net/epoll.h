#ifndef __EPOLL_H
#define __EPOLL_H

#include <stdio.h>
#include <stdint.h>
#include <vector>
#include "net_common.h"

using std::vector;

class CChannel;
struct epoll_event;

typedef std::vector<struct epoll_event> EventList;


class CEpoller
{
public:
    CEpoller();
    ~CEpoller();

    void WaitEvent(int32_t timeout_ms, ChannelList& active_channels);

    int32_t UpdateChannel(CChannel* channel);
    int32_t RemoveChannel(CChannel* channel);

private:
    static const int32_t kInitEventSize = 32;
    
    int32_t     m_epollfd;
    EventList   m_events;
    ChannelMap  m_channel_map;
};

#endif // end of __EPOLL_H
