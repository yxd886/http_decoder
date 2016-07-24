#ifndef FORMATPACKET_H_
#define FORMATPACKET_H_

#include <net/if.h>
#include <sys/types.h>
#include <netinet/ether.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <linux/if_pppox.h>
#include <linux/ppp_defs.h>
#include <linux/if_ether.h>
#include <sys/time.h>
#include <stdint.h>
#include "Public.h"


#define MAX_RAW_PACKET_SIZE 4096  /**< \brief maximun raw packet size */
#define PKT_METHOD_UNKNOWN 0 /**< \brief error, should not be unknown */
#define PKT_METHOD_SOCKET 1 /**< \brief received from packet_rx_ring socket */
#define PKT_METHOD_CAPFILE 2 /**< \brief received from lipbpcap file */
#define PKT_METHOD_LIBPCAP 3 /**< \brief received by libpcap */
#define PKT_METHOD_IPTABLES 4 /**< \brief received from iptables queue */

#define PKT_FLAG_USED 1 /**< \brief has packet */
#define PKT_FLAG_FREE 0 /**< \brief no packet */
#define PKT_FLAG_TRANSMIT 3/**< \brief transmit packet */
#define PKT_FLAG_DROP 4/**< \brief drop packet */

#define PKT_DIRECTION_UNKNOWN  0 /**< \brief packet direction unknown */
#define PKT_DIRECTION_LAN_TO_WAN   1 /**< \brief packet is sent from lan to wan */
#define PKT_DIRECTION_WAN_TO_LAN   2 /**< \brief packet is sent from wan to lan */
#define PKT_DIRECTION_LAN_TO_LAN   3 /**< \brief packet is sent from lan to lan */
#define PKT_DIRECTION_WAN_TO_WAN   4 /**< \brief packet is sent from wan to wan */

#define IP_HDR_PROTOCOL_OFF       9
#define VLAN_8021Q_IDT_LEN        4
#define VLAN_8021Q_IDT_EXTRA_LEN  2

#define PPPOE_HDR_LEN 8
#define ETH_PKT_LEN_MIN          54
#define ETH_PKT_LEN_MAX         1522


struct RawPacketInfo
{
    timeval  m_timeval;
    uint32_t m_method;
    uint32_t m_index;
    uint32_t m_length;
    uint32_t m_ethhdr_off;
    uint32_t m_iphdr_off;
    uint32_t m_tcphdr_off;
};

/**
 *  Struct for packet communication
 *   */
typedef struct
{
    int32_t flag; /**< \brief indicate packet element have received packet*/

    /**
    *  *  indicate packet receive method.
    *  *  @see PKT_METHOD_UNKNOWN
    *  *  @see PKT_METHOD_SOCKET
    *  *  @see PKT_METHOD_CAPFILE
    *  *  @see PKT_METHOD_LIBPCAP
    *  *  @see PKT_METHOD_IPTABLES
    *  */
    int16_t method;

    int16_t index; /**< \brief network interface index where packet received */

    int16_t ethhdr_off;
    int16_t iphdr_off;
    int16_t tcphdr_off;

    int16_t direct;

    int16_t length; /**< \brief real packet bytes in buffer */

    struct timeval time_val;

    char  packet[MAX_RAW_PACKET_SIZE]; /**< \brief packet content. @see MAX_RAW_PACKET_SIZE */
} SRawPacket;



class IFormatPacket
{
public:
    virtual ~IFormatPacket(){};
    virtual void Format(char *packet) = 0;
    virtual struct ether_header *GetEtherHeader() = 0;
    virtual u_int64_t GetDstMac() = 0;
    virtual u_int64_t GetSrcMac() = 0;

    virtual struct iphdr *GetIphdr() = 0;
    virtual u_int32_t GetDstIp() = 0;
    virtual u_int32_t GetSrcIp() = 0;
    virtual u_int8_t  GetIpProtocol() = 0;
    virtual u_int16_t GetIpPktLen() = 0;

    virtual struct tcphdr *GetTcphdr() = 0;
    virtual struct udphdr *GetUdphdr() = 0;
    virtual u_int16_t GetDstPort() = 0;
    virtual u_int16_t GetSrcPort() = 0;
    //virtual int16_t GetDirect() = 0;
    virtual u_int8_t *GetData() = 0;
    virtual int16_t  GetDataLen() = 0;
   // virtual SRawPacket *GetRawPacket() = 0;
};



class CFormatPacket : public IFormatPacket
{
public:
    void Format(char *packet);
    struct ether_header *GetEtherHeader();
    u_int64_t GetDstMac();
    u_int64_t GetSrcMac();

    struct iphdr *GetIphdr();
    u_int32_t GetDstIp();
    u_int32_t GetSrcIp();
    u_int8_t  GetIpProtocol();
    u_int16_t GetIpPktLen();

    struct tcphdr *GetTcphdr();
    struct udphdr *GetUdphdr();
    u_int16_t GetDstPort();
    u_int16_t GetSrcPort();
    //int16_t GetDirect(){return m_pPacketData->direct;}
    u_int8_t *GetData()   {return m_pData;}
    int16_t  GetDataLen() {return m_DataLen;}
    //SRawPacket *GetRawPacket() {return m_pPacketData;}
    struct timeval *GetPacketTime() {return &(_time);}
    int16_t *GetEthIndex(){return m_pEthIndex;}

    char * GetPkt(){return m_pPkt;};
    u_int16_t GetPktLen(){return m_uPktLen;};


private:
    //SRawPacket *m_pPacketData;
    struct ether_header *m_pEthhdr;
    struct iphdr *m_pIphdr;
    struct tcphdr *m_pTcphdr;
    u_int8_t * m_pData;
    int16_t    m_DataLen;

    int16_t *m_pEthIndex;
    u_int16_t m_uPktLen;
    char * m_pPkt;
    struct timeval _time;
};
#endif
