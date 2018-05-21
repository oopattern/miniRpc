#ifndef __TCP_SERVER_H
#define __TCP_SERVER_H

#include <stdio.h>
#include <stdint.h>


class CTcpConnection;
class CAcceptor;
class CEventLoop;

typedef std::map<string, CTcpConnection*> ConnectionMap;
typedef std::function<void(CTcpConnection* conn_ptr, char* buf, int32_t len)> MessageCallback;

typedef struct _tEndPoint
{
    char    ip[32];
    int32_t port;
} TEndPoint;

class CTcpServer
{
public:
    CTcpServer(CEventLoop* loop, TEndPoint& listen_addr);

    void Start(void);

    void NewConnection(int32_t connfd);

    void SetMessageCallback(const MessageCallback& cb) { m_message_callback = cb; }

private:
    CEventLoop*     m_loop;
    CAcceptor*      m_acceptor;
    ConnectionMap   m_connection_map;
    MessageCallback m_message_callback;
};

#endif // end of __TCP_SERVER_H

