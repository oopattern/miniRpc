#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h> // socket, bind, connect, listen, accept
#include <netinet/in.h> // sockaddr_in, htons
#include <errno.h>      // errno
#include <string.h>     // snprintf, memset
#include <strings.h>    // bzero
#include "event_loop.h"
#include "channel.h"
#include "acceptor.h"


CAcceptor::CAcceptor(CEventLoop* loop, TEndPoint& listen_addr) 
    : m_accept_socket(::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP)),
      m_listenning(false),
      m_accept_channel(new CChannel(loop, m_accept_socket))
{
    if (m_accept_socket < 0)
    {
        printf("acceptor creat error: %s\n", ::strerror(errno));
        ::exit(-1);
    }

    int32_t option = 1;
    ::setsockopt(m_accept_socket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    struct sockaddr_in inaddr;
    inaddr.sin_family = AF_INET;
    inaddr.sin_port = ::htons(listen_addr.port);
    
    if (0 > ::bind(m_accept_socket, (struct sockaddr*)&inaddr, sizeof(struct sockaddr)))
    {
        printf("acceptor bind error: %s\n", ::strerror(errno));
        ::exit(-1);
    }

    // bind read event(new connection)
    m_accept_channel->SetReadCallback(std::bind(&CAcceptor::HandleRead, this));
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
    m_accept_channel->EnableRead();
    return OK;
}

bool CAcceptor::IsListenning(void)
{
    return m_listenning;
}

// accept channel read event (new connection)
void CAcceptor::HandleRead(void)
{
    struct sockaddr_in inaddr;
    ::bzero(&inaddr, sizeof(inaddr));    
    socklen_t addrlen = sizeof(inaddr);

    int32_t connfd = ::accept(m_accept_socket, (struct sockaddr*)&inaddr, &addrlen);
    if (connfd <= 0)
    {
        if (EINTR == errno)
        {
            printf("accept connect error: %s\n", ::strerror(errno));
        }
        else if (EAGAIN == errno)
        {
            printf("[Fatal] connect error: %s\n", ::strerror(errno));
        }
        return;
    }
    else
    {
        printf("new connection arrive\n");
        if (m_new_connection_callback)
        {
            m_new_connection_callback(connfd);
        }
    }
}

