#pragma once

#ifdef __cplusplus
extern "C" {
#endif


//#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>
#include "cli.h"
#include <driver/pwr_clk.h>
#include <components/log.h>
#include <driver/mailbox_channel.h>
#include "wdrv_main.h"

#define WIFI_IPC_CMD_CHNL   MB_CHNL_WIFI_CMD
#define WIFI_IPC_DATA_CHNL  MB_CHNL_WIFI_DATA

typedef bk_err_t (*mb_open_t)(uint8_t channel,void * param);
typedef bk_err_t (*mb_close_t)(uint8_t channel);
typedef bk_err_t (*mb_config_t)(uint8_t channel,uint8_t cmd, void * param);
typedef bk_err_t (*mb_send_t)(uint8_t channel,mb_chnl_cmd_t * param);
typedef bk_err_t (*mb_close_t)(uint8_t channel);

typedef struct 
{
    uint8_t     channel;
    mb_open_t   open;
    mb_close_t  close;
    mb_config_t cb_register;
    mb_send_t   send;
    beken_semaphore_t sema;
    uint8_t sending_flag;
    struct co_list tx_list;
}wdrv_ipc_t;

typedef struct
{
    uint32_t         ipc_hdr;//for IPC use

    uint32_t         head;
    uint32_t         tail;
    uint8_t          channel;
    uint8_t          num;
    uint16_t         rsve;
} ipc_chnl_node_t;


enum tx_data_type
{
    TX_BK_CMD_DATA =   0,
    TX_CEVA_CMD_DATA = 1,
    TX_MSDU_DATA =     2,
    TX_MPDU_DATA =     3,
};
enum rx_data_type
{
    RX_BK_CMD_DATA =   0,
    RX_CEVA_CMD_DATA = 1,
    RX_MSDU_DATA =     2,
    RX_MPDU_DATA =     3,
};
enum wifi_data_type
{
    IPC_CMD,
    IPC_DATA,
    IPC_MAX
};

uint8_t wdrv_map_to_ipc_chnl(uint8_t channel);

extern bk_err_t wdrv_ipc_init();
extern wdrv_ipc_t wdrv_ipc_env[IPC_MAX];
#ifdef __cplusplus
}
#endif
