#include <stdio.h>
#include <assert.h> // assert
#include <string.h> // memcpy
#include "shm_queue.h"
#include "../thread.h"


static const char* MMAP_QUEUE = "POSIX_MMAP_QUEUE";

CShmQueue::CShmQueue()
{
    m_ptr = NULL;
    m_isAttach = false;
    m_alloc = POSIX;
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
    printf("Queue  msg count: %u\n", p->queueCount);
    printf("Queue head   pos: %d\n", p->head);
    printf("Queue tail   pos: %d\n", p->tail);
    printf("- - - - - - - - - - - - - - - QUEUE_STATUS_END - - - - - - - - - - - - - - - - \n");
}

// create shm or attach shm
int32_t CShmQueue::InitQueue(uint8_t alloc, bool bCreat, uint32_t size)
{
    m_alloc = alloc;
    
    if (false == bCreat)
    {
        return AttachShm();
    }

    return CreateShm(size);
}

int32_t CShmQueue::CreateShm(uint32_t size)
{
    if (size > SHM_QUEUE_TOTAL_SIZE)
    {
        printf("shm size=%d large than queue max size=%d\n", size, SHM_QUEUE_TOTAL_SIZE);
        return SHM_ERROR;        
    }

    // choose alloc type
    if (POSIX == m_alloc)
    {
        m_ptr = CShmAlloc::PosixCreate(MMAP_QUEUE, size);
        if (NULL == m_ptr)
        {
            return SHM_ERROR;
        }
    }
    else 
    {
        printf("create shm queue alloc type error\n");
        return SHM_ERROR;
    }

    // init shm head once
    TShmHead* p = (TShmHead*)m_ptr;
    p->head = 0;
    p->tail = 0;
    p->queueCount = 0;
    p->queueSize = size - sizeof(TShmHead);

    // mutex lock init once
    if (SHM_OK != CShmAlloc::InitLock(&p->mlock))
    {
        m_isAttach = false;
        return SHM_ERROR;
    }

    // mark attach status
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

    // choose alloc type
    if (POSIX == m_alloc)
    {
        m_ptr = CShmAlloc::PosixAttach(MMAP_QUEUE);
        if (NULL == m_ptr)
        {
            return SHM_ERROR;
        }
    }
    else
    {
        printf("attach shm queue alloc type error\n");
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

    // use end-flag to charge if reach queue end
    // full is not queue size - 1

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
    assert((head <= queueSize) && (tail <= queueSize));

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
                pNode->tag = kStartTag;
                pNode->type = kEndFlagNode;
                pNode->len = queueSize - tail - sizeof(TShmNode);                
            }
            else
            {
                //printf("shm queue push not enough space to add end-flag node\n");
            }
        }
        // no enough space to hold node data, sorry about that
        else
        {
            success = false;
            //printf("head <= tail shm queue push not enough space error\n");
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
            //printf("head > tail shm queue push not enough space error\n");
        }        
    }

    if (true == success)
    {
        // operation for node
        TShmNode* pNode = (TShmNode*)(pShm->queue + pushPos);    
        ::memcpy(pNode->data, buf, len);
        pNode->len = len;
        pNode->type = kDataNode;
        pNode->tag = kStartTag;

        // pushPos is the right position, not tail
        // add nodeLen, should include node head
        pShm->tail = pushPos + nodeLen;
        pShm->queueCount++;
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

    TShmHead* pShm = (TShmHead*)m_ptr;

    CShmAlloc::LockShm(&pShm->mlock);

    bool success  = true;
    uint32_t head = pShm->head;
    uint32_t tail = pShm->tail;
    uint32_t queueSize = pShm->queueSize;

    assert((head <= queueSize) && (tail <= queueSize));

    // when access shm head info, should protect by mutex lock
    if (head == tail)
    {
        CShmAlloc::UnlockShm(&pShm->mlock);
        return SHM_ERROR;
    }

    // not enough space to get node head
    if ((head + sizeof(TShmNode)) > queueSize)
    {
        //printf("shm queue head reach end of queue, please wrap round\n");
        head = 0;
    }

    // get node head
    TShmNode* pNode = (TShmNode*)(pShm->queue + head);

    // end flag node, reset start position
    if ((kStartTag == pNode->tag) && (kEndFlagNode == pNode->type))
    {
        //printf("shm queue node reach end flag node, please wrap round\n");
        head = 0;
        pNode = (TShmNode*)(pShm->queue + head);
    }

    // node start tag or type not match
    if ((pNode->tag != kStartTag) || (pNode->type != kDataNode))
    {
        success = false;
        printf("[fatal] shm queue node tag=%d type=%d error\n", pNode->tag, pNode->type);
        ::exit(-1);
    }

    // node size too large
    uint32_t bufsize = *len;
    uint32_t datalen = pNode->len;
    if (datalen > bufsize)
    {
        success = false;
        printf("shm queue pop node size=%d large than buf size=%d error\n", datalen, bufsize);
    }

    if (true == success)
    {
        // pop node data
        ::memcpy(buf, pNode->data, datalen);
        *len = datalen;    
        
        // update shm queue head position
        pShm->head = head + sizeof(TShmNode) + datalen;    
        if (pShm->queueCount > 0)
        {
            pShm->queueCount--;        
        }
    }

    CShmAlloc::UnlockShm(&pShm->mlock);

    return ((true == success) ? (SHM_OK) : (SHM_ERROR));
}

