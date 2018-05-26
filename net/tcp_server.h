#ifndef __TCP_SERVER_H
#define __TCP_SERVER_H

#include <stdio.h>
#include <stdint.h>
#include "net_common.h"

using namespace std;


class CTcpConnection;
class CAcceptor;
class CEventLoop;

typedef struct _tMethodProperty
{
    google::protobuf::Service* service;
    google::protobuf::MethodDescriptor* method;
} TMethodProperty;

// key: service_name + method_name, val: method_property
// one service may include many method
typedef std::map<std::string, TMethodProperty> MethodMap;


class CTcpServer
{
public:
    CTcpServer(CEventLoop* loop, TEndPoint& listen_addr);
    ~CTcpServer();

    void Start(void);    

    void NewConnection(int32_t connfd);

    void SetMessageCallback(const MessageCallback& cb) { m_message_callback = cb; }

    int32_t AddService(google::protobuf::Service* service);

private:
    CEventLoop*     m_loop;
    CAcceptor*      m_acceptor;
    ConnectionMap   m_connection_map;
    MessageCallback m_message_callback;
    MethodMap       m_method_map;
};

#endif // end of __TCP_SERVER_H

