#ifndef __TCP_CONNECTION_H
#define __TCP_CONNECTION_H

#include <stdio.h>
#include <stdint.h>
#include "net_common.h"


class CBuffer;
class CEventLoop;
class CChannel;
class CTcpServer;

class CTcpConnection
{
public:
    CTcpConnection(CEventLoop* loop, int32_t connfd, CTcpServer* server);

    void Send(const char* buf, int32_t len);

    void SetMessageCallback(const MessageCallback& cb) { m_message_callback = cb; }
    
private:
    void RpcMsgCallback(void);
    void HandleRead(void);
    void HandleWrite(void);
    void HandleClose(void);

private:
    CEventLoop*     m_loop;
    CChannel*       m_channel;
    CTcpServer*     m_server;
    MessageCallback m_message_callback;

    CBuffer*        m_rbuf; // for tcp connection read event
    CBuffer*        m_wbuf; // for tcp connection write event
};

#endif // end of __TCP_CONNECTION_H
