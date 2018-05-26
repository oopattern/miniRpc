#include <stdio.h>


void CEchoServiceImpl::Echo(google::protobuf::RpcController* cntl,
                            const EchoRequest* request,
                            EchoResponse* response,
                            google::protobuf::Closure* done)
{
    printf("request msg: %s", request->message());
    response->set_message(request->message());
}
