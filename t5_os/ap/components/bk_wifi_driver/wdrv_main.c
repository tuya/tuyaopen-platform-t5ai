#include "wdrv_main.h"
#include "wdrv_cntrl.h"
#include "net.h"
#include "lwip/err.h"
#include "lwip/netif.h"
#include "wdrv_ipc.h"
#include "wdrv_tx.h"
#include "wdrv_rx.h"
//#include "bk_err.h"
#include <components/netif.h>
#include <components/event.h>
void __asm_flush_dcache_range(void* begin, void* end);

struct wdrv_env_t wdrv_env = {0};
struct wdrv_stats * wdrv_stats_ptr = &wdrv_env.stat;
__attribute__((section(".wifi_data"))) uint8_t wdrv_cmd_buffer[MAX_NUM_CMD_BUFFERS][MAX_CMD_BUF_LENGTH] = {0};
__attribute__((section(".wifi_data"))) uint8_t wdrv_cmd_bank[MAX_NUM_CMD_RX_BANK][MAX_CMD_BANK_LENGTH] = {0};

//struct wdrv_rx_bank_debug_t wdrv_rxbank_debug = {0};

bk_err_t wdrv_txbuf_push(uint8_t channel,void* head,void* tail,uint8_t num);
bk_err_t wdrv_deinit();

void wdrv_print_debug_info()
{
    WDRV_LOGD("wdrv rx_alloc cnt:%d,rx win:%d,tx cnt:%d,process:%d,eth_num:%d,tx_free_total:%d\n rx recv:%d, ipc tx cnt:%d, ipc txc cnt:%d, ipc tx fail cnt:%d \n", 
    wdrv_stats_ptr->rx_alloc_num,wdrv_stats_ptr->rx_win,wdrv_stats_ptr->tx_alloc_num,
    wdrv_stats_ptr->tx_process_num,wdrv_stats_ptr->tx_eth_num,wdrv_stats_ptr->tx_free_total,
    wdrv_stats_ptr->rx_total_recv,
    wdrv_stats_ptr->ipc_tx_cnt,wdrv_stats_ptr->ipc_txc_cnt,wdrv_stats_ptr->ipc_tx_fail_cnt);

    WDRV_LOGD("wdrv tx_list_num:%d,first:0x%x,last:0x%x\n", 
        wdrv_stats_ptr->tx_list_num,wdrv_ipc_env[IPC_DATA].tx_list.first,wdrv_ipc_env[IPC_DATA].tx_list.last);
}
bk_err_t wdrv_msg_sender(uint32_t head,enum wdrv_task_msg_evt type,uint8_t retry)
{
    bk_err_t ret = BK_OK;
    struct wdrv_msg msg;

    msg.type = type;
    msg.arg = (uint32_t)head;
    WDRV_LOGV("%s,%d,head:0x%x\n",__func__,__LINE__,head);
    msg.retry_flag = retry;
    ret = rtos_push_to_queue(&wdrv_env.io_queue, &msg, BEKEN_NO_WAIT);
    if (BK_OK != ret) {
        WDRV_LOGW("%s failed, ret=%d\r\n",__func__, ret);
    }

    return ret;
}


void wdrv_send_buffer_bank(uint16_t num)
{
    ipc_addr_bank_t * bank = NULL;
    uint32_t temp_addr = 0;
    uint8_t i = 0;
    struct pbuf * p = NULL;
    bk_err_t ret = BK_OK;

    if(num <= 1) return;

    bank = (ipc_addr_bank_t *)wdrv_get_cmd_buffer(CMD_BANK);

    if(bank == NULL) return;

    for(;i< num;i++)
    {
        p = pbuf_alloc(PBUF_RAW,MAX_TX_DATA_LENGTH,PBUF_RAM_RX);

        if(p == NULL) 
        {
            if(i == 0)
            {
                wdrv_free_cmd_buffer((uint8_t*)bank);
                return;
            }
            else
            {
                break;
            }
        }
        //__asm_flush_dcache_range(p-40,p+2048);
//        if(((void*)p< (void*)&__wifi_start) || ((void*)p> (void*)&__wifi_end))
//        {
//            BK_ASSERT(0);
//        }

        BK_ASSERT(p->payload);
        BK_ASSERT(p->tot_len < 1600);
        
        memset((void*)p->payload,0,p->len);

        pbuf_header(p, -(s16)sizeof(struct cpdu_t));

        temp_addr = PTR_TO_U32(p);

        WDRV_LOGV("%s,%d i:%d p:%p next:%p payload:%p len:%d\r\n",
            __func__,__LINE__, i, p, p->next, p->payload, p->tot_len);
        
        //bk_mem_dump("Abank p",(uint32_t)p,sizeof(struct pbuf)+8);
        if(temp_addr)
        {
            bank->addr[i] = temp_addr;
        }
        else
        {
            break;
        }
    }
    BK_ASSERT(i<=MAX_NUM_RX_BUFFERS);
    bank->dir = BUFFER_TX;
    bank->mem_status = BUFFER_STATUS_FREE_BANK;
    bank->num = i;
    bank->co_hdr.is_buf_bank = 1;
    //bk_mem_dump("wdrv_rx_bank",PTR_TO_U32(bank+1),i*4 );
    //bk_mem_dump("wdrv_rx_bank",PTR_TO_U32(bank),(i+3)*4 );
    
    WDRV_LOGV("%s attach num:%d,rx_alloc_num = %d\r\n",__func__,i,wdrv_stats_ptr->rx_alloc_num );
    
    ret = wdrv_msg_sender((uint32_t)bank,WDRV_TASK_MSG_CMD, 0);
    //ret = wdrv_txbuf_push(TX_BK_CMD_DATA,bank,bank,1);
    
    if(ret != BK_OK)
    {
        void * temp = NULL;
        WDRV_LOGE("%s,%d,push rx bank fail type:%d\n",__func__,__LINE__,ret);
        
        for(uint8_t j=0 ;j< bank->num;j++)
        {
            temp = PTR_FROM_U32(void,bank->addr[j]);
            pbuf_free(temp);
        }
        wdrv_free_cmd_buffer((uint8_t*)bank);
    }
    else
    {
        WDRV_STATS_INC(rx_alloc_num,i);
//        for(uint8_t k=0;k<bank->num;k++)
//        {
//            wdrv_rxbank_debug.rx_buf_bank[wdrv_rxbank_debug.rx_buf_bank_cnt++] = bank->addr[i];
//        }
    }
}

void wdrv_rx_buffer_predict(uint8_t dir)
{
    return;
    switch(dir)
    {
        case BUFFER_RX:
        {
            if(wdrv_stats_ptr->rx_win < MAX_NUM_RX_BUFFERS)
            wdrv_stats_ptr->rx_win+=5;
            break;
        }
        case BUFFER_TX:
        {
            if(wdrv_stats_ptr->rx_win > INIT_NUM_RX_BUFFERS)
                wdrv_stats_ptr->rx_win-=5;
            break;
        }
        default:
        {
            break;
        }
    }
}

void wdrv_attach_rx_buffer()
{
    uint16_t num = 0;
#ifdef CONFIG_CONTROLLER_RX_DIRECT_PSH
    return;
#endif
    if(wdrv_stats_ptr->rx_alloc_num <= 2)
    {
        wdrv_rx_buffer_predict(BUFFER_RX);
    }
    num = wdrv_stats_ptr->rx_win - wdrv_stats_ptr->rx_alloc_num;
    wdrv_send_buffer_bank(num);
}

uint8_t* wdrv_get_cmd_buffer(uint8_t type) 
{
    switch(type)
    {
        case CMD_BUF:
        {
            for (int i = 0; i < MAX_NUM_CMD_BUFFERS; i++) 
            {
                uint32_t pattern = *(PTR_FROM_U32(uint32_t,(wdrv_env.cmd_addr[i] - EVENT_HEAD_LEN)));
                //WDRV_LOGE("%s,pattern=0x%x,addr=0x%x \n",__func__,pattern,(PTR_FROM_U32(uint8_t,wdrv_env.cmd_addr[i]) - EVENT_HEAD_LEN));

                if (pattern == PATTERN_FREE) 
                {
                    uint32_t pattern_addr = wdrv_env.cmd_addr[i] - EVENT_HEAD_LEN;
                    *((PTR_FROM_U32(uint32_t,pattern_addr))) = PATTERN_BUSY;
                    
                    WDRV_LOGV("%s,buffer=0x%x \n",__func__,&wdrv_env.cmd_addr[i]);

                    // cppcheck-suppress sizeofwithnumericparameter
                    memset(PTR_FROM_U32(void,wdrv_env.cmd_addr[i]),0,20);
                    
                    return PTR_FROM_U32(uint8_t,wdrv_env.cmd_addr[i]);
                }
            }
            break;
        }
        case CMD_BANK:
        {
            for (int i = 0; i < MAX_NUM_CMD_RX_BANK; i++) 
            {
                uint32_t pattern = *(PTR_FROM_U32(uint32_t,(wdrv_env.cmd_bank[i] - EVENT_HEAD_LEN)));
                //WDRV_LOGE("%s,pattern=0x%x,addr=0x%x \n",__func__,pattern,(PTR_FROM_U32(uint8_t,wdrv_env.cmd_addr[i]) - EVENT_HEAD_LEN));

                if (pattern == PATTERN_FREE) 
                {
                    uint32_t pattern_addr = wdrv_env.cmd_bank[i] - EVENT_HEAD_LEN;
                    *((PTR_FROM_U32(uint32_t,pattern_addr))) = PATTERN_BUSY;
                    
                    WDRV_LOGV("%s,buffer=0x%x \n",__func__,&wdrv_env.cmd_bank[i]);

                    // cppcheck-suppress sizeofwithnumericparameter
                    memset(PTR_FROM_U32(void,wdrv_env.cmd_bank[i]),0,MAX_CMD_BANK_LENGTH);
                    
                    return PTR_FROM_U32(uint8_t,wdrv_env.cmd_bank[i]);
                }
            }
            break;
        }
        default:
        {
            WDRV_LOGE("%s,No available buffer.\n",__func__);
            break;
        }
    }
    return NULL;
}
void wdrv_free_cmd_buffer(uint8_t* buf)
{
    if(!buf)
    {
        WDRV_LOGE("%s,Invalid buffer.\n",__func__);
        return;
    }
    else
    {
        uint32_t* pattern_addr = (uint32_t*)(buf - EVENT_HEAD_LEN);
        *pattern_addr = PATTERN_FREE;
    }
}

/* 
 * +------------+--------------------+
 * |  Header    | Payload            |
 * +------------+--------------------+
 * | 4 BYTE     |  N BYTE            |
 * +------------+--------------------+
 *
 */
void wdrv_tx_cmd_buffer_init()
{
    memset(wdrv_cmd_buffer, 0, MAX_NUM_CMD_BUFFERS*MAX_CMD_BUF_LENGTH);
    memset(wdrv_cmd_bank, 0, MAX_NUM_CMD_RX_BANK*MAX_CMD_BANK_LENGTH);

    for(uint8_t i=0; i<MAX_NUM_CMD_BUFFERS ; i++)
    {
        wdrv_env.cmd_addr[i] = PTR_TO_U32(&wdrv_cmd_buffer[i][EVENT_HEAD_LEN]);//4byte used for buffer mgmt

        uint32_t * temp = (uint32_t *)(&wdrv_cmd_buffer[i][0]);
        *temp = (uint32_t)PATTERN_FREE;

        WDRV_LOGW("%s, addr[%d]=0x%x, pattern_addr=0x%x\n",__func__, i,wdrv_env.cmd_addr[i],temp);
    }
    
    for(uint8_t i=0; i<MAX_NUM_CMD_RX_BANK ; i++)
    {
        wdrv_env.cmd_bank[i] = PTR_TO_U32(&wdrv_cmd_bank[i][EVENT_HEAD_LEN]);//4byte used for buffer mgmt

        uint32_t * temp = (uint32_t *)(&wdrv_cmd_bank[i][0]);
        *temp = (uint32_t)PATTERN_FREE;

        WDRV_LOGV("%s, addr[%d]=0x%x, pattern_addr=0x%x\n",__func__, i,wdrv_env.cmd_bank[i],temp);
    }

    wdrv_stats_ptr->rx_win = INIT_NUM_RX_BUFFERS;

    WDRV_LOGV("%s\n", __func__);
}

void wdrv_main(void *arg)
{
    bk_err_t ret;
    struct wdrv_msg msg;

    while (1)
    {
        ret = rtos_pop_from_queue(&wdrv_env.io_queue, &msg, BEKEN_WAIT_FOREVER);
        if (BK_OK != ret)
        {
            continue;
        }

        WDRV_LOGV("%s,%d, msg.type:%d\n",__func__,__LINE__,msg.type);
        switch (msg.type)
        {
            case WDRV_TASK_MSG_CMD:
                WDRV_LOGV("Wi-Fi Driver receive cmd\r\n");
                wdrv_txdata_pre_process(TX_BK_CMD_DATA,(void*)msg.arg,msg.retry_flag);
                break;
            case WDRV_TASK_MSG_EVENT:
                WDRV_LOGV("Wi-Fi Driver receive event\r\n");
                wdrv_rx_handle_msg((void*)msg.arg);
                break;
            case WDRV_TASK_MSG_TXDATA:
                wdrv_txdata_pre_process(TX_MSDU_DATA,(void*)msg.arg,msg.retry_flag);
                break;
            case WDRV_TASK_MSG_RXDATA:
                wdrv_rxdata_process((struct pbuf *)msg.arg);
                break;
            default:
                break;
        }
        if(wdrv_env.is_init)
            wdrv_attach_rx_buffer();
    }
}

bk_err_t wdrv_init()
{

    bk_err_t ret = BK_OK;
    WDRV_LOGD("%s\n", __func__);

    if(wdrv_env.is_init)
    {
        WDRV_LOGE("%s has already init\n", __func__);
        return ret;
    }

    //LWIP init
    // BK_LOG_ON_ERR(bk_event_init());
    // BK_LOG_ON_ERR(bk_netif_init());

    //IPC interface init
    wdrv_ipc_init();
    //Init event buffer(for CP->AP use)
    wdrv_tx_cmd_buffer_init();
    
    //return BK_OK;
    ret = rtos_init_queue(&wdrv_env.io_queue, "wdrv_queue", sizeof(struct wdrv_msg), WDRV_QUEUE_LEN);
    if (ret != BK_OK)
    {
        WDRV_LOGE("wdrv_init init queue failed:%d\n", ret);
        goto wdrv_init_failed;
    }
    
    
    ret = rtos_core1_create_thread(&wdrv_env.handle,
                                WDRV_TASK_PRIO,
                                "wdrv_thread",
                                (beken_thread_function_t)wdrv_main,
                                4096,
                                (beken_thread_arg_t)0);
    if (ret != BK_OK)
    {
        WDRV_LOGE("wdrv_init create task failed:%d\n", ret);
        goto wdrv_init_failed;
    }

    wdrv_host_init();

    //wdrv_attach_rx_buffer();

    wdrv_env.is_init = 1;

    return ret;
wdrv_init_failed:
    wdrv_deinit();
    return ret;
}

bk_err_t wdrv_deinit()
{
    WDRV_LOGE("ctrl_if_deinit\n");
    if (wdrv_env.handle) {
        rtos_delete_thread(&wdrv_env.handle);
        wdrv_env.handle = NULL;
    }

    if (wdrv_env.io_queue) {
        rtos_deinit_queue(&wdrv_env.io_queue);
        wdrv_env.io_queue = NULL;
    }

    return BK_OK;
}

