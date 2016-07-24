/**  @file  RawPacket.cpp
 *   @brief
 *
 *   Detailed description starts here.
 *
 *   @internal www.captech.net.cn
 *   author:  Chen Zheyi
 *   mail:    chenzheyi@captech.net.cn
 *   Created:  01/21/2008
 *   Revision:  1.0.0
 *   Compiler:  gcc/g++
 *   Company:  Captech Co., Ltd.
 *   Copyright:  Copyright (c) 2008, Captech Co., Ltd.
 *
 * =====================================================================================
 */

#include "FormatPacket.h"

void CFormatPacket::Format(char* packet)
{
    //m_pPacketData = pRawPacket;

    m_pPkt = packet;

   // if(m_pPacketData->ethhdr_off < 0)
  //  {
  //      m_pEthhdr = NULL;
  //  }
   // else
   // {
        m_pEthhdr = (struct ether_header*)packet;
  //  }

   // if(m_pPacketData->iphdr_off < 0)
   // {
       // m_pIphdr = NULL;
   // }
   // else
   // {
        m_pIphdr = (struct iphdr*)(packet + sizeof(struct ether_header));
   // }

   // if(m_pPacketData->tcphdr_off < 0)
   // {
    //    m_pTcphdr = NULL;
   // }
   // else
   // {
        m_pTcphdr = (struct tcphdr*)(packet + sizeof(struct ether_header)+(m_pIphdr->ihl)*4);
   // }
    m_uPktLen = ntohs(m_pIphdr->tot_len);
    m_pEthIndex = (int16_t*)(&m_pEthhdr->ether_type);

    m_pData = NULL;
    m_DataLen = 0;
    if (m_pIphdr)
    {
		int16_t iplen=ntohs(m_pIphdr->tot_len);
		int16_t offset;
		if (m_pIphdr->protocol == IPPROTO_TCP)
			offset = m_pIphdr->ihl * 4 + m_pTcphdr->doff * 4;
		else
			offset = m_pIphdr->ihl * 4 + 8;
		m_pData = (u_int8_t *)(packet + sizeof(struct ether_header) + offset);
		m_DataLen = iplen - offset;
		gettimeofday(&_time,NULL);
    }
    return;
}

/**
 *  Get ether header pointer
 *  @return
 *      \n NULL failed / not exist
 *      \n point to ether header in packet
 */
struct ether_header *CFormatPacket::GetEtherHeader()
{
    return m_pEthhdr;
}

/**
 *  Get Destination MAC Address
 *  @return
 *      \n NULL :failed / no destinatiom MAC address
 *      \n pointer to ether destination MAC address
 */
u_int64_t CFormatPacket::GetDstMac()
{
    if(m_pEthhdr)
    {
        //return BCD2UInt64(m_pEthhdr->ether_dhost, 6);
        return 0;
    }
    else
    {
        return 0;
    }
}

/**
 *  Get Source MAC Address
 *  @return
 *      \n NULL :failed / no source MAC address
 *      \n pointer to ether source MAC address
 */
u_int64_t CFormatPacket::GetSrcMac()
{
    if(m_pEthhdr)
    {
        //return BCD2UInt64(m_pEthhdr->ether_shost, 6);
        return 0;
    }
    else
    {
        return 0;
    }
}

/**
 *  Get IP header pointer
 *  @return
 *      \n NULL failed / not exist
 *      \n point to IP header in packet
 */
struct iphdr *CFormatPacket::GetIphdr()
{
    return m_pIphdr;
}

/**
 *  Get Destination IP Address
 *  @return
 *      \n NULL :failed / no destinatiom IP address
 *      \n pointer to destination IP address
 */
u_int32_t CFormatPacket::GetDstIp()
{
    if(m_pIphdr)
    {
        return m_pIphdr->daddr;
    }
    else
    {
        return 0;
    }
}

/**
 *  Get Source IP Address
 *  @return
 *      \n NULL :failed / no source IP address
 *      \n pointer to source IP address
 */
u_int32_t CFormatPacket::GetSrcIp()
{
    if(m_pIphdr)
    {
        return m_pIphdr->saddr;
    }
    else
    {
        return 0;
    }
}

/**
 *  Get IP protocol in ip header
 *  @return
 *      \n NULL :failed / no exist
 *      \n pointer to IP protocol
 */
u_int8_t  CFormatPacket::GetIpProtocol()
{
    if(m_pIphdr)
    {
        return m_pIphdr->protocol;
    }
    else
    {
        return 0;
    }
}

u_int16_t CFormatPacket::GetIpPktLen()
{
	return m_uPktLen;
}
/**
 *  Get TCP header pointer
 *  @return
 *      \n NULL failed / not exist
 *      \n point to TCP header in packet
 */
struct tcphdr *CFormatPacket::GetTcphdr()
{
    return m_pTcphdr;
}

/**
 *  Get UDP header pointer
 *  @return
 *      \n NULL failed / not exist
 *      \n point to UDP header in packet
 */
struct udphdr *CFormatPacket::GetUdphdr()
{
    return (struct udphdr *)m_pTcphdr;
}

/**
 *  Get Destination TCP/UDP port
 *  @return
 *      \n NULL :failed / no destinatiom TCP/UDP port
 *      \n pointer to destination TCP/UDP port
 */
u_int16_t CFormatPacket::GetDstPort()
{
    if(m_pTcphdr)
    {
        return m_pTcphdr->dest;
    }
    else
    {
        return 0;
    }
}

/**
 *  Get Source TCP/UDP port
 *  @return
 *      \n NULL :failed / no Source TCP/UDP port
 *      \n pointer to Source TCP/UDP port
 */
u_int16_t CFormatPacket::GetSrcPort()
{
    if(m_pTcphdr)
    {
        return m_pTcphdr->source;
    }
    else
    {
        return 0;
    }
}


