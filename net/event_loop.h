#ifndef __EVENT_LOOP_H
#define __EVENT_LOOP_H

#include <stdio.h>
#include <stdint.h>
#include "net_common.h"

using namespace std;

class CChannel;
class CEpoller;

class CEventLoop
{
public:
    CEventLoop();
    ~CEventLoop();

    void Loop(void);

    void UpdateChannel(CChannel* channel);

private:
    static const int32_t kEpollWaitTimeMs = 1000;
    
    CEpoller*   m_epoller;       
    bool        m_quit;
    ChannelList m_active_channels;
};

#endif // end of __EVENT_LOOP_H

