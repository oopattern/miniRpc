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
    void SetThreadNum(int32_t num_threads);
    void SetMessageCallback(const MessageCallback& cb) { m_message_callback = cb; }

    // register rpc service
    int32_t AddService(google::protobuf::Service* service);
    
    // full_name: server_name:method_name
    int32_t FindService(const std::string& full_name, TMethodProperty& method_property);

private:
    void NewConnection(int32_t connfd);
    CEventLoop* GetNextLoop(void);
    void IoLoopFunc(int32_t loop_idx);   

private:
    CEventLoop*     m_base_loop;
    CAcceptor*      m_acceptor;
    ConnectionMap   m_connection_map;
    MessageCallback m_message_callback;
    MethodMap       m_method_map;

    // use for io thread loop
    ThreadList      m_io_thread_vec;
    LoopList        m_io_loop_vec;
    int32_t         m_next_loop;
};

#endif // end of __TCP_SERVER_H

