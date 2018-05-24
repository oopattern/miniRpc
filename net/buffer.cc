#include <stdio.h>
#include <assert.h>
#include "buffer.h"


CBuffer::CBuffer()
{
    m_content.clear();
    m_index = 0;
}

CBuffer::~CBuffer()
{

}

void CBuffer::Skip(int32_t len)
{
    m_index += len;
    assert(m_index <= Size());
    
    if (m_index >= Size())
    {
        m_content.clear();
        m_index = 0;
    }
}

int32_t CBuffer::Remain(void) const 
{
    assert(m_index <= Size());
    return Size() - m_index;
}

const char* CBuffer::Data(void) const
{
    assert(m_index < Size());
    return &m_content[m_index];   
}

int32_t CBuffer::Size(void) const
{
    return m_content.size();
}

void CBuffer::Append(const char* buf, int32_t len)
{
    m_content.insert(m_content.end(), buf, buf+len);    
}


