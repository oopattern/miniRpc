#ifndef __TCP_SERVER_H
#define __TCP_SERVER_H

#include <stdio.h>
#include <stdint.h>
#include "net_common.h"

using namespace std;


class CTcpConnection;
class CAcceptor;
class CEventLoop;


class CTcpServer
{
public:
    CTcpServer(CEventLoop* loop, TEndPoint& listen_addr);
    ~CTcpServer();

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

