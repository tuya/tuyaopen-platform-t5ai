#include "wdrv_rx.h"
#include "wdrv_ipc.h"
#include "wdrv_cntrl.h"
#include "wdrv_tx.h"
#include "wdrv_co_list.h"
extern void ethernetif_input(uint8_t vif_index, struct pbuf *p);
void __asm_flush_dcache_range(void* begin, void* end);

void wdrv_rx_confirm_tx_msg(wdrv_rx_msg *msg)
{
    wdrv_cmd_cfm *cmd_cfm = NULL;
    uint32_t int_level = 0;
    
    rtos_lock_mutex(&wdrv_host_env.cfm_lock);

    cmd_cfm = (wdrv_cmd_cfm *)wdrv_host_env.cfm_pending_list.first;

    while(cmd_cfm) {
        // cppcheck-suppress oppositeInnerCondition
//        if (!cmd_cfm)
//            break;
        if ((msg->id == cmd_cfm->cfm_id) && (msg->cfm_sn == cmd_cfm->cfm_sn)) {

            if (cmd_cfm->cfm_buf && msg->param_len) 
                memcpy(cmd_cfm->cfm_buf, msg->param, msg->param_len);

            cmd_cfm->cfm_len = msg->param_len;
            
            WDRV_ENTER_TXMSG_CRITICAL(int_level);
            co_list_extract((struct co_list *)&wdrv_host_env.cfm_pending_list,(struct co_list_hdr *)&cmd_cfm->list);
            WDRV_EXIT_TXMSG_CRITICAL(int_level);

            rtos_set_semaphore(&cmd_cfm->sema);
            break;
        }
        cmd_cfm = (wdrv_cmd_cfm *)cmd_cfm->list.next;
    }
    rtos_unlock_mutex(&wdrv_host_env.cfm_lock);
}

void wdrv_rx_handle_msg(wdrv_rx_msg *msg)
{
    //bk_mem_dump("wdrv_rx_handle_msg",PTR_TO_U32(msg), 30);
    cpdu_t* cpdu = (struct cpdu_t*)msg -1;
    WDRV_LOGD("wdrv_rx_handle_msg id:%x cfm_sn:%d len:%d\r\n", msg->id, msg->cfm_sn, msg->param_len);

    if (msg->id >= WDRV_CMD_CFM_OFFSET) {
        wdrv_rx_handle_cmd_confirm(msg);
    }
    else {
        wdrv_rx_handle_event(msg);
    }

    if(cpdu->co_hdr.special_type == 0) //RX cmd pbuf(filter and monitor) don't free cmd buffer
    {
        wdrv_free_cmd_buffer((uint8_t*)cpdu);
    }
}

void wdrv_rxdata_process(struct pbuf *p)
{
    struct cpdu_t* cpdu = NULL;
    cpdu = (struct cpdu_t*)(p + 1);
#ifdef CONFIG_CONTROLLER_RX_DIRECT_PSH
    struct pbuf* p_copy = NULL;
    bk_err_t ret = BK_OK;
    uint8_t vif_idx = 0;
#endif

    wdrv_attach_rx_buffer();

    if(cpdu->co_hdr.need_free)
    {
        struct pbuf * pbuf = NULL;
        pbuf = (struct pbuf*)(PTR_FROM_U32(uint8_t,cpdu) - sizeof(struct pbuf));
        WDRV_LOGV("%s,%d,data addr receive 0x%x\n",__func__,__LINE__,pbuf);

//        for(uint8_t k=0;k<wdrv_rxbank_debug.rx_buf_bank_cnt ;k++)
//        {
//            struct pbuf * p_temp =NULL;
//            p_temp = PTR_FROM_U32(struct pbuf,wdrv_rxbank_debug.rx_buf_bank[k]);
//            if(p_temp == p) break;
//
//            if(k == (wdrv_rxbank_debug.rx_buf_bank_cnt -1)) BK_ASSERT(0);
//        }
//        --wdrv_rxbank_debug.rx_buf_bank_cnt;
        cpdu->co_hdr.need_free = 0;
#if CONFIG_CONTROLLER_DEBUG
        TRACK_PBUF_FREE(pbuf);
#endif
        pbuf_free(pbuf);
        WDRV_STATS_DEC(tx_alloc_num);
        wdrv_stats_ptr->wdrv_txc_cnt++;
        return;
    }

    if(cpdu->co_hdr.special_type != 0)
    {
        wdrv_msg_sender((uint32_t)(cpdu + 1),WDRV_TASK_MSG_EVENT,0);
        return;
    }

    WDRV_LOGV("%s p:%p next:%p payload:%p sizeof:%d cpdu:%p\r\n",
        __func__, p, p->next, p->payload, sizeof(struct pbuf), cpdu);
    WDRV_STATS_DEC(rx_alloc_num);
    WDRV_LOGV("%s rx_alloc_num = %d\r\n",__func__,wdrv_stats_ptr->rx_alloc_num );
    wdrv_stats_ptr->wdrv_rx_cnt++;
#if CONFIG_CONTROLLER_RX_DIRECT_PSH    
    p_copy = pbuf_alloc(PBUF_RAW,p->len + sizeof(cpdu_t),PBUF_RAM_RX);
    //bk_mem_dump("wdrv_rxdata_process",(uint32_t)p->payload,100);
    if(p_copy)
    {
        pbuf_header(p_copy, -(s16)sizeof(struct cpdu_t));
        memcpy(p_copy->payload,p->payload,p->len);
    }
    else
    {
        wdrv_stats_ptr->wdrv_rx_cpy_fail++;
    }
    cpdu->co_hdr.need_free = 1;//RXC free
    vif_idx = cpdu->co_hdr.vif_idx;
    ret = wdrv_txdata_sender(p,0);//vif null, just for free this RXC pbuf
    if(0 != ret)
    {
        ret = ERR_TIMEOUT;
        WDRV_LOGE("%s, RXC sender error\r\n",__func__);
    }

    if(p_copy)
    {
        WDRV_LOGV("%s, vif%d\r\n",__func__,vif_idx);
        ethernetif_input(vif_idx , p_copy);
    }
#else
    WDRV_LOGV("------%s, vif%d\r\n",__func__,cpdu->co_hdr.vif_idx);
    ethernetif_input(cpdu->co_hdr.vif_idx , p);
#endif
}
//MBOX
uint8_t wdrv_recv_buffer(void *param, uint32_t *payload)
{
    uint8_t num_temp = 0;
    uint8_t ret = BK_OK;
    cpdu_t* temp_next = NULL;
    ipc_chnl_node_t *node = (ipc_chnl_node_t*)payload;
    uint8_t chan_id = node->channel;
    cpdu_t* head = PTR_FROM_U32(cpdu_t,node->head);
    cpdu_t* tail = PTR_FROM_U32(cpdu_t,node->tail);
    uint8_t num = node->num;

    while(head)
    {
        struct cpdu_t * hdr = PTR_FROM_U32(struct cpdu_t,head);
        temp_next = hdr->next;
        WDRV_LOGV("%s,chan_id=%d,head=0x%x,tail=0x%x,num=%d,start!\n",__func__,chan_id,head,tail,num);
        //stack_mem_dump((uint32_t)head,(uint32_t)head+300);

        //__asm_flush_dcache_range(PTR_FROM_U32(void,head)-100,PTR_FROM_U32(void,head)+2048);

        switch(chan_id)
        {
            case RX_BK_CMD_DATA:
            {
                wdrv_msg_sender((uint32_t)(head + 1),WDRV_TASK_MSG_EVENT,0);
                break;
            }
            case RX_MSDU_DATA:
            {
                struct pbuf * p = NULL;
                p = (struct pbuf*)((struct pbuf*)head - 1);
                ret = wdrv_msg_sender((uint32_t)p,WDRV_TASK_MSG_RXDATA,0);
                //bk_mem_dump("wdrv_recv p",PTR_TO_U32(p),sizeof(struct pbuf)+8);qq
                if(ret != BK_OK)
                {
                    pbuf_free(p);
                }
                break;
            }
            default:
                WDRV_LOGE("%s,chan_id=%d,error data type\n",__func__,chan_id);
        };
        num_temp++;
        head = temp_next;
        if((num_temp == num)||(head == 0)) break;
    }
    return ret;
}
