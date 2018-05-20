#ifndef __EVENT_LOOP_H
#define __EVENT_LOOP_H

#include <stdio.h>

class CEventLoop
{
public:
    CEventLoop();

    void Loop(void);

    void UpdateChannel(CChannel* channel);

private:
    static const int32_t kEpollWaitTimeMs = 1000;
    CEpoller*   m_epoller;       
    bool        m_quit;
    ChannelList m_active_channels;
};

#endif // end of __EVENT_LOOP_H

