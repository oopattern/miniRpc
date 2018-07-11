#include <stdio.h>
#include "tcp_server.h"
#include "net_common.h"


class CHttpServer
{
public:
    CHttpServer(CEventLoop* loop, TEndPoint& addr);
    ~CHttpServer();

    void Start(void);
    void SetHttpCallback(const HttpCallback& cb) { m_http_callback = cb; }

private:
    void OnMessage(CTcpConnection* conn_ptr, char* buf, int32_t len);

private:
    CTcpServer      m_server;    
    HttpCallback    m_http_callback;
};
