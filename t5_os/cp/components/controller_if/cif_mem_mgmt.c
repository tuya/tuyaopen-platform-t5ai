#include "cif_main.h"
#include "cif_ipc.h"
#include "cif_co_list.h"
uint8_t cif_tx_event_buffer[MAX_NUM_CMD_LONG_BUF][CIF_MAX_CFM_DATA_LEN] = {0};
uint8_t cif_tx_event_short_buffer[MAX_NUM_CMD_SHORT_BUF][CIF_MAX_CFM_SHORT_LEN] = {0};

struct cif_rx_bank_t * cif_rxbank_ptr = &(cif_env.rx_bank);
void cif_free_ap_txbuf(struct pbuf * pbuf)
{
    CIF_LOGV("%s,%d,p:0x%x\n",__func__,__LINE__,pbuf);
    //send cpdu header addr
    struct cpdu_t * buf  =(struct cpdu_t *)(pbuf+1);
    buf->co_hdr.need_free = 1;
#if CONFIG_CONTROLLER_DEBUG
    TRACK_PBUF_FREE(pbuf);
#endif
    if(cif_msg_sender(pbuf+1,CIF_TASK_MSG_RX_DATA,0) != BK_OK)
    {
        CIF_STATS_INC(cif_tx_buf_leak);
        CIF_LOGE("%s,%d,addr send fail mem_leak:%d\n",__func__,__LINE__,pbuf);
        return;
    }
    cif_stats_ptr->cif_txc_cnt++;
    CIF_STATS_DEC(buf_in_txdata);
}

void cif_save_buffer_addr(void *head)
{
    cif_addr_bank_t * bank = (cif_addr_bank_t *)head;
    uint32_t int_level;
    CIF_LOGV("%s,%d,num=%d\n",__func__,__LINE__,bank->num);
    struct pbuf* p_tmp = NULL;
    //bk_mem_dump("cif_rx_bank",(uint32_t)bank,(bank->num + 3)*4 );

    
    for(uint16_t i=0;i< bank->num; i++)
    {
    
        CIF_IRQ_DISABLE(int_level);
        p_tmp = (struct pbuf* )(bank->addr[i]);

        BK_ASSERT(p_tmp->payload);
        BK_ASSERT(p_tmp->tot_len < 1600);

        cif_rxbank_ptr->rx_buf_bank[cif_rxbank_ptr->rx_buf_bank_cnt] = bank->addr[i];
        cif_rxbank_ptr->rx_buf_bank_cnt++;
        CIF_IRQ_ENABLE(int_level);

        struct pbuf * p = (struct pbuf*)(bank->addr[i]);
        CIF_LOGV("%s,%d i:%d p:%p next:%p payload:%p len:%d\r\n",
            __func__,__LINE__, i, p, p->next, p->payload, p->tot_len);

    }


    cif_free_cmd_buffer((uint8_t*)bank);
}
void cif_rxbank_check()
{
    
    uint8_t * buf = NULL;
    struct pbuf* p_tmp = NULL;
    for(uint8_t i =0;i<cif_rxbank_ptr->rx_buf_bank_cnt ;i++)
    {
    
        buf = (uint8_t *)(cif_rxbank_ptr->rx_buf_bank[i]);
        
        p_tmp = (struct pbuf* )buf;
        //BK_ASSERT((uint32_t)p_tmp>0x28100000);
        //BK_ASSERT((uint32_t)p_tmp < 0x281c0000);
        
        BK_ASSERT(p_tmp->payload);
        BK_ASSERT(p_tmp->tot_len < 1600);
    }

}
uint8_t* cif_maclloc_rx_buf()
{
    uint8_t * buf = NULL;
    uint32_t int_level;
    struct pbuf* p_tmp = NULL;
    uint8_t num = 0;
    CIF_IRQ_DISABLE(int_level);

    //cif_rxbank_check();
    cif_stats_ptr->cif_rx_data++;
    if (cif_rxbank_ptr->rx_buf_bank_cnt == 0)
    {
        CIF_IRQ_ENABLE(int_level);
        
        num = co_list_cnt((void*)&cif_ipc_env[IPC_DATA].rx_list);

        CIF_LOGV("%s buf NULL,IPC cnt:%d\r\n", __func__,num);
        cif_stats_ptr->rx_drop_cnt++;
        return NULL;
    }
    --cif_rxbank_ptr->rx_buf_bank_cnt;

    buf = (uint8_t *)(cif_rxbank_ptr->rx_buf_bank[cif_rxbank_ptr->rx_buf_bank_cnt]);

    p_tmp = (struct pbuf* )buf;
    
    BK_ASSERT(p_tmp->payload);
    BK_ASSERT(p_tmp->tot_len < 1600);

    //cif_rxbank_ptr->rx_buf_bank[cif_rxbank_ptr->rx_buf_bank_cnt] = 0;

    CIF_LOGV("%s buf:%x cnt:%d\r\n", __func__, buf, cif_rxbank_ptr->rx_buf_bank_cnt);
    
    CIF_IRQ_ENABLE(int_level);
    //BK_ASSERT((uint32_t)buf>0x28100000);
    //BK_ASSERT((uint32_t)buf < 0x281c0000);
    return buf;
}
void cif_free_rx_buf(uint32_t buf)
{
    uint32_t int_level;
    struct pbuf* p_tmp = NULL;

    BK_ASSERT(buf);
    
    p_tmp = (struct pbuf* )buf;
    
    BK_ASSERT(p_tmp->payload);
    BK_ASSERT(p_tmp->tot_len < 1600);
    CIF_IRQ_DISABLE(int_level);

    cif_rxbank_ptr->rx_buf_bank[cif_rxbank_ptr->rx_buf_bank_cnt] = buf;
    cif_rxbank_ptr->rx_buf_bank_cnt++;

    CIF_LOGD("%s buf:%x cnt:%d\r\n", __func__, buf, cif_rxbank_ptr->rx_buf_bank_cnt);

    CIF_IRQ_ENABLE(int_level);
}
uint8_t cif_get_event_short_buf_cnt()
{
    uint8_t free_cnt = 0;
    for (int i = 0; i < MAX_NUM_CMD_SHORT_BUF; i++) 
    {
        uint32_t pattern = *((uint32_t*)(cif_env.cmd_addr_short[i] - EVENT_HEAD_LEN));

        if (pattern == PATTERN_FREE) 
        {
            free_cnt++;
        }
    }
    return free_cnt;
}
uint8_t cif_get_event_long_buf_cnt()
{
    uint8_t free_cnt = 0;

    for (int i = 0; i < MAX_NUM_CMD_LONG_BUF; i++) 
    {
        uint32_t pattern = *((uint32_t*)(cif_env.cmd_addr[i] - EVENT_HEAD_LEN));

        if (pattern == PATTERN_FREE) 
        {
            free_cnt++;
        }
    }
    return free_cnt;
}

uint8_t* cif_get_event_buffer(uint16_t size)
{
    uint8_t type = 0xff;
    if(size < CIF_MAX_CFM_SHORT_PAYLOAD_LEN){
        type = CMD_BUF_SHORT;
    }
    else{
        type = CMD_BUF_LONG;
    }
    switch(type)
    {
        case CMD_BUF_LONG:
        {
            for (int i = 0; i < MAX_NUM_CMD_LONG_BUF; i++) 
            {
                uint32_t pattern = *((uint32_t*)(cif_env.cmd_addr[i] - EVENT_HEAD_LEN));

                if (pattern == PATTERN_FREE) 
                {
                    uint32_t pattern_addr = cif_env.cmd_addr[i] - EVENT_HEAD_LEN;
                    *((uint32_t*)pattern_addr) = PATTERN_BUSY;
                    return (uint8_t*)cif_env.cmd_addr[i];
                }
            }
            break;
        }
        case CMD_BUF_SHORT:
        {
            for (int i = 0; i < MAX_NUM_CMD_SHORT_BUF; i++) 
            {
                uint32_t pattern = *((uint32_t*)(cif_env.cmd_addr_short[i] - EVENT_HEAD_LEN));

                if (pattern == PATTERN_FREE) 
                {
                    uint32_t pattern_addr = cif_env.cmd_addr_short[i] - EVENT_HEAD_LEN;
                    *((uint32_t*)pattern_addr) = PATTERN_BUSY;
                    return (uint8_t*)cif_env.cmd_addr_short[i];
                }
            }
            break;
        }
        default:
        {
            CIF_LOGE("%s,No available buffer,size:%d.\n",__func__,size);
            break;
        }
    }
    return NULL;
}
void cif_free_cmd_buffer(uint8_t* buf)
{
    if(!buf)
    {
        CIF_LOGE("%s,Invalid buffer.\n",__func__);
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
void cif_tx_event_buffer_init()
{
    memset(cif_tx_event_buffer, 0, MAX_NUM_CMD_LONG_BUF*CIF_MAX_CFM_DATA_LEN);
    memset(cif_tx_event_short_buffer, 0, MAX_NUM_CMD_LONG_BUF*CIF_MAX_CFM_SHORT_LEN);

    for(uint8_t i=0; i<MAX_NUM_CMD_LONG_BUF ; i++)
    {
        cif_env.cmd_addr[i] = (uint32_t)(&cif_tx_event_buffer[i][EVENT_HEAD_LEN]);//4byte used for buffer mgmt

        uint32_t * temp = (uint32_t *)(&cif_tx_event_buffer[i][0]);
        *temp = (uint32_t)PATTERN_FREE;
    }
    for(uint8_t i=0; i<MAX_NUM_CMD_SHORT_BUF ; i++)
    {
        cif_env.cmd_addr_short[i] = (uint32_t)(&cif_tx_event_short_buffer[i][EVENT_HEAD_LEN]);//4byte used for buffer mgmt

        uint32_t * temp = (uint32_t *)(&cif_tx_event_short_buffer[i][0]);
        *temp = (uint32_t)PATTERN_FREE;
    }
}

