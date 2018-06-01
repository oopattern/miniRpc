#include <stdio.h>
#include <limits.h>
#include "../third_party/libco/co_routine.h"
#include "../third_party/libco/co_routine_inner.h"
#include "rpc_coroutine.h"


int32_t CRpcCoroutine::m_inc_coid = 0;

CRpcCoroutine::CRpcCoroutine()
    : m_co_id(-1),
      m_coroutine(NULL),
      m_status(kCoStop)
{

}

CRpcCoroutine::~CRpcCoroutine()
{

}

int32_t CRpcCoroutine::GetCoroutineId(void) const
{
    return m_co_id;
}

int32_t CRpcCoroutine::GenerateCoroutineId(void)
{
    ++m_inc_coid;
    if (m_inc_coid >= INT_MAX)
    {
        m_inc_coid = 1;
    }

    return m_inc_coid;
}

int32_t CRpcCoroutine::Create(void* (*routine)(void*), void* arg)
{
    stCoRoutine_t* co = NULL;
    co_create(&co, NULL, routine, arg);    

    if (NULL == co)
    {
        printf("coroutine create failed error\n");
        return -1;
    }
    
    m_co_id = CRpcCoroutine::GenerateCoroutineId();
    m_coroutine = co;   
    m_status = kCoStop;
    
    return m_co_id;
}

void CRpcCoroutine::Yield(void)
{
    if ((NULL == m_coroutine) || (m_co_id < 0))
    {
        printf("rpc yield invalid coroutine id=%d error\n", m_co_id);
        return;
    }

    m_status = kCoSuspend;
    co_yield_ct();
}

void CRpcCoroutine::Resume(void)
{
    if ((NULL == m_coroutine) || (m_co_id <= 0))
    {
        printf("rpc resume invalid coroutine id=%d error\n", m_co_id);
        return;
    }

    if (m_status != kCoSuspend)
    {
        printf("rpc coroutine id=%d status=%d error\n", m_co_id, m_status);
        return;
    }

    m_status = kCoRunning;
    co_resume(m_coroutine);
}


