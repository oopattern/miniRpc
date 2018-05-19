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
    CTcpServer(TEndPoint addr);

    void Start(void);

private:
    
};

#endif // end of __TCP_SERVER_H

