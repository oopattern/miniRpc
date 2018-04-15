#ifndef __SORT_MERGE_H
#define __SORT_MERGE_H

#include <stdio.h>


class CSortMerge
{
public:
    CSortMerge();
    ~CSortMerge();

    // create huge record file
    int InitLargeFile(void);

    // sort record in RAM, split huge record into many files
    int SplitRecord(void);

private:
    // large file name
    const char* LARGE_FILE_NAME = "HUGE_DATA.txt";
    // tmp record dir, use for split record
    const char* TMP_RECORD_DIR = "TMP_RECORD";
    // large file of max record number
    const long long MAX_RECORD_NUM = 10 * MILLION;
    // max record load in RAM once time
    const long long MAX_LOAD_NUM =  100 * THOUSAND;

    // split record file number
    long long m_splitNum;
};

#endif // end of __SORT_MERGE_H
