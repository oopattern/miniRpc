#include <stdio.h>
#include "shm_queue.h"

enum E_NodeType
{
    kDataNode = 1,
    kEndFlagNode = 2
};

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

    TShmHead* pShm = (TShmHead*)m_ptr;

    bool success = true;
    uint32_t pushPos = 0;
    uint32_t head = pShm->head;
    uint32_t tail = pShm->tail;
    uint32_t queueSize = pShm->queueSize;
    uint32_t nodeLen = len + sizeof(TShmNode); // include node head

    // head == tail means empty
    // head < tail means write not reach queue end yet
    if (head <= tail)
    {
        // much space to append node data
        if ((tail + nodeLen) < queueSize)
        {
            pushPos = tail;
        }
        // almost reach queue end, set end flag
        // check if queue head enough to hold node date
        else if (nodeLen < head)
        {
            pushPos = 0;
            if (tail + sizeof(TShmNode) <= queueSize)
            {
                TShmNode* pNode = (TShmNode*)(pShm->queue + tail);
                pNode->type = kEndFlagNode;
                pNode->len = queueSize - tail - sizeof(TShmNode);                
            }
            else
            {
                printf("shm queue push not enough space to add end-flag node\n");
            }
        }
        // no enough space to hold node data, sorry about that
        else
        {
            success = false;
            printf("head <= tail shm queue push not enough space error\n");
        }
    }
    // head > tail means write already reach queue end, and wrap round
    else
    {
        // even wrap round, we have space to hold node data
        if (tail + nodeLen < head)
        {
            pushPos = tail;
        }
        // sorry about not enough space, just give up
        else
        {
            success = false;
            printf("head > tail shm queue push not enough space error\n");
        }        
    }

    if (false == success)
    {
        return SHM_ERROR;
    }

    // operation for node
    TShmNode* pNode = (TShmNode*)(pShm->queue + pushPos);    
    ::memcpy(pNode->data, buf, len);
    pNode->len = len;
    pNode->type = kDataNode;

    // pushPos is the right position, not tail
    // add nodeLen, should include node head
    pShm->tail = pushPos + nodeLen;

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

    TShmHead* pShm = (TShmHead*)m_ptr;

    uint32_t head = pShm->head;
    uint32_t tail = pShm->tail;
    uint32_t queueSize = pShm->queueSize;

    assert((head <= queueSize) && (tail <= queueSize));

    // not enough space to get node head
    if ((head + sizeof(TShmNode)) > queueSize)
    {
        printf("shm queue head reach end of queue, please wrap round\n");
        head = 0;
    }

    // get node head
    TShmNode* pNode = (TShmNode*)(pShm->queue + head);

    // end flag node, reset start position
    if (kEndFlagNode == pNode->type)
    {
        printf("shm queue reach end flag node, please wrap round\n");
        head = 0;
        pNode = (TShmNode*)(pShm->queue + head);
    }

    // node type not match
    if (pNode->type != kDataNode)
    {
        printf("[fatal] shm queue node type error, can not find node\n");
        return SHM_ERROR;
    }

    uint32_t bufsize = *len;
    uint32_t datalen = pNode->len;
    if (datalen > bufsize)
    {
        printf("shm queue pop node size=%d large than buf size=%d error\n", datalen, bufsize);
        return SHM_ERROR;
    }

    // pop node data
    ::memcpy(buf, pNode->data, datalen);
    *len = datalen;

    // update shm queue head position
    pShm->head = head + sizeof(TShmNode) + datalen;

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

