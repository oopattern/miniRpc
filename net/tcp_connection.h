#ifndef __TCP_CONNECTION_H
#define __TCP_CONNECTION_H

#include <stdio.h>
#include <stdint.h>
#include "net_common.h"

class CEventLoop;
class CChannel;

class CTcpConnection
{
public:
    CTcpConnection(CEventLoop* loop, int32_t connfd);

    void Send(const char* buf, int32_t len);

    void SetMessageCallback(const MessageCallback& cb) { m_message_callback = cb; }

private:
    void HandleRead(void);
    void HandleWrite(void);
    void HandleClose(void);

private:
    CEventLoop*     m_loop;
    int32_t         m_connfd;
    CChannel*       m_channel;
    MessageCallback m_message_callback;
};

#endif // end of __TCP_CONNECTION_H
