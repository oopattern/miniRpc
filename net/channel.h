#ifndef __CHANNEL_H
#define __CHANNEL_H

#include <stdio.h>

class CChannel
{
public:
    CChannel(CEventLoop* loop, int32_t fd);

    void EnableRead(void);
    void EnableWrite(void);
    void DisableRead(void);
    void DisableWrite(void);

    int32_t Fd(void) const { return m_fd; }
    int32_t Events(void) const { return m_events; }
    void SetReadyEvent(int32_t val) { m_ready_events = val; }
    void HandleEvent(void);

private:
    void Update(void);

private:
    static const int32_t kNoneEvent;
    static const int32_t kReadEvent;
    static const int32_t kWriteEvent;

    int32_t m_fd;
    int32_t m_events;
    int32_t m_ready_events; // ready event from epoll_wait

    CEventLoop* m_loop;
};

typedef std::vector<CChannel*> ChannelList;

#endif // end of __CHANNEL_H

