#include "Buffer.h"


CBuffer::CBuffer():
    len(0),_free(0),buf(NULL)
{
    Reset();
}

CBuffer::~CBuffer()
{
    if(buf)
    {
        free(buf);
        buf = NULL;
    }
}

void CBuffer::Reset()
{
    if(!buf)
    {
        buf = (char*) malloc(BUFFER_SIZE);
        memset(buf,0x00,_free);
    }

    if(len > BUFFER_SIZE * 2 && buf)
    {
        //如果目前buf的大小是默认值的2倍，则对其裁剪内存，保持buf的大小为默认值，减小内存耗费
        char* newbuf = (char*) realloc(buf,BUFFER_SIZE);
        if(newbuf != buf)
            buf = newbuf;
    }

    len = 0;
    _free = BUFFER_SIZE;
}

bool CBuffer::Append(char* p, size_t size)
{
    if(!p || !size)
        return true;
    if(size < _free)
    {
        memcpy(buf + len, p , size); 
        len += size;
        _free -= size;
    }
    else
    {
        char* newbuf = (char*) realloc(buf,len + size + 1);
        if(!newbuf)
            return false;

        if(buf != newbuf)
        {
            buf = newbuf;
        }

        memcpy(buf + len, p, size); 
        len += size;
        _free = 1;
        buf[len] = '\0';

    }

    return true;
}

char* CBuffer::GetBuf(uint32_t& size)
{
    size = len;
    return buf;
}
