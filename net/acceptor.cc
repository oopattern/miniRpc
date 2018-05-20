#include <stdio.h>

CAcceptor::CAcceptor(CEventLoop* loop, TEndPoint listen_addr) 
    : m_accept_socket(::socket(family, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP)),
      m_listenning(false),
      m_accept_channel(loop, m_accept_socket)
{
    if (m_accept_socket < 0)
    {
        printf("acceptor creat error: %s\n", ::strerror(errno));
        ::exit(-1);
    }

    int32_t option = 1;
    ::setsockopt(m_accept_socket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    struct sockaddr* addr;        
    if (0 > ::bind(m_accept_socket, addr, sizeof(struct sockaddr)))
    {
        printf("acceptor bind error: %s\n", ::strerror(errno));
        ::exit(-1);
    }
}

int CAcceptor::Listen(void)
{
    if (m_accept_socket < 0)
    {
        printf("acceptor socket init error\n");
        return ERROR;
    }

    if (0 > ::listen(m_accept_socket, SOMAXCONN))
    {
        printf("acceptor listen error: %s\n", ::strerror(errno));
        return ERROR;
    }

    m_listenning = true;
    m_accept_channel.EnableRead();
    return OK;
}

bool CAcceptor::IsListenning(void)
{
    return m_listenning;
}
