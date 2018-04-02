#include <stdio.h>

// magnitude
const unsigned long MILLION = 1000000UL;
const unsigned long THOUSAND = 1000;

char* ShowMagnitude(unsigned long val);
int Uint2String(char* dst, size_t dstlen, unsigned int value);
long long TimeInMilliseconds(void);
void GetCurrentTime(char* timeStr, int timeLen);
