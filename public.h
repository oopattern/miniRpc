#ifndef __PUBLIC_H
#define __PUBLIC_H

#include <stdio.h>


// magnitude
const unsigned long MILLION = 1000000UL;
const unsigned long THOUSAND = 1000;

// public common interface
class CUtils 
{
public:
    static char* ShowMagnitude(unsigned long val);
    static int Uint2String(char* dst, size_t dstlen, unsigned int value);
    static long long TimeInMilliseconds(void);
    static void GetCurrentTime(char* timeStr, int timeLen);
};


#endif // end of __PUBLIC_H
