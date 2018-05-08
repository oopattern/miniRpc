#include <stdio.h>
#include <assert.h> // assert
#include <string.h> // memcpy
#include "shm_queue.h"
#include "../thread.h"


static const char* MMAP_QUEUE = "POSIX_MMAP_QUEUE";

CShmQueue* CShmQueue::m_pInstance = NULL;

CShmQueue* CShmQueue::Instance(void)
{
    if (NULL == m_pInstance)
    {
        m_pInstance = new CShmQueue();
    }
    return m_pInstance;
}

CShmQueue::CShmQueue()
{
    m_ptr = NULL;
    m_isAttach = false;
}

CShmQueue::~CShmQueue()
{

}

void CShmQueue::ShowQueue(void)
{
    TShmHead* p = (TShmHead*)m_ptr;
    printf("- - - - - - - - - - - - - - - QUEUE_STATUS_START - - - - - - - - - - - - - - - \n");
    printf("Queue total size: %lu\n", p->queueSize + sizeof(TShmHead));
    printf("Queue queue size: %u\n", p->queueSize);
    printf("Queue head   pos: %d\n", p->head);
    printf("Queue tail   pos: %d\n", p->tail);
    printf("- - - - - - - - - - - - - - - QUEUE_STATUS_END - - - - - - - - - - - - - - - - \n");
}

int32_t CShmQueue::CreateShm(uint32_t size)
{
    if (size > SHM_QUEUE_TOTAL_SIZE)
    {
        printf("shm size=%d large than queue max size=%d\n", size, SHM_QUEUE_TOTAL_SIZE);
        return SHM_ERROR;
    }

    m_ptr = CShmAlloc::PosixCreate(MMAP_QUEUE, size);
    if (NULL == m_ptr)
    {
        return SHM_ERROR;
    }    

    // init shm head once
    TShmHead* p = (TShmHead*)m_ptr;
    p->head = 0;
    p->tail = 0;
    p->queueSize = size - sizeof(TShmHead);

    // mutex lock init once
    if (SHM_OK != CShmAlloc::InitLock(&p->mlock))
    {
        m_isAttach = false;
        return SHM_ERROR;
    }

    m_isAttach = true;
    return SHM_OK;
}

int32_t CShmQueue::AttachShm(void)
{
    if (true == m_isAttach)
    {
        printf("tid=%d, process already attach before\n", CThread::Tid());
        return SHM_OK;
    }

    m_ptr = CShmAlloc::PosixAttach(MMAP_QUEUE);
    if (NULL == m_ptr)
    {
        return SHM_ERROR;
    }

    // mutex lock init once
    TShmHead* p = (TShmHead*)m_ptr;
    if (SHM_OK != CShmAlloc::InitLock(&p->mlock))
    {
        m_isAttach = false;
        return SHM_ERROR;
    }

    m_isAttach = true;
    return SHM_OK;
}

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

    CShmAlloc::LockShm(&pShm->mlock);

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

    if (true == success)
    {
        // operation for node
        TShmNode* pNode = (TShmNode*)(pShm->queue + pushPos);    
        ::memcpy(pNode->data, buf, len);
        pNode->len = len;
        pNode->type = kDataNode;

        // pushPos is the right position, not tail
        // add nodeLen, should include node head
        pShm->tail = pushPos + nodeLen;
    }

    CShmAlloc::UnlockShm(&pShm->mlock);

    return ((true == success) ? (SHM_OK) : (SHM_ERROR));
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

    CShmAlloc::LockShm(&pShm->mlock);

    bool success  = true;
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
        success = false;
        printf("[fatal] shm queue node type error, can not find node\n");
    }
    else
    {
        uint32_t bufsize = *len;
        uint32_t datalen = pNode->len;

        if (datalen > bufsize)
        {
            success = false;
            printf("shm queue pop node size=%d large than buf size=%d error\n", datalen, bufsize);
        }
        else
        {
            // pop node data
            ::memcpy(buf, pNode->data, datalen);
            *len = datalen;    
            // update shm queue head position
            pShm->head = head + sizeof(TShmNode) + datalen;
        }
    }

    CShmAlloc::UnlockShm(&pShm->mlock);

    return ((true == success) ? (SHM_OK) : (SHM_ERROR));
}

bool CShmQueue::IsFull(void)
{
    // use end-flag to charge if reach queue end
    return false;
}

bool CShmQueue::IsEmpty(void)
{
    if (NULL == m_ptr)
    {
        printf("shm queue init error\n");
        return false;
    }

    TShmHead* p = (TShmHead*)m_ptr;
    return (p->head == p->tail);
}
