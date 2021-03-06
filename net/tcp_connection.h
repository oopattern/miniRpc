#ifndef __TCP_CONNECTION_H
#define __TCP_CONNECTION_H

#include <stdio.h>
#include <stdint.h>
#include "net_common.h"
#include "../rpc/rpc_coroutine.h"


class CBuffer;
class CEventLoop;
class CChannel;
class CTcpServer;
class CRpcCoroutine;


class CTcpConnection
{
public:
    CTcpConnection(CEventLoop* loop, int32_t connfd, CTcpServer* server);

    void ShutDown(void);
    void Send(const char* buf, int32_t len);
    void SetMessageCallback(const MessageCallback& cb) { m_message_callback = cb; }
    CEventLoop* GetLoop(void) const { return m_loop; }

    // client rpc operation
    int32_t RegisterCoroutine(int32_t coroutine_id, CRpcCoroutine* co);
    int32_t DestroyCoroutine(int32_t coroutine_id);
    int32_t TimeoutCoroutine(int32_t coroutine_id);

    int32_t RpcSendRecv(const char* send_buf, int32_t send_len, char* recv_buf, int32_t recv_max_size, int32_t timeout_ms);

private:
    // rpc operation
    void RpcClientMsg(const char* recv_buf, int32_t recv_len);
    void RpcServerMsg(const char* recv_buf, int32_t recv_len);

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

    CoroutineMap    m_coroutine_map;
};

#endif // end of __TCP_CONNECTION_H
