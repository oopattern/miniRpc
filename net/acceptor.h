#ifndef __ACCEPTOR_H
#define __ACCEPTOR_H

#include <stdio.h>
#include <stdint.h>
#include "net_common.h"


class CEventLoop;
class CChannel;

class CAcceptor
{
public:
    CAcceptor(CEventLoop* loop, TEndPoint& listen_addr);

    int32_t Listen(void);

    bool IsListenning(void);

    void SetNewConnectionCallback(const NewConnectionCallback& cb)
    {
        m_new_connection_callback = cb;
    }

private:
    void HandleRead(void);

private:
    int32_t     m_accept_socket;
    bool        m_listenning;
    CChannel*   m_accept_channel;
    NewConnectionCallback m_new_connection_callback;
};

#endif // end of __ACCEPTOR_H
