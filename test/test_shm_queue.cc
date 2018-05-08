#include <stdio.h>
#include <string>
#include "../shm/shm_queue.h"

using namespace std;

class CTestQueue
{
public:
    static void TestQueueCapacity(void);
};

void CTestQueue::TestQueueCapacity(void)
{
    std::string push = "hello oopattern";
    char pop[256];

    if (SHM_OK != g_pShmQueue->CreateShm())    
    {
        printf("shm queue create error\n");
        return;
    }

    printf("shm queue push: %s\n", push.c_str());
    if (SHM_OK != g_pShmQueue->Push(push.c_str(), push.size()))
    {
        printf("shm queue push error\n");
        return;
    }

    uint32_t maxsize = sizeof(pop);
    if (SHM_OK != g_pShmQueue->Pop(pop, &maxsize))
    {
        printf("shm queue pop error\n");
        return;
    }
    assert(maxsize < sizeof(pop));
    pop[maxsize] = '\0';        
    printf("shm queue  pop: %s\n", pop);

    if (0 != ::strcmp(push.c_str(), pop))
    {
        printf("shm push and pop not match\n");
        return;
    }

    g_pShmQueue->ShowQueue();
}

