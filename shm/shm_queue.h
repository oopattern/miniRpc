#ifndef __SHM_QUEUE_H
#define __SHM_QUEUE_H

#include <stdio.h>
#include <stdint.h>
#include "../base/singleton.h"

// define global
#define g_shmQueue      Singleton<CShmQueue>::Instance()


// operate shm
class CShmQueue
{
public:
    CShmQueue();
    ~CShmQueue();

    // create or attach operation
    int32_t InitShm( uint8_t allocType = POSIX, 
                     uint8_t lockType = LOCK_MUTEX, 
                     bool bCreat = false, 
                     uint32_t size = SHM_QUEUE_TOTAL_SIZE);
    
    // push and pop operation
    int32_t Push(const void* buf, uint32_t len);
    int32_t Pop(void* buf, uint32_t* len);

    void ShowQueue(void);

private:
    // shm operation
    int32_t CreateShm(uint32_t size = SHM_QUEUE_TOTAL_SIZE);
    int32_t AttachShm(void);

    // lock operation
    void Lock(void);
    void Unlock(void);

private:
    // shm queue must add end node, otherwise when head reach queue nearly,
    // if not add end node, it will read the wrong info of node head
    // if not enough space to hold end node, just skip the rest space and wrap round again
    enum E_NodeType
    {
        kDataNode = 1,
        kEndFlagNode = 2
    };

    // shm head
    typedef struct _tShmHead
    {
        // should reserve some space for future use
        TMutex      mlock;      // use for mutex lock 
        uint8_t     access;     // atomic, lock free
        uint32_t    head;       // start from queue
        uint32_t    tail;       // end with queue + queueSize
        uint32_t    queueCount; // msg count
        uint8_t     reserve[64];// reserve for shm head
        uint32_t    queueSize;  // size of queue
        uint8_t     queue[0];
    } TShmHead;

    // shm node, not fix length
    typedef struct _tShmNode
    {
        uint16_t tag;        // node start tag, means it's a node
        uint8_t  type;       // data node or end node
        uint8_t  reserve[16];// data node reserve, use for crc later maybe, to check data completely
        uint32_t len;        // data len
        uint8_t  data[0];    // data addr
    } TShmNode;

    // node start tag
    static const uint16_t kStartTag = 0x9BEB;

private:
    // max queue msg size: 256 bytes
    static const int32_t  SHM_QUEUE_KEY = 0x20180622;
    static const uint32_t SHM_QUEUE_MSG_MAX_SIZE = 256; 
    static const uint32_t SHM_QUEUE_BUF_SIZE = 1 * 1024 * 1024; // 1Mb
    static const uint32_t SHM_QUEUE_TOTAL_SIZE = SHM_QUEUE_BUF_SIZE + sizeof(TShmHead);
        
    void*       m_ptr;      // shm start addr
    bool        m_isAttach; // shm attach status
    uint8_t     m_allocType;// alloc type, SVIPC or POSIX
    uint8_t     m_lockType; // lock type, mutex or atomic
};

#endif // __SHM_QUEUE_H
