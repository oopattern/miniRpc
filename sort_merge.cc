#include <stdio.h>
#include <stdlib.h> 
#include <fcntl.h>  // open
#include <unistd.h> // close
#include <limits.h> // INT_MAX
#include <sys/stat.h>// mkdir
#include <assert.h> // assert
#include <errno.h>  // errno
#include <string.h> // memcpy, strlen
#include <fstream>  // getline
#include "public.h"
#include "thread.h"
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
    m_mergeTimes = 0;
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
    
    printf("Start Time:%s\n", CUtils::GetCurrentTime());

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

    printf("End   Time:%s\n", CUtils::GetCurrentTime());

    // close the file
    ::close(fd);
    printf("Init large file success!\n");

    return OK;
}

// sort record from small file, output to recordMap
int CSortMerge::SortRecord(const char* filename, std::map<int, string>& recordMap)
{
    assert(filename != NULL);

    std::string line;
    fstream f(filename);        

    // sort record
    while (getline(f, line))
    {
        // sort record
        std::vector<string> vec;
        CUtils::SplitStr(line.c_str(), (char*)",", vec);
        if (vec.size() != 2)
        {
            printf("record format error\n");
            return ERROR;
        }
        
        int key = ::atoi(vec[0].c_str());
        recordMap[key] = vec[1];
    }

    return OK;
}

int CSortMerge::DumpRecord(const char* filename, const std::map<int, string>& recordMap)
{
    int fd = ::open(filename, O_RDWR | O_CREAT | O_TRUNC, ACCESS_MODE);
    if (fd < 0)
    {
        printf("split open error:%s\n", strerror(errno));
        return ERROR;
    }

    std::map<int, string>::const_iterator it;
    for (it = recordMap.begin(); it != recordMap.end(); ++it)
    {
        // to_string: c++ 11
        std::string record = to_string(it->first);
        record += ",";
        record += it->second;
        record += "\n";

        // append record into tmp file
        int nwrite = record.size();
        if (nwrite != ::write(fd, record.c_str(), nwrite))
        {
            ::close(fd);
            printf("split write error:%s\n", strerror(errno));
            return ERROR;
        }
    }

    ::close(fd);
    return OK;
}

void CSortMerge::MergeRecord(const char* file1, const char* file2)
{
    assert((file1 != NULL) && (file2 != NULL));

    char filename[64] = {0};
    snprintf(filename, sizeof(filename), "TMP_RECORD/merge%d.log", ++m_mergeTimes);
    
    std::string line;
    fstream f1(file1);
    fstream f2(file2);

    printf("tid=%d merge file=%s start time: %s\n", CThread::Tid(), filename, CUtils::GetCurrentTime());

    int mfd = -1;        
    mfd = ::open(filename, O_RDWR | O_CREAT | O_TRUNC, ACCESS_MODE);
    if (mfd < 0)
    {
        printf("merge thread open error:%s\n", strerror(errno));    
        return;
    }

    int key1 = 0;
    int key2 = 0;
    bool over1 = false;
    bool over2 = false;
    std::string line1;
    std::string line2;
    std::vector<string> vec1;
    std::vector<string> vec2;

    while (!over1 || !over2)
    {
        // first read two file head line
        if ((0 == key1) && (0 == key2))
        {
            getline(f1, line1);
            getline(f2, line2);
            CUtils::SplitStr(line1.c_str(), (char*)",", vec1);
            CUtils::SplitStr(line2.c_str(), (char*)",", vec2);
            assert(2 == vec1.size() && 2 == vec2.size());
            key1 = ::atoi(vec1[0].c_str());
            key2 = ::atoi(vec2[0].c_str());
        }
        else
        {
            // key1 <= key2, continue read file1
            if (key1 <= key2)
            {
                std::vector<string> vec1;
                if (getline(f1, line1))
                {
                    CUtils::SplitStr(line1.c_str(), (char*)",", vec1);
                    assert(2 == vec1.size());
                    key1 = ::atoi(vec1[0].c_str());
                }
                else
                {
                    over1 = true;
                    key1 = INT_MAX;
                }
            }
            // key1 > key2, continue read file2
            else
            {
                std::vector<string> vec2;          
                if (getline(f2, line2))
                {
                    CUtils::SplitStr(line2.c_str(), (char*)",", vec2);
                    assert(2 == vec2.size());
                    key2 = ::atoi(vec2[0].c_str());
                }
                else
                {
                    over2 = true;
                    key2 = INT_MAX;
                }
            }
        }
        
        // sort record from min to max
        std::string record = ((key1 <= key2) ? (line1+"\n") : (line2+"\n"));
        int nwrite = record.size();
        if (nwrite != ::write(mfd, record.c_str(), nwrite))
        {
            ::close(mfd);
            printf("merge write error:%s\n", strerror(errno));
            return;
        }
    }

    // finish merge, close file
    ::close(mfd);

    // gather merge file
    m_mergeQueue.Put(filename);

    printf("tid=%d merge file=%s end   time: %s\n", CThread::Tid(), filename, CUtils::GetCurrentTime());
}

// merge file from sorted tmp file
void CSortMerge::MergeThread(void)
{
    while (1)
    {        
        // if remain only one file, probably merge finish
        if (1 == m_mergeQueue.Size())
        {
            printf("tid=%d finish merge, thread quit\n", CThread::Tid());
            return;
        }

        // merge two sort file, timeout 2 sec
        std::vector<string> tbl;
        tbl = m_mergeQueue.TakeMutli(2, 2);

        if (2 == tbl.size())
        {
            MergeRecord(tbl[0].c_str(), tbl[1].c_str());
        }
    }
}

// pthread for sort record, for small tmp file
void CSortMerge::SortThread(void)
{
    while (1)
    {
        std::string file = m_fileQueue.Take();
        if (FINISH_THREAD_FLAG == file)
        {
            printf("tid=%d sort thread quit\n", CThread::Tid());
            return;
        }

        // checkout if file exsist
        if (0 != ::access(file.c_str(), F_OK))
        {
            printf("record file:%s is not exsist\n", file.c_str());
            continue;
        }            

        printf("tid=%d dump file=%s start time: %s\n", CThread::Tid(), file.c_str(), CUtils::GetCurrentTime());

        // sort record from small file
        std::map<int, string> recordMap;
        SortRecord(file.c_str(), recordMap);

        // dump sort record to file
        char filename[64] = {0};
        snprintf(filename, sizeof(filename), "%s_sort", file.c_str());
        DumpRecord(filename, recordMap);
        recordMap.clear();

        // gather merge file
        m_mergeQueue.Put(filename);

        printf("tid=%d dump file=%s end   time: %s\n", CThread::Tid(), file.c_str(), CUtils::GetCurrentTime());
    }
}

// after split huge file to small file
int CSortMerge::Merge(void)
{
    printf("Merge Record Start Time: %s\n", CUtils::GetCurrentTime());

    // start pthread pool
    CThreadPool pool(SORT_THREAD_NUM, std::bind(&CSortMerge::MergeThread, this));
    pool.StartAll();

    printf("wait for all merge pthread done...\n");

    // join all pthread 
    pool.JoinAll();

    printf("Merge Record End   Time: %s\n", CUtils::GetCurrentTime());
    return 0;
}

// split huge record into many files
int CSortMerge::Split(void)
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

    printf("Split Record Start Time: %s\n", CUtils::GetCurrentTime());

    std::string line;
    fstream f(LARGE_FILE_NAME);

    // read all record, split into many files
    int fd = -1;
    int nwrite = 0;
    long long count = 0;
    std::string fileRecord;

    // start pthread pool
    CThreadPool pool(SORT_THREAD_NUM, std::bind(&CSortMerge::SortThread, this));
    pool.StartAll();

    while (getline(f, line))
    {
        // create new tmp file
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

            fileRecord = filename;
        }

        // append record on tmp file
        line += "\n";
        nwrite = line.size();
        if (nwrite != ::write(fd, line.c_str(), nwrite))
        {
            ::close(fd);
            printf("split write error:%s\n", strerror(errno));
            return ERROR;
        }

        // close tmp file
        count++;
        if (count >= MAX_LOAD_NUM)
        {
            count = 0;
            ::close(fd);            
            fd = -1;

            // add file record, protect by mutex
            m_fileQueue.Put(fileRecord);
        }
    }
    // finish close last tmp file
    if (fd > 0)
    {
        ::close(fd);
        fd = -1;

        // add file record, protect by mutex
        m_fileQueue.Put(fileRecord);
    }

    // add pthread finish flag, notify pthread to quit
    for (int i = 0; i < SORT_THREAD_NUM; i++)
    {
        m_fileQueue.Put(FINISH_THREAD_FLAG);
    }

    printf("wait for all sort pthread done...\n");

    // join all pthread 
    pool.JoinAll();
    
    printf("Split Record End   Time: %s\n", CUtils::GetCurrentTime());
    return OK;
}

int CSortMerge::SplitRecordSlow(void)
{
    std::map<int, string> recordMap;
    std::string line;
    char time[64] = {0};
    
    fstream f(LARGE_FILE_NAME);

    printf("Split Record Start Time: %s\n", CUtils::GetCurrentTime());

    long long count = 0;
    while (getline(f, line))
    {
        // sort record
        std::vector<string> vec;
        CUtils::SplitStr(line.c_str(), (char*)",", vec);
        if (vec.size() != 2)
        {
            printf("record format error\n");
            return ERROR;
        }
        
        int key = ::atoi(vec[0].c_str());
        recordMap[key] = vec[1];

        // put record into tmp file
        count++;
        if (count >= MAX_LOAD_NUM)
        {
            char filename[64] = {0};
            snprintf(filename, sizeof(filename), "%s/tmp%d.log", TMP_RECORD_DIR, ++m_splitNum);

            printf("[%s] dump record start time: %s\n", filename, CUtils::GetCurrentTime());
            DumpRecord(filename, recordMap);
            printf("[%s] dump record end   time: %s\n", filename, CUtils::GetCurrentTime());

            count = 0;
            recordMap.clear();
        }
    }
    // rest record put into last tmp file
    if (count > 0)
    {
        char filename[64] = {0};
        snprintf(filename, sizeof(filename), "%s/tmp%d.log", TMP_RECORD_DIR, ++m_splitNum);
        DumpRecord(filename, recordMap);
    }

    printf("Split Record End   Time: %s\n", CUtils::GetCurrentTime());

    return OK;
}

