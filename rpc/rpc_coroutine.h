#ifndef __RPC_COROUTINE_H
#define __RPC_COROUTINE_H

#include <stdio.h>

typedef enum
{
    kCoStop = 0,    // coroutine finish func, or it has stopped with an error(such as timeout)
    kCoRunning = 1, // coroutine is running
    kCoSuspend = 2, // coroutine is suspended with call yield    
} ECoStatus;

typedef struct TRpcCall
{
    const char*     recv_buf;
    int32_t         recv_len;
    
    // default construct, rpc_co must init NULL
    TRpcCall() : recv_buf(NULL), recv_len(0) {}
} TRpcCall;

struct stCoRoutine_t;

class CRpcCoroutine
{
public:
    CRpcCoroutine(void* (*routine)(void*), void* arg);
    ~CRpcCoroutine();

    void Yield(void);
    void Resume(void);        
    void Release(void);
    int32_t GetId(void) const;

    // for rpc call back
    void SetRpcCall(TRpcCall& rpc_call);
    void GetRpcCall(TRpcCall& rpc_call);

public:
    // for all coroutine object to use
    static CRpcCoroutine* GetOwner(void);
    static int32_t GenerateCoroutineId(void);

private:    
    int32_t         m_co_id;    // coroutine id
    stCoRoutine_t*  m_coroutine;// coroutine
    ECoStatus       m_status;   // coroutine status
    TRpcCall        m_rpc_call; // for rpc call back

    // TODO: mutli pthread, since Concurrency, m_inc_coid should use atomic 
    static int32_t  m_inc_coid; // generate coroutine id by increase
};

#endif
