#include <stdio.h>
#include <stdlib.h> 
#include <fcntl.h>  // open
#include <unistd.h> // close
#include <sys/stat.h>// mkdir
#include <errno.h>  // errno
#include <string.h> // memcpy, strlen
#include <fstream>  // getline
#include <string>
#include "public.h"
#include "sort_merge.h"

using namespace std;

#define ACCESS_MODE 0777

#ifndef ERROR
#define ERROR   -1
#endif

#ifndef OK
#define OK      0
#endif


CSortMerge::CSortMerge()
{
    m_splitNum = 0;
}

CSortMerge::~CSortMerge()
{

}

int CSortMerge::InitLargeFile(void)
{
    int key = 0;
    int nwrite = 0;
    char line[64] = {0};
    char time[64] = {0};
    
    CUtils::GetCurrentTime(time, sizeof(time));
    printf("Start Time:%s\n", time);

    // create data file with huge record, if file exsist, turncate to zero
    int fd = ::open(LARGE_FILE_NAME, O_RDWR | O_CREAT | O_TRUNC, ACCESS_MODE);
    if (fd < 0)
    {
        printf("open error:%s\n", strerror(errno));
        return ERROR;
    }

    // make up with "key,val" record
    for (long long i = 0; i < MAX_RECORD_NUM; i++)
    {
        key = ::rand();
        snprintf(line, sizeof(line), "%d,record_%d\n", key, key);
        nwrite = strlen(line);
        
        if (nwrite != ::write(fd, line, nwrite))
        {
            printf("write error:%s\n", strerror(errno));
            ::close(fd);
            return ERROR;
        }
    }

    CUtils::GetCurrentTime(time, sizeof(time));
    printf("End   Time:%s\n", time);

    // close the file
    ::close(fd);
    printf("Init large file success!\n");

    return OK;
}

// split huge record into many files
int CSortMerge::SplitRecord(void)
{
    if (0 != ::access(LARGE_FILE_NAME, F_OK))
    {
        printf("large file:%s is not exsist\n", LARGE_FILE_NAME);
        return ERROR;
    }

    // mkdir tmp dir
    if (0 != ::access(TMP_RECORD_DIR, F_OK))
    {
        ::mkdir(TMP_RECORD_DIR, ACCESS_MODE);
        printf("create %s success\n", TMP_RECORD_DIR);
    }

    std::string line;
    fstream f(LARGE_FILE_NAME);

    // read all record, split into many files
    int fd = -1;
    long long count = 0;
    while (getline(f, line))
    {
        // check if need new tmp file
        if (0 == count)
        {
            char filename[64] = {0};
            snprintf(filename, sizeof(filename), "%s/tmp%d.log", TMP_RECORD_DIR, ++m_splitNum);
            fd = ::open(filename, O_RDWR | O_CREAT | O_TRUNC, ACCESS_MODE);
            if (fd < 0)
            {
                printf("split open error:%s\n", strerror(errno));
                return ERROR;
            }
        }

        // append record into tmp file
        int nwrite = line.size();
        if (nwrite != ::write(fd, line.c_str(), nwrite))
        {
            ::close(fd);
            printf("split write error:%s\n", strerror(errno));
            return ERROR;
        }

        // next record
        count++;
        if (count >= MAX_LOAD_NUM)
        {
            // close file, prepare for next new tmp file
            count = 0;
            ::close(fd);
            fd = -1;
        }
    }
    // take care of close last tmp file
    if (fd > 0)
    {
        ::close(fd);
        fd = -1;
    }

    return OK;
}

