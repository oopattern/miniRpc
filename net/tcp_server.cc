#include <stdio.h>
#include <google/protobuf/descriptor.h> // ServiceDescriptor
#include "acceptor.h"
#include "event_loop.h"
#include "../base/thread.h"
#include "tcp_connection.h"
#include "tcp_server.h"


CTcpServer::CTcpServer(CEventLoop* loop, TEndPoint& listen_addr)
    : m_base_loop(loop),
      m_acceptor(new CAcceptor(m_base_loop, listen_addr))
{
    m_next_loop = 0;
    m_io_thread_vec.clear();
    m_io_loop_vec.clear();
    
    m_acceptor->SetNewConnectionCallback(std::bind(&CTcpServer::NewConnection, this, _1));
}

CTcpServer::~CTcpServer()
{
    // close connection
    // delete acceptor
}

void CTcpServer::IoLoopFunc(int32_t loop_idx)
{
    assert(loop_idx < m_io_loop_vec.size());
    CEventLoop* loop = m_io_loop_vec[loop_idx];
    loop->Loop();
}

CEventLoop* CTcpServer::GetNextLoop(void)
{
    if (m_io_loop_vec.empty())
    {
        //printf("GetNextLoop connection use base loop\n");
        return m_base_loop;
    }

    assert(m_next_loop < m_io_loop_vec.size());

    CEventLoop* next_loop = m_io_loop_vec[m_next_loop++];
    printf("GetNextLoop connection use io loop=%d\n", m_next_loop);
    if (m_next_loop >= m_io_loop_vec.size())
    {
        m_next_loop = 0;
    }

    return next_loop;
}

void CTcpServer::SetThreadNum(int32_t num_threads)
{
    for (int32_t i = 0; i < num_threads; ++i)
    {
        // first, create loop, thread func will access loop, take care create loop first
        CEventLoop* io_loop = new CEventLoop();
        m_io_loop_vec.push_back(io_loop);
        // second, create thread
        CThread* io_thread = new CThread(std::bind(&CTcpServer::IoLoopFunc, this, i));
        m_io_thread_vec.push_back(io_thread);
    }
}

void CTcpServer::Start(void)
{
    // start io thread loop
    assert(m_io_thread_vec.size() == m_io_loop_vec.size());
    ThreadList::iterator it;
    for (it = m_io_thread_vec.begin(); it != m_io_thread_vec.end(); ++it)
    {
        CThread* thread = *it;
        thread->Start();
    }

    // base thread loop for listen
    m_acceptor->Listen();   
}

void CTcpServer::NewConnection(int32_t connfd)
{
    // find pthread to handle this connection
    //printf("build new connection\n");

    CEventLoop* io_loop = GetNextLoop();

    // build connection
    std::string name = std::to_string(connfd);
    CTcpConnection* conn_ptr = new CTcpConnection(io_loop, connfd, this);

    // attach message callback
    conn_ptr->SetMessageCallback(m_message_callback);
    
    m_connection_map[name] = conn_ptr;
}

int32_t CTcpServer::FindService(const std::string& full_name, TMethodProperty& method_property)
{
    MethodMap::iterator iter = m_method_map.find(full_name);
    if (iter == m_method_map.end())
    {
        printf("RPC find full_name: %s error\n", full_name.c_str());
        return ERROR;
    }

    method_property.service = iter->second.service;
    method_property.method = iter->second.method;
    return OK;
}

int32_t CTcpServer::AddService(google::protobuf::Service* service)
{
    if (NULL == service)
    {
        printf("register service is NULL error\n");
        return ERROR;
    }

    const google::protobuf::ServiceDescriptor* sd = service->GetDescriptor();

    if (0 == sd->method_count())
    {
        printf("service: %s no methods error\n", sd->full_name().c_str());
        return ERROR;
    }

    for (int32_t i = 0; i < sd->method_count(); ++i)
    {
        // key: service_name : method_name
        const google::protobuf::MethodDescriptor* md = sd->method(i);
        std::string key = sd->full_name() + ":" + md->name();
        
        if (m_method_map.find(key) != m_method_map.end())
        {
            printf("service: %s method: %s already exists error\n", sd->full_name().c_str(), md->full_name().c_str());
            return ERROR;
        }

        // register service and method
        TMethodProperty mp;
        mp.service = service;
        mp.method = md;
        m_method_map[key] = mp;
        printf("Tcp Server register service: %s success\n", key.c_str());
    }

    return OK;
}


