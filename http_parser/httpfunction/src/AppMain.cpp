#include "Public.h"
#include "Receiver.h"

int main(int argc, char** argv)
{
     //char** a;
    //a[1]=(char*)malloc(sizeof(pkt1));a[1]=(char*)pkt1;

	struct ether_header *m_pEthhdr;
	struct iphdr *m_pIphdr;
    Receiver  rcv;
    CSessionPtr fs = CSessionPtr( new CSession());
    char tmp1[2000];
    char *head=tmp1;
    char *packet=tmp1+34;
    uint16_t len;


    

    //char a[1000]={"16:32:43.078599 IP sunmmer-VirtualBox.mshome.net.35234 > 42.156.145.13.http: Flags [.], ack 1636869017, win 237, length 0"};
  
   



    struct ether_iphdr* hd;
  	

   FILE* f;
  if( (f=fopen("../../code.txt","r"))==NULL)
	  {
	  printf("OPen File failure\n");
	  }

   while (!feof(f))
   {
	   fread(head,34,1,f);
	   m_pEthhdr=(struct ether_header *)head;
	   m_pIphdr=(struct iphdr *)(head+sizeof(struct ether_header));
	   len = ntohs(m_pIphdr->tot_len);
	   cout<<"len:"<<len<<endl;
	   fread(packet,len-20,1,f);
	   rcv.Work(head,fs);
  }
    
   uint32_t length;
    //printf("Server IP: %x\n",fs->ServerIp );
   // printf("Client IP: %x\n",fs->ClientIp );
   // printf("Server port: %x\n",fs->ServerPort);
   // printf("Client port: %x\n",fs->ClientPort );

  char* rsp=fs->RspBuf.GetBuf(length);
  printf("Response Buffer length: %d\n",length);
  for(int i=0;i<len;i++)
   {
   	fprintf(f,"%c",*rsp);
    	rsp++;
   }
    


    fclose(f);

}

