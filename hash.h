#ifndef __HASH_H
#define __HASH_H

#include <stdio.h>

// hash table
typedef struct THashTbl {
    unsigned long size; // bucket number
    unsigned long mask; // bucket size - 1
    unsigned long used; // already used
    unsigned long collision; // collision number
} THashTbl;

// macro
// why bucket 5000 VS bucket 6000, collision will more than prev condition ?
const unsigned long HASH_BUCKET_SIZE = 6000;    // default hash bucket size
const unsigned long HASH_COLLISION_SIZE = 100;  // default collision size, 1/10 with bucket size

// slot = hashkey & mask
unsigned int HashKeyFunction(const void *key, int len);
int ll2string(char* dst, size_t dstlen, long long svalue);
long long TimeInMilliseconds(void);
void GetCurrentTime(char* timeStr, int timeLen);

#endif // __HASH_H
