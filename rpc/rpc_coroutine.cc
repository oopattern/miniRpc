#include <stdio.h>
#include <limits.h> // INT_MAX
#include <assert.h> // assert
#include "../third_party/libco/co_routine.h"
#include "../third_party/libco/co_routine_inner.h"
#include "rpc_coroutine.h"


int32_t CRpcCoroutine::m_inc_coid = 0;

CRpcCoroutine::CRpcCoroutine(void* (*routine)(void*), void* arg)
{
    stCoRoutine_t* co = NULL;
    co_create(&co, NULL, routine, arg);    

    if (NULL == co)
    {
        printf("coroutine create failed error\n");
        ::exit(-1);
    }

    m_co_id = CRpcCoroutine::GenerateCoroutineId();
    m_coroutine = co;   
    m_status = kCoStop;

    // store coroutine object, use for get coroutine id
    co->aSpec[0].value = this;
}

CRpcCoroutine::~CRpcCoroutine()
{

}

void CRpcCoroutine::SetRpcCall(TRpcCall& rpc_call)
{
    m_rpc_call = rpc_call;
}

void CRpcCoroutine::GetRpcCall(TRpcCall& rpc_call)
{
    rpc_call = m_rpc_call;
}

int32_t CRpcCoroutine::GetId(void) const
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

void CRpcCoroutine::Yield(void)
{
    if ((NULL == m_coroutine) || (m_co_id < 0))
    {
        printf("rpc yield invalid coroutine id=%d error\n", m_co_id);
        return;
    }

    m_status = kCoSuspend;
    //printf("Yield co_id=%d coroutine status=%d\n", m_co_id, m_status);
    co_yield_ct();
}

void CRpcCoroutine::Resume(void)
{
    if ((NULL == m_coroutine) || (m_co_id <= 0))
    {
        printf("rpc resume invalid coroutine id=%d error\n", m_co_id);
        return;
    }

    //printf("Resume co_id=%d coroutine status=%d\n", m_co_id, m_status);
    m_status = kCoRunning;
    co_resume(m_coroutine);
}

void CRpcCoroutine::Release(void)
{
    if ((NULL == m_coroutine) || (m_co_id <= 0))
    {
        printf("rpc release invalid coroutine id=%d error\n", m_co_id);
        return;
    }

    //printf("delete stCoRoutine_t = %p\n", m_coroutine);

    co_release(m_coroutine);
    m_coroutine = NULL;
    m_co_id = -1;
    m_status = kCoStop;
    m_rpc_call.recv_buf = NULL;
    m_rpc_call.recv_len = 0;
}

CRpcCoroutine* CRpcCoroutine::GetOwner(void)
{
    stCoRoutine_t* co = co_self();
    assert(co != NULL);
    CRpcCoroutine* rpc_co = (CRpcCoroutine*)(co->aSpec[0].value);
    
    return rpc_co;
}

