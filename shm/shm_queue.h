#ifndef __SHM_QUEUE_H
#define __SHM_QUEUE_H

#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include "shm_alloc.h"

// define global
#define g_pShmQueue     CShmQueue::Instance()


// operate shm
class CShmQueue
{
public:
    static CShmQueue* Instance(void);

    // create operation
    int32_t CreateShm(uint32_t size = SHM_QUEUE_TOTAL_SIZE);
    int32_t AttachShm(void);

    // push and pop operation
    int32_t Push(const void* buf, uint32_t len);
    int32_t Pop(void* buf, uint32_t* len);

    void ShowQueue(void);

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
        // should reserve some space for future use
        TMutex      mlock;// use for mutex lock 
        uint32_t    head; // start from queue
        uint32_t    tail; // end with queue + queueSize
        uint32_t    queueCount; // msg count
        uint32_t    queueSize; // size of queue
        uint8_t     reserve[64]; // reserve for shm head
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
    static const uint32_t SHM_QUEUE_TOTAL_SIZE = SHM_QUEUE_BUF_SIZE + sizeof(TShmHead);
        
    void*       m_ptr;
    bool        m_isAttach;
};

#endif // __SHM_QUEUE_H
