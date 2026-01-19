#include "cif_main.h"
#include "cif_ipc.h"

struct cif_env_t cif_env = {0};
struct cif_stats * cif_stats_ptr = &(cif_env.stats);

extern void stack_mem_dump(uint32_t stack_top, uint32_t stack_bottom);
extern bk_err_t cif_free_rxdata(struct common_header* co_hdr);

void cif_register_customer_msg_handler(cif_customer_msg_cb_t func)
{
    cif_env.customer_msg_cb = func;
}

void cif_print_debug_info()
{
    CIF_LOGI("cif rx bank cnt:%d\n",cif_rxbank_ptr->rx_buf_bank_cnt);
    CIF_LOGI("cif rx drop cnt:%d\n",cif_stats_ptr->rx_drop_cnt);
    CIF_LOGI("cif rx data cnt:%d\n",cif_stats_ptr->cif_rx_data);

    CIF_LOGI("cif tx current cnt:%d\n",cif_stats_ptr->buf_in_txdata);
    CIF_LOGI("cif tx total cnt:%d\n",cif_stats_ptr->cif_tx_cnt);
    CIF_LOGI("cif txc total cnt:%d\n",cif_stats_ptr->cif_txc_cnt);
    CIF_LOGI("cif tx and rxc total cnt:%d\n",cif_stats_ptr->cif_tx_dnld_cnt);

    CIF_LOGI("rx total cnt:%d\n",cif_stats_ptr->cif_rx_cnt);
    CIF_LOGI("rxc total cnt:%d\n",cif_stats_ptr->cif_rxc_cnt);

    CIF_LOGI("ipc tx cnt:%d, ipc txc cnt:%d, ipc tx fail cnt:%d \n",
    cif_stats_ptr->ipc_tx_cnt,cif_stats_ptr->ipc_txc_cnt,cif_stats_ptr->ipc_tx_fail_cnt);

    CIF_LOGI("cif msg snder fail:%d\n",cif_stats_ptr->cif_msg_snder_fail);
    CIF_LOGI("cif txbuf leak:%d\n",cif_stats_ptr->cif_tx_buf_leak);

    for(uint8_t j = 0; j<cif_rxbank_ptr->rx_buf_bank_cnt ;j++)
    {
        CIF_LOGI("cnt:%d,addr:0x%x \n",j,cif_rxbank_ptr->rx_buf_bank[j]);
    }

    CIF_LOGI("cif event short buf cnt:%d,long buf cnt:%d\n",cif_get_event_short_buf_cnt(),cif_get_event_long_buf_cnt());

    CIF_LOGI("\n");
}

bk_err_t cif_msg_sender(void* head,enum cif_task_msg_evt type,uint8_t retry)
{
    bk_err_t ret = BK_OK;
    struct cif_msg msg;

    msg.type = type;
    msg.arg = (uint32_t)head;
    CIF_LOGV("%s,%d,head:0x%x\n",__func__,__LINE__,head);
    msg.retry_flag = retry;
    ret = rtos_push_to_queue(&cif_env.io_queue, &msg, BEKEN_NO_WAIT);
    if (BK_OK != ret) {
        CIF_STATS_INC(cif_msg_snder_fail);
        CIF_LOGW("%s failed, ret=%d\r\n",__func__, ret);
    }

    return ret;
}
void cif_main(void *arg)
{
    bk_err_t ret;
    struct cif_msg msg;

    while (1)
    {
        ret = rtos_pop_from_queue(&cif_env.io_queue, &msg, BEKEN_WAIT_FOREVER);
        if (BK_OK != ret)
        {
            continue;
        }
        switch (msg.type)
        {
            case CIF_TASK_MSG_CMD:
            {
                ret = cif_handle_bk_cmd((void*)msg.arg);
                break;
            }
            case CIF_TASK_MSG_DATA:
            {
                ret = cif_handle_txdata((void*)msg.arg);
                break;
            }
            case CIF_TASK_MSG_EVT:
            {
                cif_rxdata_pre_process(RX_BK_CMD_DATA,(void*)msg.arg,msg.retry_flag);
                break;
            }
            case CIF_TASK_MSG_RX_DATA:
            {
                cif_rxdata_pre_process(RX_MSDU_DATA,(void*)msg.arg,msg.retry_flag);
                break;
            }
            default:
                BK_ASSERT(0);
                break;
        }
    }
}

uint8_t cif_dnld_buffer(void *param, void *payload)
{
    uint8_t num_temp = 0;
    uint8_t ret = BK_OK;
    void * temp_next = NULL;
    cif_chnl_node_t *node = (cif_chnl_node_t*)payload;
    uint8_t chan_id = node->channel;
    void * head = node->head;
    void * tail = node->tail;
    uint8_t num = node->num;

    CIF_LOGV("%s,chan_id=%d,head=0x%x,tail=0x%x,num=%d,start!\n",__func__,chan_id,head,tail,num);

    while(head != NULL)
    {
        struct cpdu_t * hdr = (struct cpdu_t*)head;
        temp_next = (void*)hdr->next;
        
        CIF_LOGV("%s,head=0x%x,head1=0x%x,head2=0x%x\n",__func__,chan_id,*((uint32_t*)head),*((uint32_t*)head+1),*((uint32_t*)head + 2));
        //cif_rxbank_check();

        switch(chan_id)
        {
            case TX_BK_CMD_DATA:
            {
                cif_msg_sender(head,CIF_TASK_MSG_CMD,0);
                break;
            }
            case TX_MSDU_DATA:
            {
                cif_stats_ptr->cif_tx_dnld_cnt += 1;
                //ret = cif_handle_txdata(head);
                cif_msg_sender(head,CIF_TASK_MSG_DATA,0);
                break;
            }
            default:
                CIF_LOGE("%s,chan_id=%d,error data type\n",__func__,chan_id);
        };
        num_temp++;
        head = temp_next;
        if((num_temp == num)||(head == NULL)) break;
    }
    return ret;
}


bk_err_t cif_rxbuf_push(uint8_t channel,void* head,void* tail,uint8_t num)
{
    //struct cpdu_t* buf = (struct cpdu_t*)head;
    cif_chnl_node_t ipc_node = {0};
    bk_err_t ret = BK_OK;

    BK_ASSERT(head);
    cif_rxbank_check();
    //stack_mem_dump((uint32_t)buf,(uint32_t)buf+3);
    CIF_LOGV("%s, channel=%d,head:0x%x,tail:0x%x,num:%d\n",__func__, channel,head,tail,num);

#if 0
    if (!cif_env.host_connected)
    {
        CIF_LOGD("Host does NOT connected, SKIP rxbuf_push\n");
        return false;
    }

    if (!cif_env.host_powerup)
    {
        CIF_LOGD("Host does NOT power up, SKIP rxbuf_push\n");
        return false;
    }

    if (cif_env.cif_sleeping)
    {
        CIF_LOGD("SDIO Sleeping, SKIP rxbuf_push\n");
        return false;
    }
    //memset(buf,0,sizeof(struct cpdu_t));
#endif
    switch(channel)
    {
        case RX_BK_CMD_DATA:
        {
            ipc_node.channel = RX_BK_CMD_DATA;
            ipc_node.head = head;
            ipc_node.tail = tail;
            ipc_node.num = num;

            ret = cif_ipc_env[IPC_CMD].send(WIFI_IPC_CMD_CHNL,(mb_chnl_cmd_t*)&ipc_node);

            break;
        }
        case RX_MSDU_DATA:
        {
            //struct cpdu_t * buf  =(struct cpdu_t *)(head);
            ipc_node.channel = RX_MSDU_DATA;
            ipc_node.head = head;
            ipc_node.tail = tail;
            ipc_node.num = num;

            //struct pbuf* p = (struct pbuf*)head - 1;
            //bk_mem_dump("Meth2 input p",(uint32_t)p,sizeof(struct pbuf)+8);
            //bk_mem_dump("Meth2 input payload",(uint32_t)p->payload,30);

            ret = cif_ipc_env[IPC_DATA].send(WIFI_IPC_DATA_CHNL,(mb_chnl_cmd_t*)&ipc_node);
            if(ret == BK_OK)
            {
                cif_stats_ptr->ipc_tx_cnt++;
            }else{
                CIF_LOGE("%s,%d,error type:%d\n",__func__,__LINE__,ret);
                cif_stats_ptr->ipc_tx_fail_cnt++;
            }
            break;
        }
        default:
        {
            CIF_LOGE("%s,error data type\n",__func__);
            BK_ASSERT(0);
        }
    }

    return ret;
}

bk_err_t cif_rxdata_pre_process(uint8_t channel,void* head,uint8_t need_retry)
{
    bk_err_t ret = BK_OK;
    uint32_t int_level;
    uint8_t ipc_chnl = cif_map_to_ipc_chnl(channel);
    
    void* first = head;
    void* last = head;
    uint8_t num = 1;
    
    if(!need_retry)
    {
        BK_LOGV(NULL,"%s,%d,p:0x%x,ipc_chnl:%d\n",__func__,__LINE__,(struct pbuf*)head-1,ipc_chnl);
        //add to tx pending list tail
        CIF_IRQ_DISABLE(int_level);
        co_list_push_back((struct co_list *)&cif_ipc_env[ipc_chnl].rx_list,(struct co_list_hdr *)head);
        CIF_IRQ_ENABLE(int_level);
    }

    if(cif_ipc_env[ipc_chnl].sending_flag) 
    {
        return BK_OK;
    }
    else
    {
        cif_ipc_env[ipc_chnl].sending_flag =1;
    }
    
    first = (void*)cif_ipc_env[ipc_chnl].rx_list.first;
    last = (void*)cif_ipc_env[ipc_chnl].rx_list.last;
    num = co_list_cnt((void*)&cif_ipc_env[ipc_chnl].rx_list);

    if(((first != NULL)&&(last!= NULL))&&(num == 0)) BK_ASSERT(0);

    if(first == NULL) goto ERR_EXIT;

    CIF_LOGV("%s,%d,p:0x%x,p:0x%x,num:%d\n",__func__,__LINE__,(struct pbuf*)first-1,(struct pbuf*)last-1,num);

    ret = cif_rxbuf_push(channel,first,last,num);
    
    if(ret != BK_OK){
        CIF_LOGE("%s,ipc send fail,0x%x\n",__func__,ret);
    }
    else{
        co_list_init((void*)&(cif_ipc_env[ipc_chnl].rx_list));
        return BK_OK;
    }
ERR_EXIT:
    cif_ipc_env[ipc_chnl].sending_flag = 0;
    return BK_OK;
}

void cif_rx_data_complete(void *param, void *ack_buf)
{
    cif_ipc_env[IPC_DATA].sending_flag = 0;
    cif_stats_ptr->ipc_txc_cnt++;
    //BK_LOGD(NULL,"%s,%d\n",__func__,__LINE__);
    if(cif_ipc_env[IPC_DATA].rx_list.first != NULL)
    {
        cif_msg_sender(NULL,CIF_TASK_MSG_RX_DATA,1);
    }
}
void cif_rx_evt_complete(void *param, void *ack_buf)
{
    cif_ipc_env[IPC_CMD].sending_flag = 0;
    
    //BK_LOGD(NULL,"%s,%d\n",__func__,__LINE__);
    if(cif_ipc_env[IPC_CMD].rx_list.first != NULL)
    {
        cif_msg_sender(NULL,CIF_TASK_MSG_EVT,1);
    }
}

bk_err_t cif_deinit()
{
    CIF_LOGE("cif_deinit\n");
    if (cif_env.handle) {
        rtos_delete_thread(&cif_env.handle);
        cif_env.handle = NULL;
    }

    if (cif_env.io_queue) {
        rtos_deinit_queue(&cif_env.io_queue);
        cif_env.io_queue = NULL;
    }

    if (cif_env.enter_lv_timer.handle) {
        rtos_deinit_oneshot_timer(&cif_env.enter_lv_timer);
        cif_env.enter_lv_timer.handle = NULL;
    }

    return BK_OK;
}

bk_err_t cif_init()
{
    bk_err_t ret = BK_OK;

    //IPC interface init
    cif_ipc_init();
    //Init event buffer(for CP->AP use)
    cif_tx_event_buffer_init();

    ret = rtos_init_oneshot_timer(&(cif_env.enter_lv_timer), CNTRL_IF_ENTER_LV_DELAY_TIME_MS,
                    cif_enter_lv_timer_cb, 0, 0);

    if (ret != BK_OK)
    {
        CIF_LOGE("cif_init init oneshot timer failed:%d\n", ret);
        goto cif_init_failed;
    }

    ret = rtos_init_queue(&cif_env.io_queue, "cif_queue", sizeof(struct cif_msg), CNTRL_IF_QUEUE_LEN);
    if (ret != BK_OK)
    {
        CIF_LOGE("cif_init init queue failed:%d\n", ret);
        goto cif_init_failed;
    }

    ret = rtos_create_thread(&cif_env.handle,
                                CNTRL_IF_TASK_PRIO,
                                "cif_thread",
                                (beken_thread_function_t)cif_main,
                                4096,
                                (beken_thread_arg_t)0);
    if (ret != BK_OK)
    {
        CIF_LOGE("cif_init create task failed:%d\n", ret);
        goto cif_init_failed;
    }

    CIF_LOGD("cif_init\n");
    cif_env.cif_sleeping = false;
    cif_env.host_powerup = false;
    cif_env.no_host = true;
    return ret;
cif_init_failed:
    cif_deinit();
    return ret;
}

