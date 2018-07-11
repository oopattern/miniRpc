#include <stdio.h>
#include "../base/thread.h"
#include "../net/http_server.h"
#include "../net/event_loop.h"


class CTestHttp
{
public:
    static void TestHttpServer(void);
    static void TestHttpCallback(const std::string& request, std::string& response);
};

void CTestHttp::TestHttpCallback(const std::string& request, std::string& response)
{
    printf("http request: %s\n", request.c_str());   
    response =  "HTTP/1.1 200 OK\r\n"
                "Connection: Keep-Alive\r\n";
    printf("http response: %s\n", response.c_str());
}

void CTestHttp::TestHttpServer(void)
{
    TEndPoint listen_addr;
    snprintf(listen_addr.ip, sizeof(listen_addr.ip), "127.0.0.1");
    listen_addr.port = 8888;
    printf("pid: %d http server start listen: %s:%d\n", CThread::Tid(), listen_addr.ip, listen_addr.port);

    CEventLoop loop;
    CHttpServer server(&loop, listen_addr);
    server.SetHttpCallback(std::bind(TestHttpCallback, _1, _2));
    server.Start();
    loop.Loop();
}
