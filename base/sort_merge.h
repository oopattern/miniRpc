#ifndef __SORT_MERGE_H
#define __SORT_MERGE_H

#include <stdio.h>
#include <map> 
#include <unordered_map> // c++11
#include <string>
#include <vector>
#include "block_queue.h"
#include "public.h"
#include "../third_party/btree/btree_map.h"


class CSortMerge
{
public:
    CSortMerge();
    ~CSortMerge();

    // create huge record file
    int InitLargeFile(void);

    // sort record in RAM, split huge record into many files
    int GeneralSplit(void);

    // merge record in file, merge sorted small file int huge file
    int GeneralMerge(void);

    // sort huge file, use bucket method
    int BucketSort(void);

    // sort huge file, use bitmap and hash method
    int BitmapSort(void);

    // sort huge file, use btree method
    int BtreeSort(void);

    // query data, use bitmap
    int BitmapQuery(int key, std::string& result);

    // query data, use btree
    int BtreeQuery(int key, std::string& result);

    // // search compare between map and btree_map
    int SearchCompare(void);

private:
    int SplitRecordSlow(void);
    int BucketMerge(void);
    int SortRecord(const char* filename, std::map<int, string>& recordMap);
    int DumpRecord(const char* filename, const std::map<int, string>& recordMap);
    void MergeRecord(const char* file1, const char* file2);
    void SortThread(void);
    void MergeThread(void);
    bool IsAllMergeFin(void);

    // bitmap operation
    void SetBit(int key);
    void ClearBit(int key);
    bool CheckBit(int key);
    void ShowBitMap(const std::unordered_map<int, int>& umap);

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
    // bucket file num, from 0 - 127, INT_MAX
    const long long BUCKET_NUM = 128;
    // max bitmap ram size: 256Mb
    const long long BITMAP_SIZE = 256 * 1024 * 1024;

    // code not finish...
    // bucket sort or general sort operation
    int         m_splitNum;     // small tmp file, split by huge file
    int         m_mergeTimes;   // merge times, should use atomic var
    CBlockQueue m_fileQueue;    // sorted tmp file, from small tmp file
    CBlockQueue m_mergeQueue;   // merge file, from sort tmp file
    std::map<int,bool> m_bMergeFin; // pthread merge status
    
    // bitmap operation
    char*       m_bitAddr; // bitmap start addr
    std::unordered_map<int, int> m_bitKeyMap; // key: num, val: offset

    // btree operation
    btree::btree_map<int, int> m_btreeKeyMap; // key: num, val: offset
};

#endif // end of __SORT_MERGE_H
