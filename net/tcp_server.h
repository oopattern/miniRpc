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

    // register rpc service
    int32_t AddService(google::protobuf::Service* service);

    // full_name: server_name:method_name
    int32_t FindService(const std::string& full_name, TMethodProperty& method_property);
private:
    CEventLoop*     m_loop;
    CAcceptor*      m_acceptor;
    ConnectionMap   m_connection_map;
    MessageCallback m_message_callback;
    MethodMap       m_method_map;
};

#endif // end of __TCP_SERVER_H

