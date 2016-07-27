#ifndef SESSIONHASH_H_
#define SESSIONHASH_H_


#include <boost/shared_ptr.hpp>
#include "Public.h"
#include "Buffer.h"
#include "FormatPacket.h"
#include "BehaviorInfo.h"

using namespace std;
using namespace boost;

struct CSession
{
    CSession()
    {
        Reset(); 
    }

    void Reset()
    {
        ReqBuf.Reset();
        RspBuf.Reset();
        Result.Reset();
         
    }
    uint32_t ServerIp;
    uint32_t ClientIp;
    uint16_t ServerPort;
    uint16_t ClientPort;
    uint64_t ServerMac;
    uint64_t ClientMac;

    time_t   CreatedTime;
    time_t   RefreshTime;
    uint32_t SeqNo;
    uint32_t AckSeqNo;

    PROTOCOL Protocol;
    CBuffer  ReqBuf;
    CBuffer  RspBuf;
    CResult  Result;
};


typedef shared_ptr<CSession>         CSessionPtr;
typedef deque<CSessionPtr>           CSessionPtrList;
typedef shared_ptr<CSessionPtrList>  CSessionPtrListPtr;
typedef vector<CSessionPtrListPtr>   CSesHash;


class CSessionHashTable
{
public:
    CSessionHashTable();
    ~CSessionHashTable();
    CSessionPtr Find(uint32_t srcIp,uint32_t dstIp, uint16_t srcPort, uint16_t dstPort);
    CSessionPtr Create(IFormatPacket *pPacket,CBhvInf* pInfo);
    void        Remove(uint32_t srcIp,uint32_t dstIp, uint16_t srcPort,uint16_t dstPort);
    void        Init();
    CSesHash GetHash()
    {
        return _cseshash; 
    }

private:
    uint16_t HashVal(uint32_t srcIp,uint32_t dstIp, uint16_t srcPort, uint16_t dstPort);
    CSesHash _cseshash;
    bool     IsSameSes(uint32_t srcIp,uint32_t dstIp, uint16_t srcPort, uint16_t dstPort, const CSessionPtr& ptr);
};

#endif
