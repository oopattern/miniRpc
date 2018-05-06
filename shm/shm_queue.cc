#include <stdio.h>
#include "shm_queue.h"


int32_t CShmQueue::Push(const void* buf, uint32_t len)
{
    if (NULL == m_ptr)
    {
        printf("shm queue push invalid error\n");
        return SHM_ERROR;
    }

    if (true == IsFull())
    {
        printf("shm queue is full, push failed\n");
        return SHM_ERROR;
    }

    if (len > SHM_QUEUE_MSG_MAX_SIZE)
    {
        printf("shm queue msg size too large, push failed\n");
        return SHM_ERROR;
    }

    TShmHead* pHead = (TShmHead*)m_ptr;
    int32_t pushPos = -1;
    uint32_t readIdx = pHead->readIdx;
    uint32_t writeIdx = pHead->writeIdx;
    uint32_t nodeLen = len + sizeof(TShmNode);
    uint32_t appendIdx = writeIdx + nodeLen;

    if (readIdx < writeIdx)
    {
        if (appendIdx < m_queueSize)
        {
            pushPos = writeIdx;
        }
        else if (appendIdx < writeIdx + readIdx)
        {
            ::memmove(p->readIdx, p->writeIdx, writeIdx - readIdx);
            p->readIdx = 0;
            p->writeIdx = writeIdx - readIdx;
            pushPos = p->writeIdx;
        }
    }
    else
    {
        if (appendIdx < readIdx)
        {
            pushPos = writeIdx;
        }
    }

    if (pushPos < 0)
    {
        printf("shm queue push not enough space\n");
        return SHM_ERROR;
    }

    // push data in queue
    TShmNode* node = (TShmNode*)(p->queue + pushPos);
    ::memcpy(node->data, buf, len);

    // update index
    node->len = len;
    p->writeIdx += nodeLen;

    return SHM_OK;
}

int32_t CShmQueue::Pop(void* buf, uint32_t* len)
{
    if (NULL == m_ptr)
    {
        printf("shm queue pop invalid error\n");
        return SHM_ERROR;
    }

    if (true == IsEmpty())
    {
        printf("shm queue is empty, pop failed\n");
        return SHM_ERROR;
    }



    return SHM_OK;
}

bool CShmQueue::IsFull(void)
{
    return false;
}

bool CShmQueue::IsEmpty(void)
{
    return false;
}

