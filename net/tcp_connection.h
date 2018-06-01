#ifndef __TCP_CONNECTION_H
#define __TCP_CONNECTION_H

#include <stdio.h>
#include <stdint.h>
#include "net_common.h"


class CBuffer;
class CEventLoop;
class CChannel;
class CTcpServer;
class CRpcCoroutine;

class CTcpConnection
{
public:
    CTcpConnection(CEventLoop* loop, int32_t connfd, CTcpServer* server);

    void Send(const char* buf, int32_t len);

    void SetMessageCallback(const MessageCallback& cb) { m_message_callback = cb; }

    // client rpc operation
    int32_t RpcSendRecv(const char* send_buf, int32_t send_len, char* recv_buf, int32_t recv_max_size, int32_t timeout_ms);
    int32_t RpcClientYield(void);
    int32_t RpcClientResume(void);    
    int32_t RpcClientMsg(std::vector<char>& recv_data);

private:
    void RpcServerMsg(void);
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

    CRpcCoroutine*  m_coroutine;        
};

#endif // end of __TCP_CONNECTION_H
