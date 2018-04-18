#ifndef __SORT_MERGE_H
#define __SORT_MERGE_H

#include <stdio.h>
#include <map>      // 
#include <string>
#include <vector>
#include "block_queue.h"


class CSortMerge
{
public:
    CSortMerge();
    ~CSortMerge();

    // create huge record file
    int InitLargeFile(void);

    // sort record in RAM, split huge record into many files
    int SplitRecordFast(void);

private:
    int DumpRecord(const char* filename, std::map<int, string>& recordMap);
    int SplitRecordSlow(void);
    void SortThread(void);

private:
    // thread finish flag
    const char* FINISH_THREAD_FLAG = "pthread_finish";
    // large file name
    const char* LARGE_FILE_NAME = "HUGE_DATA.txt";
    // tmp record dir, use for split record
    const char* TMP_RECORD_DIR = "TMP_RECORD";
    // large file of max record number
    const long long MAX_RECORD_NUM = 10 * MILLION;
    // max record load in RAM once time
    const long long MAX_LOAD_NUM = 1 * MILLION;
    // split or sort thread num
    const long long SORT_THREAD_NUM = 4;

    // split record file number
    long long   m_splitNum;
    CBlockQueue m_fileQueue;  
};

#endif // end of __SORT_MERGE_H
