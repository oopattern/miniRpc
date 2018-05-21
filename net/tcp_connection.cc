#include <stdio.h>
#include <unistd.h> // read, close
#include "channel.h"
#include "event_loop.h"
#include "tcp_connection.h"


CTcpConnection::CTcpConnection(CEventLoop* loop, int32_t connfd)
    : m_loop(loop),
      m_connfd(connfd),
      m_channel(new CChannel(loop, connfd))
{
    m_channel->SetReadCallback(std::bind(&CTcpConnection::HandleRead, this));
    m_channel->SetWriteCallback(std::bind(&CTcpConnection::HandleWrite, this));
    m_channel->SetCloseCallback(std::bind(&CTcpConnection::HandleClose, this));
}

void CTcpConnection::HandleRead(void)
{
    printf("need to read socket\n");
    char buf[1024*100];
    int32_t nread = 0;

    nread = ::read(m_connfd, buf, sizeof(buf));
    if (nread > 0)
    {
        if (m_message_callback)
        {
            m_message_callback(this, buf, nread);
        }
    }
    else if (0 == nread)
    {
        HandleClose();
    }
}

void CTcpConnection::HandleWrite(void)
{
    printf("prepare to write socket\n");
}

void CTcpConnection::HandleClose(void)
{
    printf("prepare to close socket\n");
}
