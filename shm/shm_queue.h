#ifndef __SHM_QUEUE_H
#define __SHM_QUEUE_H

#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include "shm_alloc.h"


// operate shm
class CShmQueue
{
public:
    static CShmQueue* Instance(void);

    int32_t CreatShm(uint32_t size);
    int32_t AttachShm(void);

    int32_t Push(const void* buf, uint32_t len);
    int32_t Pop(void* buf, uint32_t* len);

private:
    static CShmQueue* m_pInstance;
    CShmQueue();
    ~CShmQueue();

private:
    bool IsFull(void);
    bool IsEmpty(void);

private:
    enum E_NodeType
    {
        kDataNode = 1,
        kEndFlagNode = 2
    };

    // shm head
    typedef struct _tShmHead
    {
        TMutex      mlock;
        uint32_t    head; // start from queue
        uint32_t    tail; // end with queue + queueSize
        uint32_t    queueSize;
        uint8_t     queue[0];
    } TShmHead;

    // shm node, not fix length
    typedef struct _tShmNode
    {
        uint8_t  type;   // data node or end node
        uint32_t len;    // data len
        uint8_t  data[0];// data addr
    } TShmNode;

private:
    // max queue msg size: 256 bytes
    static const uint32_t SHM_QUEUE_MSG_MAX_SIZE = 256; 
    static const uint32_t SHM_QUEUE_BUF_SIZE = 1 * 1024 * 1024; // 1Mb

    void*       m_ptr;
    bool        m_isAttach;
};

#endif // __SHM_QUEUE_H
