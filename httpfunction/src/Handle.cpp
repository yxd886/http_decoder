#include "Handle.h"


CHandle::CHandle()
{
}

CHandle::~CHandle()
{
}

void CHandle::Init()
{
    _sesHash.Init();
    _httpParse.Init();
    _lastTimeOutCheck = time(0);
}

void CHandle::Process(CFormatPacket packet, CSharedBehaviorInfo* pInfo, CSessionPtr& sesp)
{

    TimeOutCheck(); //超时检测，用于会话链的老化

    if( !pInfo)
    {
        //log handle.process arguament is null
        return;
    }





    uint32_t srcIp = ntohl(packet.GetSrcIp());
    uint32_t dstIp = ntohl(packet.GetDstIp());
    uint16_t srcPort = ntohs(packet.GetSrcPort());
    uint16_t dstPort = ntohs(packet.GetDstPort());

    //从会话链中查找该会话
    CSessionPtr sesptr = _sesHash.Find(srcIp,dstIp,srcPort,dstPort);

    if(sesptr.get() == NULL)
    {
        //如果不存在，则创建新会话 
    	printf("new session created!\n");
    	//getchar();
        sesptr = _sesHash.Create(&packet,pInfo); 
        if(sesptr.get() == NULL)
        {
            //log  create session error 
            return;
        }
    }

    //开始组包
 
    if(packet.GetTcphdr()->fin == 1 || packet.GetTcphdr()->rst == 1)
    {
        //log 会话结束,session over 
    	printf("session over\n");
        _httpParse.Parse(sesptr);
        _sesHash.Remove(srcIp,dstIp,srcPort,dstPort);
        return;
    }

    if(sesptr->SeqNo == ntohl(packet.GetTcphdr()->seq))
    {
        //log repeated packet  重复的包
    	printf("repeated packet!\n");
        return;
    }

    if(packet.GetDataLen() == 0)
    {
        //log zero length
    	printf("data zero!\n");
        return; 
    }

    sesptr->SeqNo    = ntohl(packet.GetTcphdr()->seq);
    sesptr->AckSeqNo = ntohl(packet.GetTcphdr()->ack_seq);

    if(pInfo->m_nIdtMatchWay == UNK_MATCH)
    {
        //log unknown match 
    	printf("unknow match!\n");
        return;
    }
    else if(pInfo->m_nIdtMatchWay == C2S_MATCH)
    {
        printf("C2S\n");
    	if(sesptr->ReqBuf.GetBufLen() > 0 && sesptr->RspBuf.GetBufLen() > 0)
        {
            printf("enter sesptr->ReqBuf.GetBufLen() > 0 && sesptr->RspBuf.GetBufLen() > 0\n");
        	_httpParse.Parse(sesptr);
        }

        if(sesptr->ReqBuf.GetBufLen() == 0 && sesptr->Result.RequestTimeStamp == 0)
        {
            //the first request packet. we will get timestamp from this packet
        	printf("first request packet!\n");
            sesptr->Result.RequestTimeStamp = packet.GetPacketTime()->tv_sec * 1000000LL + packet.GetPacketTime()->tv_usec;
        }
        printf("appending request buffer!\n");
        if(!sesptr->ReqBuf.Append((char*) packet.GetData(), (size_t) packet.GetDataLen()))
        {

        	//log  c2s append date error
            return;
        }
    }
    else if(pInfo->m_nIdtMatchWay == S2C_MATCH)
    {
    	printf("S2C\n");
    	if(sesptr->RspBuf.GetBufLen() == 0 && sesptr->Result.ResponseTimeStamp == 0)
        {
            //the first response packet. we will get timestamp from this packet
    		printf("first response packet!\n");
            sesptr->Result.ResponseTimeStamp = packet.GetPacketTime()->tv_sec * 1000000LL + packet.GetPacketTime()->tv_usec;
        }

    	printf("appending respone buffer!\n");
    	if(!sesptr->RspBuf.Append((char*) packet.GetData(), (size_t) packet.GetDataLen()))
        {
            //log  c2s append date error 
            return;
        }
    }
    sesp=sesptr;
    unsigned int i;
    printf("session request buffer:%s\n\n\n\n",sesptr->ReqBuf.GetBuf(i));
   // getchar();
    printf("session response buffer:%s\n",sesptr->RspBuf.GetBuf(i));
    return;
}

void CHandle::TimeOutCheck()
{
    time_t tsNow = time(0);
    if(tsNow - _lastTimeOutCheck < TIMEOUT_CHECK_DUR)
    {
        //每隔2分钟对会话链进行一次超时检查，每次检查需要遍历整个会话链 
        return;
    }


    //遍历整个vector
    for(uint32_t i = 0; i < 65535; i++)
    {
        CSessionPtrListPtr listPtr = _sesHash.GetHash()[i];
        CSessionPtrList::iterator it = listPtr->begin();
        for(; it != listPtr->end();)
        {
            if(((*it)->ReqBuf.GetBufLen() > 0  || (*it)->RspBuf.GetBufLen() > 0) && tsNow - (*it)->RefreshTime > 60)
            {
                _httpParse.Parse(*it); 
            }

            if(tsNow - (*it)->RefreshTime >= TIMEOUT)
            {
                it = listPtr->erase(it);
            }
            else
            {
                it++; 
            }
        }
    }
}
