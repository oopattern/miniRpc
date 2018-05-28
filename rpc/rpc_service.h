#ifndef __RPC_SERVICE_H
#define __RPC_SERVICE_H

#include <stdio.h>
#include "echo.pb.h"


class CEchoServiceImpl : public CEchoService 
{
public:
    CEchoServiceImpl() {}
    virtual ~CEchoServiceImpl() {}

    virtual void Echoxxx(google::protobuf::RpcController* cntl,
                      const EchoRequest* request,
                      EchoResponse* response,
                      google::protobuf::Closure* done);
};

#endif
