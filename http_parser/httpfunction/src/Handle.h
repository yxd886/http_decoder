#ifndef HANDLE_H_
#define HANDLE_H_

#include "Public.h"
#include "BehaviorInfo.h"
#include "FormatPacket.h"
#include "SessionHash.h"
#include "HttpParse.h"

class CHandle
{
public:
    CHandle();
    ~CHandle();
    void Init();
    void Process(CFormatPacket packet, CSharedBehaviorInfo* pInfo, CSessionPtr& sesp);

private:
    void TimeOutCheck();

    CSessionHashTable  _sesHash;
    CHttpParse         _httpParse;
    time_t             _lastTimeOutCheck;

};
#endif
