#include <stdio.h>
#include "../rpc/rpc_coroutine.h"


class CTestCoroutine
{
public:
    static void TestNoCoroutine(void);
    static void TestCoroutine(void);
    static void* Routine(void* arg);
};

void* CTestCoroutine::Routine(void* arg)
{
    CRpcCoroutine* co = (CRpcCoroutine*)arg;
    printf("suspend routine\n");
    // after yield, routine will suspend, until receive resume to continue
    co->Yield();
    printf("continue routine\n");
    // when routine finish(routine return), co is no longer useful, can not resume or yield again
}

void CTestCoroutine::TestNoCoroutine(void)
{
    // if no routine register, there is no point, since nothing happen
    CRpcCoroutine co(NULL, NULL);
    co.Resume();
    while (1)
    {
        ::sleep(1);
        printf("coroutine sleep for 1 second\n");
    }
}

void CTestCoroutine::TestCoroutine(void)
{
    CRpcCoroutine co(Routine, &co);
    // just create routine, routine not running
    // first resume: immediately run routine register by co_create
    co.Resume();
    // second resume: routine yield, continue routine to go on running
    co.Resume();
    // when routine finish(routine return), co is no longer useful, can not resume or yield again
    // when routine finish, if try to yield or resume again, process will coredump
    while (1)
    {
        printf("test coroutine, sleep for 3 second\n");
        ::sleep(3);                
        //co.Yield();
    }
}

