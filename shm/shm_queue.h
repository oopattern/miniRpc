#ifndef __SHM_QUEUE_H
#define __SHM_QUEUE_H

#include <stdio.h>
#include <stdint.h>
#include <pthread.h>

// max queue msg size: 256 bytes
const uint32_t SHM_QUEUE_MSG_MAX_SIZE = 256; 
const uint32_t SHM_QUEUE_BUF_SIZE = 1 * 1024 * 1024; // 1Mb

// shm head
typedef struct _tShmHead
{
    pthread_mutex_t     mutex;
    pthread_mutexattr_t attr;
    uint32_t            head; // start from queue
    uint32_t            tail; // end with queue + queueSize
    uint32_t            queueSize;
    uint8_t             queue[0];
} TShmHead;

// shm node, not fix length
typedef struct _tShmNode
{
    uint8_t  type;   // data node or end node
    uint32_t len;    // data len
    uint8_t  data[0];// data addr
} TShmNode;

// operate shm
class CShmQueue
{
public:
    int32_t Push(const void* buf, uint32_t len);
    int32_t Pop(void* buf, uint32_t* len);

private:
    bool IsFull(void);
    bool IsEmpty(void);

private:
    void*       m_ptr;
};

#endif // __SHM_QUEUE_H
