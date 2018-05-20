#ifndef __TCP_SERVER_H
#define __TCP_SERVER_H

#include <stdio.h>

typedef struct _tEndPoint
{
    char    ip[32];
    int32_t port;
} TEndPoint;

class CTcpServer
{
public:
    CTcpServer(CEventLoop* loop, TEndPoint listen_addr);

    void Start(void);

private:
    CAcceptor* m_acceptor;
};

#endif // end of __TCP_SERVER_H

