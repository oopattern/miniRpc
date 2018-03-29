#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include <limits.h>

// macro
typedef unsigned long long  uint64_t;
typedef unsigned int        uint32_t;
typedef signed int          int32_t;
typedef unsigned short      uint16_t;
typedef signed short        int16_t;
typedef unsigned char       uint8_t;
typedef signed char         int8_t;

// hash function seed
static uint32_t dict_hash_function_seed = 5381;

/* MurmurHash2, by Austin Appleby
 * Note - This code makes a few assumptions about how your machine behaves -
 * 1. We can read a 4-byte value from any address without crashing
 * 2. sizeof(int) == 4
 *
 * And it has a few limitations -
 *
 * 1. It will not work incrementally.
 * 2. It will not produce the same results on little-endian and big-endian
 *    machines.
 */
unsigned int HashKeyFunction(const void *key, int len) 
{
    /* 'm' and 'r' are mixing constants generated offline.
     They're not really 'magic', they just happen to work well.  */
    uint32_t seed = dict_hash_function_seed;
    const uint32_t m = 0x5bd1e995;
    const int r = 24;

    /* Initialize the hash to a 'random' value */
    uint32_t h = seed ^ len;

    /* Mix 4 bytes at a time into the hash */
    const unsigned char *data = (const unsigned char *)key;

    while (len >= 4) 
    {
        uint32_t k = *(uint32_t*)data;

        k *= m;
        k ^= k >> r;
        k *= m;

        h *= m;
        h ^= k;

        data += 4;
        len -= 4;
    }

    /* Handle the last few bytes of the input array  */
    switch (len) 
    {
        case 3: h ^= data[2] << 16;     break;
        case 2: h ^= data[1] << 8;      break;
        case 1: h ^= data[0]; h *= m;   break;
    };

    /* Do a few final mixes of the hash to ensure the last few
     * bytes are well-incorporated. */
    h ^= h >> 13;
    h *= m;
    h ^= h >> 15;

    return (unsigned int)h;
}

/* Return the number of digits of 'v' when converted to string in radix 10.
 * See ll2string() for more information. */
uint32_t digits10(uint64_t v) 
{
    if (v < 10) return 1;
    if (v < 100) return 2;
    if (v < 1000) return 3;
    if (v < 1000000000000UL) 
    {
        if (v < 100000000UL) 
        {
            if (v < 1000000) 
            {
                if (v < 10000) return 4;
                return 5 + (v >= 100000);
            }
            return 7 + (v >= 10000000UL);
        }
        if (v < 10000000000UL) 
        {
            return 9 + (v >= 1000000000UL);
        }
        return 11 + (v >= 100000000000UL);
    }
    return 12 + digits10(v / 1000000000000UL);
}

/* Convert a long long into a string. Returns the number of
 * characters needed to represent the number.
 * If the buffer is not big enough to store the string, 0 is returned.
 *
 * Based on the following article (that apparently does not provide a
 * novel approach but only publicizes an already used technique):
 *
 * https://www.facebook.com/notes/facebook-engineering/three-optimization-tips-for-c/10151361643253920
 *
 * Modified in order to handle signed integers since the original code was
 * designed for unsigned integers. */
int ll2string(char* dst, size_t dstlen, long long svalue) 
{
    static const char digits[201] =
        "0001020304050607080910111213141516171819"
        "2021222324252627282930313233343536373839"
        "4041424344454647484950515253545556575859"
        "6061626364656667686970717273747576777879"
        "8081828384858687888990919293949596979899";
    int negative;
    unsigned long long value;

    /* The main loop works with 64bit unsigned integers for simplicity, so
     * we convert the number here and remember if it is negative. */
    if (svalue < 0) 
    {
        if (svalue != LLONG_MIN) 
        {
            value = -svalue;
        } 
        else 
        {
            value = ((unsigned long long) LLONG_MAX)+1;
        }
        negative = 1;
    } 
    else 
    {
        value = svalue;
        negative = 0;
    }

    /* Check length. */
    uint32_t const length = digits10(value)+negative;
    if (length >= dstlen) return 0;

    /* Null term. */
    uint32_t next = length;
    dst[next] = '\0';
    next--;
    while (value >= 100) 
    {
        int const i = (value % 100) * 2;
        value /= 100;
        dst[next] = digits[i + 1];
        dst[next - 1] = digits[i];
        next -= 2;
    }

    /* Handle last 1-2 digits. */
    if (value < 10) 
    {
        dst[next] = '0' + (uint32_t) value;
    } 
    else 
    {
        int i = (uint32_t) value * 2;
        dst[next] = digits[i + 1];
        dst[next - 1] = digits[i];
    }

    /* Add sign. */
    if (negative) dst[0] = '-';
    return length;
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
        tm.tm_min, tm.tm_sec, (uint32_t)(tv.tv_usec / 1000), (uint32_t)(tv.tv_usec % 1000));
}
