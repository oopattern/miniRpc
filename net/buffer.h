#ifndef __BUFFER_H
#define __BUFFER_H

#include <stdio.h>
#include <stdint.h>
#include <vector>

using std::vector;

class CBuffer
{
public:
    CBuffer();
    ~CBuffer();

    const char* Data(void) const;
    void Append(const char* buf, int32_t len);
    int32_t Remain(void) const; // the rest length in buf
    void Skip(int32_t len); // handle length in buf
    void Clear(void);

private:
    int32_t Size(void) const;

private:
    std::vector<char>   m_content;
    int32_t             m_index; // the rest content to handle, content index start from 0
};

#endif // end of __BUFFER_H

