#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>

const uint32_t  MAX_BUFFER_SIZE = 64 * 1024 * 1024;
const uint32_t  BUFFER_SIZE = 2048;

class CBuffer
{
public:
    CBuffer();
    ~CBuffer();
    bool Append(char* p, size_t size);
    char* GetBuf(uint32_t& size);
    void Reset();
    uint32_t GetBufLen()
    {
        return len; 
    }


private:
    uint32_t len;
    uint32_t _free;
    char*    buf;

};
