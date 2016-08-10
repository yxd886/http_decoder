/*-
 *   BSD LICENSE
 *
 *   Copyright(c) 2010-2014 Intel Corporation. All rights reserved.
 *   All rights reserved.
 *
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of Intel Corporation nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <sys/queue.h>
#include <rte_config.h>
#include <rte_memory.h>
#include <rte_memzone.h>
#include <rte_launch.h>
#include <rte_eal.h>
#include <rte_per_lcore.h>
#include <rte_lcore.h>
#include <rte_debug.h>
#include "Public.h"
#include "Receiver.h"
#include <netinet/ip6.h>


#include <string>
#include <iostream>

#include "caf/all.hpp"

using std::endl;
using std::string;
using std::cout;
using std::pair;
using namespace caf;

using start_atom = atom_constant<atom("start")>;

static int
lcore_hello(__attribute__((unused)) void *arg)
{
	unsigned lcore_id;
	lcore_id = rte_lcore_id();
	printf("hello from core %u\n", lcore_id);
	return 0;
}



class http_parser : public event_based_actor{
public:
  http_parser(actor_config& cfg):event_based_actor(cfg)
{

    rcv = Receiver();
    fs = CSessionPtr(new CSession());

}

    behavior make_behavior() override {
        //return http_parser_fun(this);
     // send(this, step_atom::value);
    // philosophers start to think after receiving {think}
     // become(normal_task());
    //  become(keep_behavior, reconnecting());
    return behavior{

      [=](start_atom) {
          start();

      }

    };
    
  
  
}

 Receiver  rcv;
 CSessionPtr  fs;


void start()
{
     //char** a;
    //a[1]=(char*)malloc(sizeof(pkt1));a[1]=(char*)pkt1;

  struct ether_header *m_pEthhdr;
  struct iphdr *m_pIphdr;
  struct ip6_hdr *ip6hdr;
    char tmp1[2000];
    char *head=tmp1;
    char *iphead=tmp1+14;
    char *ip4packet=tmp1+34;
    char *ip6packet=tmp1+54;
    uint16_t len;


    struct ether_iphdr* hd;
    

   FILE* f;
  if( (f=fopen("/home/net/nf-actor/actor-framework/examples/nfactor/code.txt","rb"))==NULL)
    {
    printf("OPen File failure\n");
    }

   while (!feof(f))
   {
     fread(head,sizeof(char),14,f);
    // fwrite(head,14,1,p);
    // fclose(p);
    // getchar();
     m_pEthhdr=(struct ether_header *)head;
     cout<<hex<<ntohs(m_pEthhdr->ether_type)<<endl;
     if(ntohs(m_pEthhdr->ether_type)==ETHERTYPE_IP)
     {
       cout<<"type: ipv4:"<<hex<<ntohs(m_pEthhdr->ether_type)<<endl;
       fread(iphead,sizeof(char),20,f);
       m_pIphdr=(struct iphdr *)(head+sizeof(struct ether_header));
       len = ntohs(m_pIphdr->tot_len);
       cout<<"len:"<<len<<endl;
       fread(ip4packet,len-20,1,f);
       rcv.Work(head,fs);
     }else if(m_pEthhdr->ether_type==ETHERTYPE_IPV6)
     {
       fread(iphead,40,1,f);
       ip6hdr=(struct ip6_hdr *)(head+sizeof(struct ether_header));
       len = ntohs(ip6hdr->ip6_ctlun.ip6_un1.ip6_un1_plen);
       fread(ip6packet,len,1,f);
       rcv.Work(head,fs);
     }

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



};






void caf_main(actor_system& system) {
	int ret;
	unsigned lcore_id;
	char c[] = {"./build/bin/hellodpdk"};
	char* t = c;
	ret = rte_eal_init(1, &t);
	if (ret < 0)
		rte_panic("Cannot init EAL\n");
	/* call lcore_hello() on every slave lcore */
	RTE_LCORE_FOREACH_SLAVE(lcore_id) {
		rte_eal_remote_launch(lcore_hello, NULL, lcore_id);
	}

	/* call it on master lcore too */
	lcore_hello(NULL);

	rte_eal_mp_wait_lcore();

	// our CAF environment
  auto http_decoder=system.spawn<http_parser>();
  auon_send(decoder,start_atom::value);
}

CAF_MAIN()