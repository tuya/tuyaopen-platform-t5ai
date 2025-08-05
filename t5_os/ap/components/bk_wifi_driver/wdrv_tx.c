#include "wdrv_tx.h"
#include "wdrv_ipc.h"
#include "wdrv_main.h"
#include "wdrv_cntrl.h"

void __asm_flush_dcache_range(void* begin, void* end);

int wdrv_txdata_sender(struct pbuf *p, uint32_t vif_idx)
{
	bk_err_t ret;
	struct wdrv_msg msg;
	struct cpdu_t * cpdu = (struct cpdu_t *)(p + 1);
	
//    if(((void*)cpdu< (void*)&__wifi_start) || ((void*)cpdu> (void*)&__wifi_end))
//    {
//        BK_ASSERT(0);
//    }
    WDRV_LOGV("%s p:%x next:%x payload%x sizeof:%d\r\n",__func__, p, p->next, p->payload, sizeof(struct pbuf));
	msg.type = WDRV_TASK_MSG_TXDATA;
	msg.arg = (uint32_t)cpdu;
	msg.len = vif_idx;
	msg.retry_flag = 0;

	cpdu->co_hdr.vif_idx = vif_idx;
	cpdu->co_hdr.type = TX_MSDU_DATA;
	cpdu->next = NULL;
#if CONFIG_CONTROLLER_DEBUG
    if(!cpdu->co_hdr.need_free)
        TRACK_PBUF_ALLOC(p);
#endif
    if(!cpdu->co_hdr.need_free)
    {
        WDRV_STATS_INC(wdrv_tx_cnt,1);
        WDRV_STATS_INC(tx_alloc_num,1);
    }
    else
    {
        WDRV_STATS_INC(wdrv_rxc_cnt,1);
    }
	pbuf_ref(p);
	ret = rtos_push_to_queue(&wdrv_env.io_queue, &msg, 1 * SECONDS);
	if (kNoErr != ret) {
		WDRV_LOGE("%s failed, ret=%d\r\n",__func__, ret);
        WDRV_STATS_INC(wdrv_tx_snder_fail,1);
		pbuf_free(p);
	}

	return ret;
}

bk_err_t wdrv_txbuf_push(uint8_t channel,void* head,void* tail,uint8_t num)
{
    //struct cpdu_t* buf = (struct cpdu_t*)head;
    ipc_chnl_node_t ipc_node = {0};
    bk_err_t ret = BK_OK;
    BK_ASSERT(head);

    // if((head< (void*)&__wifi_start) || (head> (void*)&__wifi_end))
    // {
    //     BK_ASSERT(0);
    // }
    // if((tail< (void*)&__wifi_start) || (tail> (void*)&__wifi_end))
    // {
    //     BK_ASSERT(0);
    // }
   // __asm_flush_dcache_range(head-64,head+2048);

    //bk_mem_dump("push",PTR_TO_U32(head),20);
    WDRV_LOGV("%s, channel=%d,head=0x%x,tail=0x%x,num:%d\n",__func__, channel,head,tail,num);

    switch(channel)
    {
        case TX_BK_CMD_DATA:
        {
            ipc_node.channel = RX_BK_CMD_DATA;
            ipc_node.head = PTR_TO_U32(head);
            ipc_node.tail = PTR_TO_U32(tail);
            ipc_node.num = num;

            ret = wdrv_ipc_env[IPC_CMD].send(WIFI_IPC_CMD_CHNL,(mb_chnl_cmd_t*)&ipc_node);

            break;
        }
        case TX_MSDU_DATA:
        {
            ipc_node.channel = TX_MSDU_DATA;
            ipc_node.head = PTR_TO_U32(head);
            ipc_node.tail = PTR_TO_U32(tail);
            ipc_node.num = num;
            
            WDRV_LOGV("%s,%d,ipc_node head=0x%x,ipc_node channel=0x%x \n",__func__,__LINE__,ipc_node.head,*(&ipc_node.tail + 1));
            
            //bk_mem_dump("A_TX",PTR_TO_U32((struct pbuf*)head -1),100);
            ret = wdrv_ipc_env[IPC_DATA].send(WIFI_IPC_DATA_CHNL,(mb_chnl_cmd_t*)&ipc_node);

            if(ret == BK_OK)
            {
                wdrv_stats_ptr->ipc_tx_cnt++;
            }else{
                WDRV_LOGE("%s,%d,error type:%d\n",__func__,__LINE__,ret);
                wdrv_stats_ptr->ipc_tx_fail_cnt++;
            }
//            if(ret != BK_OK)
//            {
//                WDRV_LOGE("%s,%d,error type:%d\n",__func__,__LINE__,ret);
//                pbuf_free((void*)((struct cpdu_t*)head - 1));
//                break;
//            }
            break;
        }
        default:
        {
            WDRV_LOGE("%s,error data type\n",__func__);
            BK_ASSERT(0);
        }
    }
    return ret;
}

void wdrv_txdata_pre_process(uint8_t channel, void* head,uint8_t need_retry)
{
    bk_err_t ret = BK_OK;
    uint32_t int_level;
    
    
    void* first = head;
    void* last = head;
    uint8_t num = 1;
    
    uint8_t ipc_chnl = wdrv_map_to_ipc_chnl(channel);

    if(!need_retry)
    {
        //BK_LOGD(NULL, "%s,%d,p:0x%x\n",__func__,__LINE__,(struct pbuf*)head-1);
        //add to tx pending list tail

        if(channel == TX_MSDU_DATA)
        {
            WDRV_STATS_INC(wdrv_tx_process_cnt,1);

            WDRV_STATS_INC(tx_list_num,1);
        }
        //bk_mem_dump("process",PTR_TO_U32(head),50);

        WDRV_IRQ_DISABLE(int_level);
        co_list_push_back((struct co_list *)&wdrv_ipc_env[ipc_chnl].tx_list,(struct co_list_hdr *)head);
        WDRV_IRQ_ENABLE(int_level);
    }

//    ret = rtos_get_semaphore(&wdrv_ipc_env[ipc_chnl].sema, 0);
//    if(ret == BK_OK) {
//        
//        //BK_LOGD(NULL, "%s,%d,get_sema\n",__func__,__LINE__);
//    }
//    if (ret != BK_OK){
//        return;
//    }

    if(wdrv_ipc_env[ipc_chnl].sending_flag) 
    {
        return;
    }
    else
    {
        wdrv_ipc_env[ipc_chnl].sending_flag =1;
    }


    first = (void*)wdrv_ipc_env[ipc_chnl].tx_list.first;
    last = (void*)wdrv_ipc_env[ipc_chnl].tx_list.last;
    num = co_list_cnt((void*)&wdrv_ipc_env[ipc_chnl].tx_list);

    if(((first != NULL)&&(last!= NULL))&&(num == 0)) BK_ASSERT(0);

    if(first == NULL) goto ERR_EXIT;


//    if((first< (void*)&__wifi_start) || (first> (void*)&__wifi_end))
//    {
//        BK_ASSERT(0);
//    }

    WDRV_LOGV("%s,%d,p:0x%x,p:0x%x,num:%d\n",__func__,__LINE__,(struct pbuf*)first-1,(struct pbuf*)last-1,num);

    ret = wdrv_txbuf_push(channel,first,last,num);
    
    if(ret != BK_OK){
        WDRV_LOGE("%s,ipc send fail,0x%x\n",__func__,ret);
    }
    else{
        co_list_init((void*)&wdrv_ipc_env[ipc_chnl].tx_list);
        if(channel == TX_MSDU_DATA)
            WDRV_STATS_RESET(tx_list_num,0);
        return;
    }
ERR_EXIT:
    //ret = rtos_set_semaphore(&wdrv_ipc_env[ipc_chnl].sema);
    
    wdrv_ipc_env[ipc_chnl].sending_flag = 0;
    if(ret != BK_OK) {
        BK_LOGD(NULL, "%s,%d,set_sema fail\n",__func__,__LINE__);
    }
}

void wdrv_tx_complete(void *param, mb_chnl_ack_t *ack_buf)
{
    //bk_err_t ret =BK_OK;
    //ret = 
    //rtos_set_semaphore(&wdrv_ipc_env[IPC_DATA].sema);
    wdrv_ipc_env[IPC_DATA].sending_flag = 0;
    wdrv_stats_ptr->ipc_txc_cnt++;
    //BK_LOGD(NULL, "%s,%d,set_sema:%d\n",__func__,__LINE__,ret);
    if(wdrv_ipc_env[IPC_DATA].tx_list.first != NULL)
    {
        wdrv_msg_sender(0,WDRV_TASK_MSG_TXDATA,1);
    }
}
void wdrv_tx_msg_complete(void *param, mb_chnl_ack_t *ack_buf)
{
    //bk_err_t ret =BK_OK;
    //ret = 
    //rtos_set_semaphore(&wdrv_ipc_env[IPC_DATA].sema);
    
    wdrv_ipc_env[IPC_CMD].sending_flag = 0;
    //BK_LOGD(NULL, "%s,%d,set_sema:%d\n",__func__,__LINE__,ret);
    
    if(wdrv_ipc_env[IPC_CMD].tx_list.first != NULL)
    {
        wdrv_msg_sender(0,WDRV_TASK_MSG_CMD,1);
    }
}

int wdrv_tx_msg_send(uint8_t *msg, uint16_t msg_len,uint8_t waitcfm)
{
    bk_err_t ret = BK_OK;
    struct cpdu_t * cpdu = (struct cpdu_t *)wdrv_get_cmd_buffer(CMD_BUF);

    if(cpdu == NULL) return BK_FAIL;

    WDRV_LOGD("%s msg:%x len:%d cfm:%d\r\n",__func__, msg, msg_len, waitcfm);
    memcpy(cpdu+1, msg,msg_len);
    
    cpdu->co_hdr.type = TX_BK_CMD_DATA;
    cpdu->co_hdr.length = msg_len + sizeof(struct cpdu_t);
    cpdu->co_hdr.is_buf_bank = 0;
    
    //bk_mem_dump("msg",PTR_TO_U32(msg),50);
    //bk_mem_dump("process",PTR_TO_U32(cpdu),50);

    //wdrv_txbuf_push(TX_BK_CMD_DATA,(void*)cpdu,(void*)cpdu,1);
    ret = wdrv_msg_sender((uint32_t)cpdu,WDRV_TASK_MSG_CMD, 0);

    if(ret != BK_OK)
    {
        WDRV_LOGE("%s,%d,push msg fail type:%d\n",__func__,__LINE__,ret);
        
        wdrv_free_cmd_buffer((uint8_t*)cpdu);
    }

    return BK_OK;
}

int wdrv_tx_msg(uint8_t *msg, uint16_t msg_len, wdrv_cmd_cfm *cfm, uint8_t *result)
{
    int ret = 0;
    uint32_t int_level = 0;
    wdrv_cmd_hdr *hdr = NULL;

    WDRV_LOGD("%s msg:%x len:%d\r\n",__func__, msg, msg_len);
    BK_ASSERT(msg_len < MAX_CMD_BUF_PAYLOAD);
    if (!msg) {
        WDRV_LOGE("%s: warning msg is null.\n", __func__);
        ret = -2;
        return ret;
    }
    hdr = (wdrv_cmd_hdr *)msg;

    WDRV_LOGD("%s: msg_id:0x%x len:%d sn:%d waitcfm:%d cfm_id:%x cfm_sn:%d \n", __func__, 
            hdr->cmd_id, msg_len, hdr->cmd_sn, cfm->waitcfm, cfm->cfm_id, cfm->cfm_sn);

    if (cfm->waitcfm == WDRV_CMD_WAITCFM) {

        ret = rtos_init_semaphore(&cfm->sema, 1);
        if(ret == BK_OK) 
        {

            WDRV_ENTER_TXMSG_CRITICAL(int_level);
            co_list_push_back((struct co_list *)&wdrv_host_env.cfm_pending_list,(struct co_list_hdr *)&cfm->list);
            WDRV_EXIT_TXMSG_CRITICAL(int_level);

            cfm->cfm_buf = (uint8_t *)result;
            cfm->cfm_id  = hdr->cmd_id + WDRV_CMD_CFM_OFFSET;
            cfm->waitcfm = WDRV_CMD_WAITCFM;
            cfm->cfm_sn  = hdr->cmd_sn;
            wdrv_tx_msg_send(msg, msg_len, WDRV_CMD_WAITCFM);

            // The len of result-buff is PRIVATE_COMMAND_DEF_LEN.
            if ((rtos_get_semaphore(&cfm->sema, WDRV_CMDCFM_TIMEOUT)) != 0) {

                WDRV_ENTER_TXMSG_CRITICAL(int_level);
                co_list_extract((struct co_list *)&wdrv_host_env.cfm_pending_list,(struct co_list_hdr *)&cfm->list);
                WDRV_EXIT_TXMSG_CRITICAL(int_level);

                //Print AP/CP debug statistics
                wdrv_print_debug_info();
                wdrv_cntrl_get_cif_stats();

                WDRV_LOGW("%s: cmd confirm timeout.\n", __func__);
                ret = -3;
            } else {
                // receive cmd-cfm result
                ret = cfm->cfm_len;
            }

            rtos_deinit_semaphore(&cfm->sema);
        }
        else
        {
            BK_LOGD(NULL, "%s,%d,sema_init fail,send msg fail\n",__func__,__LINE__);
        }
    } else if (cfm->waitcfm == WDRV_CMD_NOWAITCFM) {
        // cmd send direct.
        wdrv_tx_msg_send(msg, msg_len, WDRV_CMD_NOWAITCFM);
    } else {
        WDRV_LOGE("waitcfm param err\n");
        ret = -1;
    }

    return ret;
}

