#include <stdio.h>
#include <stdlib.h>
#include <string.h> // strlen
#include <sys/time.h>
#include <time.h>
#include <assert.h> // assert
#include "public.h"

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

int CUtils::SplitStr(const char* src, char* mark, std::vector<std::string>& vecRet)
{
    char* ps_temp;
    char* p;
    std::string st_str;
    ps_temp = new char[strlen(src)+2];
    snprintf(ps_temp, strlen(src)+1 , "%s" , src);
    char *last = NULL;

    p = strtok_r(ps_temp, mark, &last);
    if (NULL == p)
    {
        delete ps_temp;
        return 0;
    }

    st_str = (std::string)p;
    vecRet.push_back(st_str);
    while (NULL != (p = strtok_r(NULL, mark, &last)))
    {
        st_str = (std::string)p;
        vecRet.push_back(st_str);
    }
    delete ps_temp;

    return 0;
}

// simple unsigned int to string, more slowly than redis method
int CUtils::Uint2String(char* dst, size_t dstlen, unsigned int value)
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

// unix time with misec
long long CUtils::TimeInMilliseconds(void) 
{
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return (((long long)tv.tv_sec)*1000)+(tv.tv_usec/1000);
}

// current time with "yy-mm-dd hh:mm::ss msec.usec"
char* CUtils::GetCurrentTime(void)
{
    static char time[64] = {0};
	struct timeval tv;
	struct tm tm;

	gettimeofday(&tv, NULL);
	localtime_r(&tv.tv_sec, &tm);

	tm.tm_year += 1900;
	tm.tm_mon += 1;

	snprintf(time, sizeof(time), "%4d-%02d-%02d %02d:%02d:%02d %03d.%03d",
		tm.tm_year, tm.tm_mon, tm.tm_mday, tm.tm_hour, 
        tm.tm_min, tm.tm_sec, (unsigned int)(tv.tv_usec / 1000), (unsigned int)(tv.tv_usec % 1000));

    return time;
}

int64_t CUtils::NowMsec(void)
{
    int64_t now_ms = 0;
    struct timeval tv;

    ::gettimeofday(&tv, NULL);
    now_ms = (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
    return now_ms;
}

// show magnitude
char* CUtils::ShowMagnitude(unsigned long val)
{
    double fp = 0.0;   
    static char buf[64];

    if (val >= MILLION)
    {
        fp = (double)val / MILLION; 
        snprintf(buf, sizeof(buf), "%.2f million", fp);
    }
    else if (val >= THOUSAND)
    {
        fp = (double)val / THOUSAND;
        snprintf(buf, sizeof(buf), "%.2f thousand", fp);
    }
    else
    {
        snprintf(buf, sizeof(buf), "%lu", val);
    }

    return buf;
}
