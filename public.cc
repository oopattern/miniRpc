#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <assert.h> // assert

static unsigned long s_digit_tbl[] = {
    1000000000UL,
    100000000UL,
    10000000,
    1000000,
    100000,
    10000,
    1000,
    100,
    10,
    1,
};

const int DIGIT_SIZE = sizeof(s_digit_tbl) / sizeof(s_digit_tbl[0]);

// simple unsigned int to string, more slowly than redis method
int Uint2String(char* dst, size_t dstlen, unsigned int value)
{
    int i = 0;
    unsigned int cnum = 0;
    unsigned int digit = 0;
    bool firstNonZero = false;

    assert(dstlen > DIGIT_SIZE);

    while (i < DIGIT_SIZE)    
    {
        if ((i + 1) == DIGIT_SIZE)
        {
            digit = value % 10;
        }
        else
        {
            digit = value / s_digit_tbl[i];        
        }

        // care first digit not be zero
        if (!firstNonZero)
        {
            if (digit > 0)
            {
                firstNonZero = true;
                value = value - digit * s_digit_tbl[i];
                dst[cnum++] = digit + '0';                
            }
        }
        else
        {
            if (digit >= 0)
            {
                value = value - digit * s_digit_tbl[i];
                dst[cnum++] = digit + '0';                
            }
        }
        i++;
    }

    return cnum;
}

long long TimeInMilliseconds(void) 
{
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return (((long long)tv.tv_sec)*1000)+(tv.tv_usec/1000);
}

void GetCurrentTime(char* timeStr, int timeLen)
{
	struct timeval tv;
	struct tm tm;

	gettimeofday(&tv, NULL);
	localtime_r(&tv.tv_sec, &tm);

	tm.tm_year += 1900;
	tm.tm_mon += 1;

	snprintf(timeStr, timeLen, "%4d-%02d-%02d %02d:%02d:%02d %03d.%03d",
		tm.tm_year, tm.tm_mon, tm.tm_mday, tm.tm_hour, 
        tm.tm_min, tm.tm_sec, (unsigned int)(tv.tv_usec / 1000), (unsigned int)(tv.tv_usec % 1000));
}
