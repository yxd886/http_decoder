#ifndef HTTPPARSE_H_
#define HTTPPARSE_H_

#include "Public.h"
#include "SessionHash.h"
#include "FormatPacket.h"
#include <string>

using namespace std;
union iptrans{
	struct{
		uint8_t x1;
		uint8_t x2;
		uint8_t x3;
		uint8_t x4;
	};
	uint32_t ip;
};

class CHttpParse
{
public:
    CHttpParse();
    ~CHttpParse();

    void  Init();
    void  Parse(CSessionPtr& sesptr);
private:
    bool  ParseMethodAndUri(const char* pBuf, const uint32_t len,uint32_t& pos, CResult& result);
    bool  ParseRspState(const char* pBuf, const uint32_t len, uint32_t& pos, CResult& result);
    bool  ParseHeader(const char* pBuf, const uint32_t len, uint32_t& pos,  HeaderMap& headmap);
    bool  ParseReqData(const char* pBuf, const uint32_t len, uint32_t& pos, CResult& result);
    bool  ParseRspData(const char* pBuf, const uint32_t len, uint32_t& pos, CResult& result);
    uint32_t  GetMethod(string method);
    bool  GetVersion(string version, uint32_t& ver);

    void  Send(CSessionPtr&  sesptr);


    int GetBufByTag(const char* in, const int len, const char* tag, const int tagsize, string& out)
    {
        int i;
        for(i = 0; i< len; i++)
        {
            if(strncmp(in + i, tag, tagsize) == 0)
            {
                out.append(in,i);
                return i; 
            }
        }

        return -1;
    }

};

#endif
