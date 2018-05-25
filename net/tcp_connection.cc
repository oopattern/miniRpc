#include <stdio.h>
#include <unistd.h> // read, close
#include <string.h> // strerror
#include <errno.h>  // errno
#include "channel.h"
#include "event_loop.h"
#include "buffer.h"
#include "tcp_connection.h"


CTcpConnection::CTcpConnection(CEventLoop* loop, int32_t connfd)
    : m_loop(loop),
      m_channel(new CChannel(loop, connfd)),
      m_rbuf(new CBuffer),
      m_wbuf(new CBuffer)
{
    m_channel->SetReadCallback(std::bind(&CTcpConnection::HandleRead, this));
    m_channel->SetWriteCallback(std::bind(&CTcpConnection::HandleWrite, this));
    m_channel->SetCloseCallback(std::bind(&CTcpConnection::HandleClose, this));

    // enable socket read
    m_channel->EnableRead();
}

void CTcpConnection::HandleRead(void)
{
    //printf("need to read socket\n");
    char buf[1024*100];
    int32_t nread = 0;

    nread = ::read(m_channel->Fd(), buf, sizeof(buf));    
    if (nread > 0)
    {
        m_rbuf->Append(buf, nread);
        if (m_message_callback)
        {
            m_message_callback(this, (char*)m_rbuf->Data(), m_rbuf->Remain());
        }
        m_rbuf->Skip(nread);
    }
    else if (0 == nread)
    {
        HandleClose();
    }
}

void CTcpConnection::HandleWrite(void)
{
    //printf("prepare to write socket\n");

    if (! m_channel->IsWriteable())
    {
        printf("tcp connection channel down, no more writing\n");
        return;
    }

    if (m_wbuf->Remain() <= 0)
    {
        printf("tcp connection channel nothing to write error\n");
        return;
    }

    int32_t nwrite = ::write(m_channel->Fd(), m_wbuf->Data(), m_wbuf->Remain());
    if (nwrite > 0)
    {
        m_wbuf->Skip(nwrite);

        // finish write content, disable epoll writeable
        if (m_wbuf->Remain() <= 0)
        {
            m_channel->DisableWrite();
        }
    }
    else
    {
        printf("tcp connection write socket error: %s\n", ::strerror(errno));    
    }
}

void CTcpConnection::HandleClose(void)
{
    printf("prepare to close socket\n");
    ::exit(-1);
}

void CTcpConnection::Send(const char* buf, int32_t len)
{
    int32_t nwrite = 0;
    int32_t remaining = len;   

    // nothing in output queue, try to write directly
    if (m_channel->IsWriteable() && (0 == m_wbuf->Remain()))
    {
        nwrite = ::write(m_channel->Fd(), buf, len);
        if (nwrite > 0)
        {
            remaining = len - nwrite;
            m_wbuf->Skip(nwrite);            
        }
        else
        {
            printf("tcp connection write error: %s\n", ::strerror(errno));
            return;
        }
    }

    // handle rest content to write
    if (remaining > 0)
    {
        m_wbuf->Append(buf+nwrite, remaining);
        if (! m_channel->IsWriteable())
        {
            m_channel->EnableWrite();
        }
    }
}


