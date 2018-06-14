#include <stdio.h>
#include "../net/timer_queue.h"


static int32_t s_timer_loop_times = 0;    
static int32_t s_cancel_timer_seq = 0;

class CTestTimer
{
public:
    static void TestTimer(void);
    static void OnceFunc(void);
    static void RepeatFunc(CEventLoop* loop);
};

void CTestTimer::OnceFunc(void)
{
    printf("timer once func now time = %s\n", CUtils::GetCurrentTime());
}

void CTestTimer::RepeatFunc(CEventLoop* loop)
{
    if (++s_timer_loop_times >= 10)
    {
        loop->CancelTimer(s_cancel_timer_seq);
    }
    printf("timer repeat loop = %d, func now time = %s\n", s_timer_loop_times, CUtils::GetCurrentTime());    
}

void CTestTimer::TestTimer(void)
{
    printf("welcome to test timer\n");
    ::sleep(1);

    CEventLoop loop;
    printf("test start time: %s\n", CUtils::GetCurrentTime());
    //loop.RunAfter(2500, std::bind(OnceFunc));
    s_cancel_timer_seq = loop.RunEvery(500, std::bind(RepeatFunc, &loop));
    loop.Loop();
}
