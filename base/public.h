#ifndef __PUBLIC_H
#define __PUBLIC_H

#include <stdio.h>
#include <string>
#include <stdint.h>
#include <vector>

using std::string;
using std::vector;

// magnitude
const unsigned long MILLION = 1000000UL;
const unsigned long THOUSAND = 1000;

// public common interface
class CUtils 
{
public:
    static int Uint2String(char* dst, size_t dstlen, unsigned int value);
    static int SplitStr(const char* src, char* mark, std::vector<std::string>& vecRet);
    static char* ShowMagnitude(unsigned long val);
    static char* GetCurrentTime(void);
    static long long TimeInMilliseconds(void);
    static int64_t NowMsec(void);
};


#endif // end of __PUBLIC_H
