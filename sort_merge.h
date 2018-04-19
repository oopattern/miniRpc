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
    int SortRecord(const char* filename, std::map<int, string>& recordMap);
    int DumpRecord(const char* filename, const std::map<int, string>& recordMap);
    int SplitRecordSlow(void);
    void SortThread(void);
    void MergeThread(void);

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
    long long   m_splitNum;     // small tmp file, split by huge file
    long long   m_mergeTimes;   // merge times, should use atomic var
    CBlockQueue m_fileQueue;    // sorted tmp file, from small tmp file
    CBlockQueue m_mergeQueue;   // merge file, from sort tmp file
};

#endif // end of __SORT_MERGE_H
