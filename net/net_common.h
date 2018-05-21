#ifndef __NET_COMMON_H
#define __NET_COMMON_H

#include <stdio.h>
#include <map>
#include <vector>
#include <string>
#include <functional>

using std::map;
using std::string;
using std::vector;
using std::function;
using namespace std::placeholders;

#ifndef ERROR
#define ERROR   -1
#endif

#ifndef OK
#define OK      0
#endif

typedef struct _tEndPoint
{
    char    ip[32];
    int32_t port;
} TEndPoint;

class CChannel;
class CTcpConnection;

typedef std::vector<CChannel*> ChannelList;
typedef std::map<std::string, CTcpConnection*> ConnectionMap;

typedef std::function<void()> EventCallback;
typedef std::function<void(int32_t connfd)> NewConnectionCallback;
typedef std::function<void(CTcpConnection* conn_ptr, char* buf, int32_t len)> MessageCallback;


#endif
