#include "Receiver.h"
#include "BehaviorInfo.h"
#include "FormatPacket.h"
#include "FormatPacket.h"

Receiver::Receiver()
    //_ctx(1),
   // _socket(_ctx,ZMQ_PULL)
{
    _handle.Init();
    //try
   // {

   //     uint64_t hwm = 100000;
   //     zmq_setsockopt((_socket), ZMQ_SNDHWM, &hwm, sizeof (hwm));
     //   _socket.bind("ipc:///tmp/decodehttp");
   // }
  //  catch(zmq::error_t& e)
   // {
        //log e.what() 
  //  }
  //  catch(...)
  //  {
        //log("zmq init error"); 
  //  }
}

Receiver::~Receiver()
{
   // _socket.close();
}


void Receiver::Work(char* msg, CSessionPtr& sesp)
{
    //try
   // {
        //zmq::message_t msg;
       // zmq::pollitem_t item = {_socket,0,ZMQ_POLLIN,0};

       // while(1)
       // {
            //zmq::poll(&item,1,1000000); 
            //if(item.revents & ZMQ_POLLIN)
           // {
         //       _socket.recv(&msg);
	printf("packet %d processing!\n",++counter);
	HandleMessage( msg,sesp);
	//getchar();
          //  }
           // else
          //  {
                //log 没有包到达，去推动最后一组包的解析
                //_handle.Process(NULL,NULL);
         //   }

       // }
   // }
    //catch(zmq::error_t& e)
   // {
        //log e.what() 
   // }
   // catch(...)
   // {
        //log unkonwn error 
   // }

}


void Receiver::HandleMessage(char* msg, CSessionPtr& sesp)
{
    if(msg == NULL)
    {
        cout<<"message is empty, return"<<endl;
    	return;
    }
    //格式化一个二进制包
       CFormatPacket packet;
       packet.Format(msg);
       printf("packet.GetDstPort:%x\n",packet.GetDstPort());
       printf("ntoh packet.GetDstPort:%x\n",ntohs(packet.GetDstPort()));
       CSharedBehaviorInfo info;
       if(ntohs(packet.GetDstPort())==0x50)//if destport is 80
       {
    	  // info.CSharedBehaviorInfo(ntohl(packet.GetDstIp()),ntohl((uint32_t)packet.GetDstPort()),packet.GetIpProtocol());
    	   info.m_nIP=ntohl(packet.GetDstIp());
    	   info.m_nPort=ntohs(packet.GetDstPort());
    	   info.m_nBehaviorId=packet.GetIpProtocol();
    	   info.m_nIdtMatchWay=C2S_MATCH;
       }
       else
       {
    	   //info.CSharedBehaviorInfo(ntohl(packet.GetSrcIp()),ntohs(packet.GetSrcPort()),packet.GetIpProtocol());
    	   info.m_nIP=ntohl(packet.GetSrcIp());
    	   info.m_nPort=ntohs(packet.GetSrcPort());
    	   info.m_nBehaviorId=packet.GetIpProtocol();
    	   info.m_nIdtMatchWay=S2C_MATCH;
       }
       CSharedBehaviorInfo* pInfo=&info;


   // RawPacketInfo*       pRawInfo = (RawPacketInfo*) (msg + sizeof(CSharedBehaviorInfo));
   // printf("length :%x\n",pRawInfo->m_length);
   // printf("sizeof(CSharedBehaviorInfo) :%d\n",sizeof(CSharedBehaviorInfo));
   // printf("sizeof(RawPacketInfo) :%d\n",sizeof(RawPacketInfo));

   // if(pRawInfo->m_length > MAX_RAW_PACKET_SIZE)
  //  {
        //log 
   // 	 cout<<"something wrong, return"<<endl;
   //     return;
   // }


   // SRawPacket tmp;
   // SRawPacket* pRawPkt  = &tmp;
   // pRawPkt->time_val    = pRawInfo->m_timeval;
   // pRawPkt->method      = pRawInfo->m_method;
   // pRawPkt->index       = pRawInfo->m_index;
   // pRawPkt->ethhdr_off  = pRawInfo->m_ethhdr_off;
   // pRawPkt->iphdr_off   = pRawInfo->m_iphdr_off;
   // pRawPkt->tcphdr_off  = pRawInfo->m_tcphdr_off;
   // pRawPkt->length      = pRawInfo->m_length;


   // cout<<"method: "<<pRawPkt->method<<endl;
   // cout<<"index: "<<pRawPkt->index<<endl;
   // cout<<"length: "<<pRawPkt->length<<endl;



    //memcpy(pRawPkt->packet, (char *) (msg+sizeof(RawPacketInfo)+sizeof(CSharedBehaviorInfo)), pRawPkt->length);

    _handle.Process(packet,pInfo,sesp);

    return;
}
