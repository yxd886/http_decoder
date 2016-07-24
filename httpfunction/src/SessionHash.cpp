#include "SessionHash.h"


CSessionHashTable::CSessionHashTable()
{
}

CSessionHashTable::~CSessionHashTable()
{
}

void CSessionHashTable::Init()
{
    _cseshash.clear();
    for(uint32_t i = 0; i < 65536; i++)
    {
        _cseshash.push_back(CSessionPtrListPtr(new CSessionPtrList())); 
    }
}

CSessionPtr CSessionHashTable::Find(uint32_t srcIp,uint32_t dstIp, uint16_t srcPort, uint16_t dstPort)
{
    uint32_t tsNow = time(0);
    uint16_t index = HashVal(srcIp,dstIp,srcPort,dstPort);
    CSessionPtrListPtr listPtr = _cseshash[index];

    deque<CSessionPtr>::iterator iter = listPtr->begin();
    for(;iter != listPtr->end();)
    {
        if(IsSameSes(srcIp,dstIp,srcPort,dstPort,*iter))
        {
            (*iter)->RefreshTime = tsNow;
            return *iter;
        } 
        else
        {
            iter++; 
        }
    }

    return CSessionPtr();
}

CSessionPtr CSessionHashTable::Create(IFormatPacket *pPacket,CSharedBehaviorInfo* pInfo)
{
    CSessionPtr   ptr = CSessionPtr(new CSession());
//printf("pInfo->m_nIP:%x\n",pInfo->m_nIP);
//printf("ntohl(pPacket->GetSrcIp()):%x\n",ntohl(pPacket->GetSrcIp()));
//printf("pInfo->m_nPort:%x\n",pInfo->m_nPort);
//printf("ntohs(pPacket->GetSrcPort()):%x\n",ntohs(pPacket->GetSrcPort()));


    if(pInfo->m_nIP == ntohl(pPacket->GetSrcIp()) && pInfo->m_nPort == ntohs(pPacket->GetSrcPort()))
    {
        ptr->ServerIp   = ntohl(pPacket->GetSrcIp());
        ptr->ClientIp   = ntohl(pPacket->GetDstIp()); 
        ptr->ServerPort = ntohs(pPacket->GetSrcPort());
        ptr->ClientPort = ntohs(pPacket->GetDstPort());
        ptr->ServerMac  = pPacket->GetSrcMac();
        ptr->ClientMac  = pPacket->GetDstMac();
        ptr->Protocol   = (PROTOCOL) pInfo->m_nBehaviorId;

        ptr->CreatedTime = time(0);
        ptr->RefreshTime = ptr->CreatedTime;
    }
    else
    {
        ptr->ServerIp   = ntohl(pPacket->GetDstIp());
        ptr->ClientIp   = ntohl(pPacket->GetSrcIp()); 
        ptr->ServerPort = ntohs(pPacket->GetDstPort());
        ptr->ClientPort = ntohs(pPacket->GetSrcPort());
        ptr->ServerMac  = pPacket->GetDstMac();
        ptr->ClientMac  = pPacket->GetSrcMac();
        ptr->Protocol   = (PROTOCOL) pInfo->m_nBehaviorId;

        ptr->CreatedTime = time(0);
        ptr->RefreshTime = ptr->CreatedTime;
    
    }
    uint16_t index = HashVal(ptr->ServerIp,ptr->ClientIp,ptr->ServerPort,ptr->ClientPort);

    _cseshash[index]->push_back(ptr);
    
    return ptr;
}

void CSessionHashTable::Remove(uint32_t srcIp,uint32_t dstIp, uint16_t srcPort,uint16_t dstPort)
{
    
    uint16_t index = HashVal(srcIp,dstIp,srcPort,dstPort);
    CSessionPtrListPtr listPtr = _cseshash[index];

    deque<CSessionPtr>::iterator iter = listPtr->begin();
    for(;iter != listPtr->end();)
    {
        if(IsSameSes(srcIp,dstIp,srcPort,dstPort,*iter))
        {
            iter = listPtr->erase(iter);
            return ;
        } 
        else
        {
            iter++; 
        }
    }
}

uint16_t  CSessionHashTable::HashVal(uint32_t srcIp,uint32_t dstIp, uint16_t srcPort,uint16_t dstPort)
{
    uint32_t h = ((srcIp ^ dstIp) ^ (srcPort ^ dstPort) );
    h ^= h >> 16;
    h ^= h >> 8;
    h &= 0x0000ffff;
    uint16_t r = h;
    return r;
}

bool CSessionHashTable::IsSameSes(uint32_t srcIp,uint32_t dstIp, uint16_t srcPort, uint16_t dstPort,const CSessionPtr& ptr)
{
    return   (
                (ptr->ServerIp == srcIp)     &&
                (ptr->ClientIp == dstIp)     && 
                (ptr->ServerPort == srcPort) && 
                (ptr->ClientPort == dstPort)
             )
             ||
             (
                (ptr->ServerIp == dstIp)     &&
                (ptr->ClientIp == srcIp)     && 
                (ptr->ServerPort == dstPort) && 
                (ptr->ClientPort == srcPort)
             );
}
