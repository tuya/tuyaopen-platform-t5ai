#include "cif_wifi_dp.h"

#include "lwip/prot/ethernet.h"
#include "lwip/prot/ip4.h"
#include "lwip/prot/ip.h"
#include "lwip/prot/udp.h"
#include "lwip/prot/tcp.h"
#include "lwip/prot/icmp.h"
#include "lwip/ping.h"

#include "../../dhcpd/dhcp-bootp.h"
#include "cif_ipc.h"
#include "cif_co_list.h"
extern int bmsg_tx_sender(struct pbuf *p, uint32_t vif_idx);
extern void stack_mem_dump(uint32_t stack_top, uint32_t stack_bottom);
extern uint8_t vif_mgmt_get_sta_vif_index();
extern uint8_t vif_mgmt_get_softap_vif_index();
extern bk_err_t dma_memcpy(void *out, const void *in, uint32_t len);
extern uint8_t get_ping_state();
extern uint8_t iperf_get_state();
static uint8_t cif_vif_id_route()
{
    uint8_t sta_vif_id = vif_mgmt_get_sta_vif_index();
    uint8_t sap_vif_id = vif_mgmt_get_softap_vif_index();
    uint8_t ap_id = INVALID_STA_IDX;
    void *vif = NULL;

    if ((sta_vif_id != INVALID_VIF_IDX) && (sap_vif_id != INVALID_VIF_IDX))
    {
        vif = mac_vif_mgmt_get_entry(sta_vif_id);
        if (vif)
            ap_id = mac_vif_mgmt_get_u_sta_ap_id(vif);

        if (ap_id != INVALID_STA_IDX)
        {
            return sta_vif_id + 0xF;
        }
        else
        {
            return sap_vif_id + 0xF;
        }
    }
    else if ((sta_vif_id != INVALID_VIF_IDX) && (sap_vif_id == INVALID_VIF_IDX))
    {
        return sta_vif_id + 0xF;
    }
    else if ((sta_vif_id == INVALID_VIF_IDX) && (sap_vif_id != INVALID_VIF_IDX))
    {
        return sap_vif_id + 0xF;
    }

    return INVALID_VIF_IDX;
}
bk_err_t cif_handle_txdata(void *head)
{
    uint8_t ret = BK_OK;
    struct pbuf* pbuf = NULL;
    cpdu_t* cpdu = (cpdu_t*)head;
    uint8_t vif_id = cpdu->co_hdr.vif_idx + 0xF;//cif_vif_id_route();

    //struct tx_desc_tag * tx_desc = NULL;
    pbuf = (struct pbuf*)((uint8_t*)head - sizeof(struct pbuf));
#if CONFIG_CONTROLLER_RX_DIRECT_PSH
    if(cpdu->co_hdr.need_free)
    {
        CIF_LOGV("%s free p:%x,p->ref:%d\r\n",__func__, pbuf,pbuf->ref);
        pbuf->ref--;
        pbuf_free(pbuf);
        return BK_OK;
    }
#endif
    CIF_STATS_INC(buf_in_txdata);

    //CTRL_IF_DATA("%s,1 length:%d\n",__func__,hdr->co_hdr.length);
    //tx_desc = (struct tx_desc_tag *)(hdr + 1);
    //stack_mem_dump((uint32_t)head,(uint32_t)head + hdr->co_hdr.length);
//    pbuf_header_force(pbuf, -(s16_t)sizeof(struct cpdu_t));
//    pbuf->len = hdr->co_hdr.length - sizeof(struct common_header);
//    pbuf->tot_len = pbuf->len;
    //stack_mem_dump((uint32_t)pbuf->payload,(uint32_t)pbuf->payload + pbuf->len);
    //CTRL_IF_DATA("%s,2 length:%d\n",__func__,hdr->co_hdr.length);
    //Index offset 0xf is used to distinguish data from the controller interface.
    CIF_LOGV("%s p:%x next:%x payload%x sizeof:%d\r\n",__func__, pbuf, pbuf->next, pbuf->payload, sizeof(struct pbuf));
    CIF_LOGV("%s p:%x,vif_id=%d\r\n",__func__, pbuf,vif_id);
#if CONFIG_CONTROLLER_DEBUG
    TRACK_PBUF_ALLOC(pbuf);
#endif
    ret = bmsg_tx_sender(pbuf, vif_id);

    if(ret != BK_OK)
    {
        cif_free_ap_txbuf(pbuf);
        ret = false;
    }

//    int_level = rtos_disable_int();
//    cif_stats_ptr->buf_in_ctrlif_data--;
//    
//    if(cif_stats_ptr->buf_in_ctrlif_data <= 2)
//    {
//        cif_traffic_predictor_upd(BUFFER_TX);
//    }
//
//    rtos_enable_int(int_level);
//    BK_ASSERT(cif_stats_ptr->buf_in_ctrlif_data >= 0);

//    pbuf_free(pbuf);

    return ret;
}

void cif_filter_add_customer_filter(uint32_t ip, uint16_t port)
{
    cif_env.filter.src_ip = ip;
    cif_env.filter.src_port = port;
}
static bool cif_filter_check_customer_filter(struct ip_hdr *iphdr, uint32_t src_port, uint32_t dst_port)
{
    if (cif_env.filter.src_ip == 0)
        return false;
    
    if (cif_env.filter.src_port == 0)
        return false;

    if ((iphdr->src.addr == cif_env.filter.src_ip) && (src_port == cif_env.filter.src_port))
    {
        return true;
    }

    return false;
}
static bool cif_filter_check_bk_filter(uint32_t src_port, uint32_t dst_port)
{
    if ((dst_port == DHCP_SERVER_PORT) || (dst_port == DHCP_CLIENT_PORT) || (dst_port == NAMESERVER_PORT))
    {
        return true;
    }

    if((dst_port >= LOCAL_PORT_RANGE_START) && (dst_port <= LOCAL_PORT_RANGE_END))
    {
        return true;
    }

    return false;
}
static bool cif_filter_check_ip_and_port(struct ip_hdr *iphdr, uint32_t src_port, uint32_t dst_port)
{
    if(iperf_get_state() != 0)
    {
        return true;
    }

    if (cif_filter_check_customer_filter(iphdr, src_port, dst_port))
    {
        return true;
    }

    if (cif_filter_check_bk_filter(src_port, dst_port))
    {
        return true;
    }

    return false;
}
bool cif_filter_check_ip_data(struct pbuf *p)
{
    bool upload2ctrl = false;
    u16_t iphdr_hlen;
//	u16_t iphdr_len;
    uint32_t dest_port=0;
    uint32_t src_port=0;

    struct ip_hdr *iphdr;
    struct udp_hdr *udphdr;
    struct tcp_hdr *tcphdr;
    struct icmp_echo_hdr *iecho;
    if ((p->len <= SIZEOF_ETH_HDR) )//|| pbuf_header(p, (s16_t)-SIZEOF_ETH_HDR)) 
    {
        BK_ASSERT(0);
        return 0;
    }
    //stack_mem_dump((uint32_t)p->payload,(uint32_t)p->payload + 300);
    iphdr = (struct ip_hdr *)(p->payload + SIZEOF_ETH_HDR);
    
    /* obtain IP header length in number of 32-bit words */
    iphdr_hlen = IPH_HL(iphdr);
    /* calculate IP header length in bytes */
    iphdr_hlen *= 4;
  
    /* obtain ip length in bytes */
//	iphdr_len = lwip_ntohs(IPH_LEN(iphdr));
  
    CIF_LOGV("iphdr_hlen=%d,p->len=%d,p->tot_len=%d,IP_HLEN=%d\n",iphdr_hlen,p->len,p->tot_len,IP_HLEN);
    CIF_LOGV("IP RX dest_port = 0x%x\n",dest_port);
//   /* header length exceeds first pbuf length, or ip length exceeds total pbuf length? */
//   if ((iphdr_hlen > p->len) || (iphdr_len > p->tot_len) || (iphdr_hlen < IP_HLEN)) 
//   {
// 	BK_ASSERT(0);
// 	 return 0;
//   }

  switch (IPH_PROTO(iphdr)) 
  {
     case IP_PROTO_UDP:
         udphdr = (struct udp_hdr *)(p->payload+(s16_t)iphdr_hlen+SIZEOF_ETH_HDR);
         dest_port=lwip_ntohs(udphdr->dest);
         src_port=lwip_ntohs(udphdr->src);
         CIF_LOGV("IP_PROTO_UDP dest_port:%d,src:%d\r\n",dest_port,src_port);
         upload2ctrl = cif_filter_check_ip_and_port(iphdr, src_port, dest_port);
         break;

     case IP_PROTO_TCP:
         tcphdr = (struct tcp_hdr *)(p->payload+(s16_t)iphdr_hlen+SIZEOF_ETH_HDR);
         dest_port=lwip_ntohs(tcphdr->dest);
         src_port=lwip_ntohs(tcphdr->src);
         CIF_LOGV("IP_PROTO_TCP port:%d\r\n",dest_port);
         upload2ctrl = cif_filter_check_ip_and_port(iphdr, src_port, dest_port);
         //BK_LOGD(NULL,"RX TCP src_ip:%x, src_port:%d\n", iphdr->src.addr, src_port);
         break;

    case IP_PROTO_ICMP:
        iecho = (struct icmp_echo_hdr *)((p->payload+(s16_t)iphdr_hlen+SIZEOF_ETH_HDR));
        if (iecho->type == ICMP_ECHO)
        {
            upload2ctrl = true;
        }
        else if (2 == get_ping_state()) //PING_STATE_STARTED
        {
            upload2ctrl = true;
        }
        else
        {
            upload2ctrl = false;
        }
        CIF_LOGV("IP_PROTO_ICMP,%p\r\n",p->payload);
        break;

    case IP_PROTO_IGMP:
        upload2ctrl = false;
        CIF_LOGV("IP_PROTO_IGMP\r\n");
        break; 
     default:
         break;
    }
    CIF_LOGV("%s %d dest_port = %d\r\n",__func__,__LINE__,dest_port);

    return upload2ctrl;
}

bool cif_rx_local_packet_check(struct pbuf **p_ptr, struct eth_hdr * ethhdr,void* vif)
{
    bool upload2ctrl = true;
    struct pbuf *p = *p_ptr;
    bk_err_t ret = BK_OK;

    CIF_LOGV("%s p:%x next:0x%x payload:0x%x sizeof:%d\r\n",__func__, p, p->next, p->payload, sizeof(struct pbuf));

    if (cif_env.no_host)
    {
         CIF_LOGV("%s no host connected, upload to controller\r\n",__func__, upload2ctrl);
         return true;
    }

    // if (!cif_env.host_wifi_init)
    // {
    //      CIF_LOGV("%s AP Wi-Fi does not start, upload to controller\r\n",__func__);
    //      return true;
    // }

    switch (htons(ethhdr->type))
    {
        case ETHTYPE_EAPOL:
        {
            CIF_LOGV("ETHTYPE_EAPOL RX\n");
            upload2ctrl = true;
            break;
        }
        case ETHTYPE_ARP:
        {
            struct pbuf* p_copy = NULL;
            CIF_LOGV("ARP RX\n");

#if CONFIG_CONTROLLER_RX_DIRECT_PSH
            p_copy = pbuf_alloc(PBUF_RAW,p->len+sizeof(cpdu_t),PBUF_RAM_RX);
            upload2ctrl = true;
            if(p_copy)
            {
                pbuf_header(p_copy, -(s16)sizeof(struct cpdu_t));
                memcpy(p_copy->payload,p->payload,p->len);
            }
            else
            {
                return upload2ctrl;   
            }
#else
            p_copy = (struct pbuf*)cif_maclloc_rx_buf();

            upload2ctrl = true;

            if (p_copy == NULL)
            {
                CIF_LOGV("%s,%d,alloc fail\n",__func__,__LINE__);
                return upload2ctrl;
            }
            else
            {
                //pbuf_copy(p_copy, p);
                #ifdef CONFIG_CONTROLLER_WAR
                memcpy(p_copy->payload,p->payload,p->len);
                #else
                dma_memcpy(p_copy->payload,p->payload,p->len);
                #endif
                p_copy->len = p->len;
            }
#endif            
            //bk_mem_dump("Meth input p",(uint32_t)p,sizeof(struct pbuf)+8);
            //bk_mem_dump("Meth input payload",(uint32_t)p->payload,30);
            
            CIF_LOGV("%s,%d p:%p next:%p payload:%p len:%d\r\n",
                __func__,__LINE__, p_copy, p_copy->next, p_copy->payload, p_copy->tot_len);

            //pbuf_header_force(p, (s16)macif_get_rxl_payload_offset() + sizeof(struct cpdu_t));

            struct cpdu_t *cpdu = (struct cpdu_t*)(p_copy + 1);
            cpdu->co_hdr.length = p_copy->len - sizeof(struct pbuf);
            cpdu->co_hdr.type = RX_MSDU_DATA;
            cpdu->co_hdr.need_free = 0;
            cpdu->co_hdr.special_type = 0;
            cpdu->co_hdr.vif_idx = wifi_netif_vif_to_netif_type(vif);
            //bk_mem_dump("cif_filter before snder",(uint32_t)p_copy->payload,100);
            ret = cif_msg_sender(cpdu,CIF_TASK_MSG_RX_DATA,0);
            if(ret != BK_OK)
            {
                #if CONFIG_CONTROLLER_RX_DIRECT_PSH
                pbuf_free(p_copy);
                #else
                //If rxbuf push fail, free it immediately
                cif_free_rx_buf((uint32_t)p_copy);
                #endif
            }else
            {
                cif_stats_ptr->total_recv_cnt++;
            }

            break;
        }
        case ETHTYPE_IP:
        {
            if (cif_filter_check_ip_data(p) == false)
            {
                //pbuf_header_force(p, (s16)macif_get_rxl_payload_offset() + sizeof(struct cpdu_t));
                /*
                * +-----  host_id (struct pbuf{} *)
                * |
                * V
                * +--------------+-------------+---------------------+
                * |  common hdr  | fhost hdr   | IEEE 802.3 Data     |
                * +--------------+-------------+---------------------+
                */
                struct pbuf* p_copy = NULL;

#if CONFIG_CONTROLLER_RX_DIRECT_PSH
                p_copy =  p;
                upload2ctrl = false;
#else
                p_copy = (struct pbuf*)cif_maclloc_rx_buf();
                
                if (p_copy == NULL)
                {
                    CIF_LOGV("%s,%d,alloc fail\n",__func__,__LINE__);
                    pbuf_free(p);
                    upload2ctrl = false;
                    return upload2ctrl;
                }

                BK_ASSERT(p_copy->payload);
                #ifdef CONFIG_CONTROLLER_WAR
                memcpy(p_copy->payload,p->payload,p->len);
                #else
                dma_memcpy(p_copy->payload,p->payload,p->len);
                #endif
                p_copy->len = p->len;

                pbuf_free(p);
#endif
                struct cpdu_t *cpdu = (struct cpdu_t*)(p_copy + 1);
                cpdu->co_hdr.length = p_copy->len - sizeof(struct pbuf);
                cpdu->co_hdr.type = RX_MSDU_DATA;
                cpdu->co_hdr.need_free = 0;
                cpdu->co_hdr.special_type = 0;
                cpdu->co_hdr.vif_idx = wifi_netif_vif_to_netif_type(vif);
                CIF_LOGV("%s,%d p:%p next:%p payload:%p len:%d\r\n",
                    __func__,__LINE__, p_copy, p_copy->next, p_copy->payload, p_copy->tot_len);

                ret = cif_msg_sender(cpdu,CIF_TASK_MSG_RX_DATA,0);
                if(ret != BK_OK)
                {
                    #if CONFIG_CONTROLLER_RX_DIRECT_PSH
                    pbuf_free(p_copy);
                    #else
                    //If rxbuf push fail, free it immediately
                    cif_free_rx_buf((uint32_t)p_copy);
                    #endif
                }else
                {
                    cif_stats_ptr->total_recv_cnt++;
                }


                upload2ctrl = false;
            }
            else
            {
                upload2ctrl = true;
            }
            break;
        }
        default:
        {
            upload2ctrl = true;
            break;
        }
    }

    return upload2ctrl;
}
