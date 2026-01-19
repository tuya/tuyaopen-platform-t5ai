#include "wdrv_ipc.h"
#include "wifi_cli.h"
#include "wdrv_rx.h"
#include "wdrv_tx.h"

wdrv_ipc_t wdrv_ipc_env[IPC_MAX] = {0};

uint8_t wdrv_map_to_ipc_chnl(uint8_t channel)
{
    switch(channel)
    {
        case TX_BK_CMD_DATA:
            return IPC_CMD;
            
        case TX_CEVA_CMD_DATA:
            return IPC_CMD;
            
        case TX_MSDU_DATA:
            return IPC_DATA;

        case TX_MPDU_DATA:
            return IPC_DATA;
        default:
            BK_ASSERT(0);
    }
    return 0XFF;
}


bk_err_t wdrv_ipc_env_init()
{
    bk_err_t ret = BK_OK;
    WDRV_LOGD("%s\n", __func__);

    wdrv_ipc_env[IPC_CMD].channel       = WIFI_IPC_CMD_CHNL;
    wdrv_ipc_env[IPC_CMD].open          = mb_chnl_open;
    wdrv_ipc_env[IPC_CMD].close         = mb_chnl_close;
    wdrv_ipc_env[IPC_CMD].cb_register   = mb_chnl_ctrl;
    wdrv_ipc_env[IPC_CMD].send          = mb_chnl_write;

    wdrv_ipc_env[IPC_DATA].channel       = WIFI_IPC_DATA_CHNL;
    wdrv_ipc_env[IPC_DATA].open          = mb_chnl_open;
    wdrv_ipc_env[IPC_DATA].close         = mb_chnl_close;
    wdrv_ipc_env[IPC_DATA].cb_register   = mb_chnl_ctrl;
    wdrv_ipc_env[IPC_DATA].send          = mb_chnl_write;

//    ret = rtos_init_semaphore(&wdrv_ipc_env[IPC_DATA].sema, 1);
//    ret = rtos_init_semaphore(&wdrv_ipc_env[IPC_CMD].sema, 1);
    if(ret != BK_OK) 
    {
        WDRV_LOGE("%s,%d,sema_init fail\n",__func__,__LINE__);
        return BK_FAIL;
    }
    
//    rtos_set_semaphore(&wdrv_ipc_env[IPC_DATA].sema);
//    rtos_set_semaphore(&wdrv_ipc_env[IPC_CMD].sema);

    return BK_OK;
}

bk_err_t wdrv_ipc_init()
{
    bk_err_t ret = BK_OK;
    BK_LOGD(NULL, "%s\n", __func__);

    ret = wdrv_ipc_env_init();
    if(ret != BK_OK)
    {
        WDRV_LOGE("wdrv_ipc_env_init fail\n");
        return ret;
    }

    for(uint8_t i=0; i<IPC_MAX; i++)
    {
        /* reigster a mailbox logical channel */
        ret = wdrv_ipc_env[i].open(wdrv_ipc_env[i].channel,NULL);
        if(ret != BK_OK)
        {
            WDRV_LOGE("ipc init fail\n");
            return ret;
        }
        /* register mailbox logical channel rx callbcak */
        wdrv_ipc_env[i].cb_register(wdrv_ipc_env[i].channel, MB_CHNL_SET_RX_ISR, wdrv_recv_buffer);
        if(ret != BK_OK)
        {
            WDRV_LOGE("ipc rx callback register fail\n");
            return ret;
        }

        co_list_init((struct co_list *)&wdrv_ipc_env[i].tx_list);
    }


    /* register mailbox logical channel txc callbcak */
    wdrv_ipc_env[IPC_CMD].cb_register(wdrv_ipc_env[IPC_CMD].channel, MB_CHNL_SET_TX_CMPL_ISR, wdrv_tx_msg_complete);
    if(ret != BK_OK)
    {
        WDRV_LOGE("ipc txc callback register fail\n");
        return ret;
    }

    /* register mailbox logical channel txc callbcak */
    wdrv_ipc_env[IPC_DATA].cb_register(wdrv_ipc_env[IPC_DATA].channel, MB_CHNL_SET_TX_CMPL_ISR, wdrv_tx_complete);
    if(ret != BK_OK)
    {
        WDRV_LOGE("ipc txc callback register fail\n");
        return ret;
    }

    wdrv_cli_init();

    return ret;
}

bk_err_t wdrv_ipc_deinit()
{
    return BK_OK;
}


