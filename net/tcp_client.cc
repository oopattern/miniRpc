#include <stdio.h>
#include <unistd.h>     // close
#include <sys/socket.h> // socket, bind, connect, listen, accept
#include <netinet/in.h> // sockaddr_in, htons
#include <arpa/inet.h>  // inet_addr
#include <string.h>     // strerror
#include <strings.h>    // bzero
#include "channel.h"
#include "event_loop.h"
#include "tcp_connection.h"
#include "tcp_client.h"


CTcpClient::CTcpClient(CEventLoop* loop)
    : m_connfd(-1),
      m_loop(loop),
      m_channel(NULL),
      m_connection(NULL)
{
    
}

CTcpClient::~CTcpClient()
{
    ::close(m_connfd);
    //delete m_channel;
}

int32_t CTcpClient::Connect(TEndPoint& server_addr)
{
    // if socket is non block, connect may be return < 0, we should concern about write event
    // if socket is block, connect will block until build connection or timeout
    // care flag SOCK_NONBLOCK
    m_connfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if (m_connfd < 0)
    {
        printf("tcp client create socket error: %s\n", ::strerror(errno));
        return ERROR;
    }

    struct sockaddr_in inaddr;
    ::bzero(&inaddr, sizeof(inaddr));

    // if port not use htons, build connection will close at once, so why ???
    inaddr.sin_family = AF_INET;
    inaddr.sin_port = ::htons(server_addr.port);
    inaddr.sin_addr.s_addr = ::inet_addr(server_addr.ip);

    int32_t ret = ::connect(m_connfd, (struct sockaddr*)&inaddr, sizeof(struct sockaddr));

    // if socket in nonblocking, connection may not complete at once, 
    // just select socket for write event, and concern about SO_ERROR
    if ((ret < 0) && (errno != EINPROGRESS))
    {
        printf("tcp client connect error: %s\n", ::strerror(errno));
        return ERROR;
    }

    // not finish connect, NewConnection do the rest things
    m_channel = new CChannel(m_loop, m_connfd);       
    m_channel->SetWriteCallback(std::bind(&CTcpClient::NewConnection, this));
    m_channel->EnableWrite();

    return OK;
}

void CTcpClient::NewConnection(void)
{
    // check if connection ok...
    int32_t optval = -1;
    socklen_t optlen = sizeof(optval);

    if (0 > ::getsockopt(m_connfd, SOL_SOCKET, SO_ERROR, &optval, &optlen))
    {
        printf("tcp client connection failed error: %s\n", ::strerror(errno));
        return;
    }

    // cancel write event, otherwise will trigger connection all the time
    m_channel->DisableAll();
    m_channel->Remove();

    printf("tcp client connect ok, build new connection\n");   
    m_connection = new CTcpConnection(m_loop, m_connfd, NULL);

    // when client read event arrive, will call message callback
    m_connection->SetMessageCallback(m_message_callback);

    const char* hint = "new connection, hello oopattern\n";
    m_connection->Send(hint, ::strlen(hint));
}

int32_t CTcpClient::RpcSendRecv(const char* send_buf, 
                                int32_t send_len, 
                                char* recv_buf, 
                                int32_t recv_max_size, 
                                int32_t timeout_ms)
{
    if ((m_connfd < 0) || (NULL == m_connection))
    {
        printf("tcp client connection is invalid error\n");
        return ERROR;
    }

    // send packet to server until finish or fail
    int32_t nsend = 0;
    while (nsend < send_len)
    {
        int32_t n = ::send(m_connfd, send_buf + nsend, send_len - nsend, 0);
        if (n < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            
            if ((errno != EAGAIN) && (errno != EWOULDBLOCK))
            {
                printf("rpc send error: %s\n", ::strerror(errno));
                return ERROR;
            }
        }
        else
        {
            nsend += n;            
        }
    }

    // recv packet from server until timeout or fail
    int32_t nrecv = -1;
    while (nrecv <= 0)
    {
        nrecv = ::recv(m_connfd, recv_buf, recv_max_size, 0);
        if (nrecv < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }
            if (errno != EAGAIN && errno != EWOULDBLOCK)
            {
                printf("rpc recv error: %s\n", ::strerror(errno));
                return ERROR;
            }
        }
        else if (0 == nrecv)
        {
            printf("tcp server may shutdown read 0 error\n");
            return ERROR;
        }
    }

    return nrecv;
}

