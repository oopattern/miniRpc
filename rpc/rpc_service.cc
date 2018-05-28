#include <stdio.h>
#include "rpc_service.h"


void CEchoServiceImpl::Echoxxx(google::protobuf::RpcController* cntl,
                            const EchoRequest* request,
                            EchoResponse* response,
                            google::protobuf::Closure* done)
{
    printf("request msg: %s", request->message().c_str());
    response->set_message(request->message());
}
