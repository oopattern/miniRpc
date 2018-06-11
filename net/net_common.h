#ifndef __NET_COMMON_H
#define __NET_COMMON_H

#include <stdio.h>
#include <map>
#include <vector>
#include <string>
#include <functional>
#include <google/protobuf/service.h>

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

class CChannel;
class CTcpConnection;
class CRpcCoroutine;
class CThread;
class CEventLoop;


typedef enum 
{
    kNew    = 0,
    kAdded  = 1,
    kDeleted= 2,
} EChannelStat;

typedef struct _tEndPoint
{
    char    ip[32];
    int32_t port;
} TEndPoint;

typedef struct _tMethodProperty
{
    google::protobuf::Service* service;
    const google::protobuf::MethodDescriptor* method;
} TMethodProperty;

typedef std::vector<CThread*>    ThreadList;
typedef std::vector<CEventLoop*> LoopList;
typedef std::map<int, CChannel*> ChannelMap; // key: channel fd, val: channel point
typedef std::vector<CChannel*>   ChannelList; 
typedef std::map<std::string, CTcpConnection*> ConnectionMap; // key: connection name, val: connection point
typedef std::map<int32_t, CRpcCoroutine*> CoroutineMap; // key: co_id, val: coroutine point

// key: service_name + method_name, val: method_property
// one service may include many method
typedef std::map<std::string, TMethodProperty> MethodMap;

typedef std::function<void()> EventCallback;
typedef std::function<void(int32_t connfd)> NewConnectionCallback; // use for accept new connection
typedef std::function<void(CTcpConnection* conn_ptr)> ConnectionCallback; // use for client connect with server
typedef std::function<void(CTcpConnection* conn_ptr, char* buf, int32_t len)> MessageCallback;


#endif
