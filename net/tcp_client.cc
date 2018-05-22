#include <stdio.h>
#include <unistd.h>     // close
#include <sys/socket.h> // socket, bind, connect, listen, accept
#include <netinet/in.h> // sockaddr_in, htons
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
    m_connfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if (m_connfd < 0)
    {
        printf("tcp client create socket error: %s\n", ::strerror(errno));
        return ERROR;
    }

    struct sockaddr_in inaddr;
    ::bzero(&inaddr, sizeof(inaddr));

    inaddr.sin_family = AF_INET;
    inaddr.sin_port = server_addr.port;

    int32_t ret = ::connect(m_connfd, (struct sockaddr*)&inaddr, sizeof(struct sockaddr));
    if (ret < 0)
    {
        printf("tcp client connect error: %s\n", ::strerror(errno));
        return ERROR;
    }

    m_channel = new CChannel(m_loop, m_connfd);       
    m_channel->SetWriteCallback(std::bind(&CTcpClient::NewConnection, this));
    m_channel->EnableWrite();

    return OK;
}

void CTcpClient::NewConnection(void)
{
    printf("tcp client connect ok, build new connection\n");   
    m_connection = new CTcpConnection(m_loop, m_connfd);

    // when client read event arrive, will call message callback
    m_connection->SetMessageCallback(m_message_callback);
}


