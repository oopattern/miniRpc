#ifndef __RPC_COROUTINE_H
#define __RPC_COROUTINE_H

#include <stdio.h>

typedef enum
{
    kCoStop = 0,    // coroutine finish func, or it has stopped with an error(such as timeout)
    kCoRunning = 1, // coroutine is running
    kCoSuspend = 2, // coroutine is suspended with call yield    
} ECoStatus;


struct stCoRoutine_t;

class CRpcCoroutine
{
public:
    CRpcCoroutine();
    ~CRpcCoroutine();

    int32_t Create(void* (*routine)(void*), void* arg);

    void Yield(void);

    void Resume(void);        

    int32_t GetId(void) const;

public:
    static int32_t GenerateCoroutineId(void);

private:    
    int32_t         m_co_id;    // coroutine id
    stCoRoutine_t*  m_coroutine;// coroutine
    ECoStatus       m_status;   // coroutine status

    // TODO: mutli pthread, since Concurrency, m_inc_coid should use atomic 
    static int32_t  m_inc_coid; // generate coroutine id by increase
};

#endif
