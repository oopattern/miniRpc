#ifndef __TCP_CLIENT_H
#define __TCP_CLIENT_H

#include <stdio.h>
#include <stdint.h>
#include "net_common.h"

class CChannel;
class CEventLoop;
class CTcpConnection;

class CTcpClient
{
public:
    CTcpClient(CEventLoop* loop);
    ~CTcpClient();

    int32_t Connect(TEndPoint& server_addr);

    void SetMessageCallback(const MessageCallback& cb) { m_message_callback = cb; }

    int32_t RpcSendRecv(const char* send_buf, int32_t send_len, char* recv_buf, int32_t recv_max_size, int32_t timeout_ms);

private:
    void NewConnection(void);

private:
    int32_t         m_connfd;
    CEventLoop*     m_loop;
    CChannel*       m_channel;
    CTcpConnection* m_connection;
    MessageCallback m_message_callback;
};

#endif // end of __TCP_CLIENT_H

