#ifndef __ACCEPTOR_H
#define __ACCEPTOR_H

#include <stdio.h>

class CAcceptor
{
public:
    CAcceptor(CEventLoop* loop, TEndPoint listen_addr);

    int32_t Listen(void);

    bool IsListenning(void);

private:
    int32_t     m_accept_socket;
    bool        m_listenning;
    CChannel    m_accept_channel;
};

#endif // end of __ACCEPTOR_H
