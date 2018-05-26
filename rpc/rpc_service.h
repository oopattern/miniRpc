#ifndef __RPC_SERVICE_H
#define __RPC_SERVICE_H

#include <stdio.h>

class CEchoServiceImpl : public CEchoService
{
public:
    CEchoServiceImpl() {}
    virtual ~CEchoServiceImpl() {}

    virtual void Echo(google::protobuf::RpcController* cntl,
                      const EchoRequset* request,
                      EchoResponse* response,
                      google::protobuf::Closure* done);
};

#endif
