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
    if (m_connfd > 0)
    {
        ::close(m_connfd);
        m_connfd = -1;
    }
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

    // client connection finish, do register callback
    if (m_connection_callback)
    {
        m_connection_callback(m_connection);
    }
}



