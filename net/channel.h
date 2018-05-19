#ifndef __CHANNEL_H
#define __CHANNEL_H

#include <stdio.h>

class CChannel
{
public:
    CChannel(int32_t fd);

    void EnableRead(void);

    void EnableWrite(void);

private:
    static const int32_t kNoneEvent;
    static const int32_t kReadEvent;
    static const int32_t kWriteEvent;

    int32_t m_fd;
    int32_t m_events;

};

#endif // end of __CHANNEL_H

