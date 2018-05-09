#include <stdio.h>
#include <string>
#include "../public.h"
#include "../thread.h"
#include "../shm/shm_queue.h"

using namespace std;

const uint32_t QUEUE_THREAD_NUM = 3;
const uint32_t LOOP_TIME = 1*MILLION;
const char* s_task_end = "QUEUE_TASK_END";
const char* s_rand_content[] = 
{
    "Business is business.",
    "Tomorrow comes never.",
    "No root, no fruit.",
    "Time flies.",
    "No one can call back yesterday, Yesterday will not be called again.",
    "Have you somewhat to do tomorrow, do it today.",
    "Time tames the strongest grief.",
    "When an opportunity is neglected, it never comes back to you.",
    "If you want knowledge, you must toil for it.",
    "Doubt is the key of knowledge.",
    "Practice makes perfect.",
    "Wit once bought is worth twice taught."
};
const uint32_t ELEMENT = sizeof(s_rand_content) / sizeof(s_rand_content[0]);


class CTestQueue
{
public:
    static void TestQueueCapacity(void);
    static void TestQueuePush(void);
    static void TestQueuePop(void);
    static void TestQueueTPS(void);
    static void PopTask(void);
};

void CTestQueue::PopTask(void)
{
    g_shmQueue.InitQueue(POSIX, false);

    while (1)
    {
        char buf[256];
        uint32_t len = sizeof(buf);
        
        if (SHM_OK != g_shmQueue.Pop(buf, &len))
        {
            //printf("Pop task waitting for pop\n");
            //::sleep(1);
            continue;
        }
        assert(len < sizeof(buf));
        buf[len] = '\0';

        if (0 == ::strcmp(buf, s_task_end))
        {
            printf("tid=%d finish, quit safely\n", CThread::Tid());
            return;
        }
    }
}

// one-write-mutli-read
void CTestQueue::TestQueueTPS(void)
{
    printf("Test Queue TPS start time: %s\n", CUtils::GetCurrentTime());

    g_shmQueue.InitQueue(POSIX, false);
    g_shmQueue.ShowQueue();

    // worker pthread just pop message from queue
    CThreadPool pool(QUEUE_THREAD_NUM, std::bind(PopTask));
    pool.StartAll();

    // main pthread just push message into queue
    uint32_t cnt = 0;
    while (cnt < LOOP_TIME)
    {
        uint32_t idx = ::rand() % ELEMENT;
        uint32_t len = ::strlen(s_rand_content[idx]);
        if (SHM_OK != g_shmQueue.Push(s_rand_content[idx], len))
        {
            ::usleep(5000);
            continue;
        }
        cnt++;
    }

    printf("main pthread=%d push finish time: %s\n", CThread::Tid(), CUtils::GetCurrentTime());

    // main pthread add end flag    
    cnt = 0;
    while (cnt < QUEUE_THREAD_NUM)
    {
        if (SHM_OK != g_shmQueue.Push(s_task_end, ::strlen(s_task_end)))
        {
            ::usleep(5000);
            printf("check out...........\n");
            continue;
        }
        cnt++;
    }

    pool.JoinAll();

    g_shmQueue.ShowQueue();

    printf("Test Queue TPS   end time: %s\n", CUtils::GetCurrentTime());
}

void CTestQueue::TestQueuePop(void)
{
    g_shmQueue.InitQueue(POSIX, false);

    while (1)
    {
        char pop[256];
        uint32_t len = sizeof(pop);

        if (SHM_OK != g_shmQueue.Pop(pop, &len))
        {
            break;
        }        
        assert(len < sizeof(pop));
        pop[len] = '\0';
        printf("shm queue pop data: %s\n", pop);
    }

    g_shmQueue.ShowQueue();
}

// mutli-write-one-read
void CTestQueue::TestQueuePush(void)
{
    g_shmQueue.InitQueue(POSIX, false);

    // main pthread push content
    uint32_t cnt = 0;
    while (cnt < LOOP_TIME)
    {
        uint32_t idx = ::rand() % ELEMENT;
        uint32_t len = ::strlen(s_rand_content[idx]);
        if (SHM_OK != g_shmQueue.Push(s_rand_content[idx], len))
        {
            break;
        }
        printf("shm queue push data: %s\n", s_rand_content[idx]);
        cnt++;
    }

    g_shmQueue.ShowQueue();
}

void CTestQueue::TestQueueCapacity(void)
{
    std::string push = "hello oopattern";
    char pop[256];

    if (SHM_OK != g_shmQueue.InitQueue(POSIX, true))    
    {
        printf("shm queue create error\n");
        return;
    }

    printf("shm queue push: %s\n", push.c_str());
    if (SHM_OK != g_shmQueue.Push(push.c_str(), push.size()))
    {
        printf("shm queue push error\n");
        return;
    }

    uint32_t maxsize = sizeof(pop);
    if (SHM_OK != g_shmQueue.Pop(pop, &maxsize))
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

    g_shmQueue.ShowQueue();
}

