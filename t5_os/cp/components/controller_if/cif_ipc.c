#include "cif_ipc.h"

extern uint8_t cif_dnld_buffer(void *param, void *node);

cif_ipc_t cif_ipc_env[IPC_MAX];

uint8_t cif_map_to_rx_wifi_type(uint8_t channel)
{
    switch(channel)
    {
        case IPC_CMD:
            return  RX_BK_CMD_DATA;

        case IPC_DATA:
            return RX_MSDU_DATA;

        default:
            BK_ASSERT(0);
    }
    return 0xff;
}
uint8_t cif_map_to_ipc_chnl(uint8_t channel)
{
    switch(channel)
    {
        case RX_BK_CMD_DATA:
            return IPC_CMD;
            
        case RX_CEVA_CMD_DATA:
            return IPC_CMD;
            
        case RX_MSDU_DATA:
            return IPC_DATA;

        case RX_MPDU_DATA:
            return IPC_DATA;
        default:
            BK_ASSERT(0);
    }
    return 0XFF;
}


void cif_ipc_env_init()
{
    cif_ipc_env[IPC_CMD].channel       = WIFI_IPC_CMD_CHNL;
    cif_ipc_env[IPC_CMD].open          = mb_chnl_open;
    cif_ipc_env[IPC_CMD].close         = mb_chnl_close;
    cif_ipc_env[IPC_CMD].cb_register   = mb_chnl_ctrl;
    cif_ipc_env[IPC_CMD].send          = mb_chnl_write;

    cif_ipc_env[IPC_DATA].channel       = WIFI_IPC_DATA_CHNL;
    cif_ipc_env[IPC_DATA].open          = mb_chnl_open;
    cif_ipc_env[IPC_DATA].close         = mb_chnl_close;
    cif_ipc_env[IPC_DATA].cb_register   = mb_chnl_ctrl;
    cif_ipc_env[IPC_DATA].send          = mb_chnl_write;
}

bk_err_t cif_ipc_init()
{
    bk_err_t ret = BK_OK;

    cif_ipc_env_init();

    for(uint8_t i=0; i<IPC_MAX; i++)
    {
        /* reigster a mailbox logical channel */
        ret = cif_ipc_env[i].open(cif_ipc_env[i].channel,NULL);
        if(ret != BK_OK)
        {
            CIF_LOGE("ipc init fail\n");
            return ret;
        }
        /* register mailbox logical channel rx callbcak */
        cif_ipc_env[i].cb_register(cif_ipc_env[i].channel, MB_CHNL_SET_RX_ISR, cif_dnld_buffer);
        if(ret != BK_OK)
        {
            CIF_LOGE("ipc rx callback register fail\n");
            return ret;
        }
    }

    /* register mailbox logical channel rxc callbcak */
    ret = cif_ipc_env[IPC_DATA].cb_register(cif_ipc_env[IPC_DATA].channel, MB_CHNL_SET_TX_CMPL_ISR, cif_rx_data_complete);
    if(ret != BK_OK)
    {
        CIF_LOGE("ipc rxc data callback register fail\n");
        return ret;
    }

    /* register mailbox logical channel rxc callbcak */
    ret = cif_ipc_env[IPC_CMD].cb_register(cif_ipc_env[IPC_CMD].channel, MB_CHNL_SET_TX_CMPL_ISR, cif_rx_evt_complete);
    if(ret != BK_OK)
    {
        CIF_LOGE("ipc rxc evt callback register fail\n");
        return ret;
    }

    co_list_init((struct co_list *)&cif_ipc_env[IPC_CMD].rx_list);
    co_list_init((struct co_list *)&cif_ipc_env[IPC_DATA].rx_list);

    return ret;
}

bk_err_t cif_ipc_deinit()
{
    return BK_OK;
}

