/*
 * tkl_ipc.c
 * Copyright (C) 2024 cc <cc@tuya>
 *
 * Distributed under terms of the MIT license.
 */

#include <os/os.h>
#include "sdkconfig.h"
#include "tuya_error_code.h"
#include "tkl_ipc.h"
#include "media_ipc.h"

// #define __PRINT_CORD_ID(x) #x
// #define LOCAL_CPU_CORE_NAME     "cpu"__PRINT_CORD_ID(CONFIG_CPU_INDEX)

#if CONFIG_CPU_INDEX == 0
    #define LOCAL_CPU_CORE_NAME     "cpu0"
#elif CONFIG_CPU_INDEX == 1
    #define LOCAL_CPU_CORE_NAME     "cpu1"
#endif

#define TKL_IPC_CHANNEL_NAME    "aic"   // for application used

OPERATE_RET tkl_ipc_init(TKL_IPC_CONF_T *config, TKL_IPC_HANDLE *handle)
{
    if (handle == NULL || config == NULL) {
        bk_printf("Error: parameter invalid %x %x\r\n", config, handle);
        return OPRT_INVALID_PARM;
    }

    if (config->cb == NULL) {
        bk_printf("Warning: not set ipc cb\r\n");
    }

    media_ipc_chan_cfg_t cfg = {0};
    cfg.cb      = config->cb;
    cfg.name    = TKL_IPC_CHANNEL_NAME;
    cfg.param   = LOCAL_CPU_CORE_NAME;

    int ret = media_ipc_channel_open((meida_ipc_t)handle, &cfg);
    if (ret != BK_OK) {
        bk_printf("Error: create ipc failed, %d\n", ret);
        return OPRT_COM_ERROR;
    }

    bk_printf("%s: create ipc: %s %s\n", __func__, cfg.name, (char *)cfg.param);
    return OPRT_OK;
}

OPERATE_RET tkl_ipc_message_send(TKL_IPC_HANDLE *handle, const uint8_t *buffer, uint32_t length)
{
    bk_printf("--- trace [%s %d] %d\n", __func__, __LINE__, length);
    // MIPC_CHAN_SEND_FLAG_SYNC: need reply
    int ret = media_ipc_send(handle, (void*)buffer, length, MIPC_CHAN_SEND_FLAG_SYNC);
    if (ret != BK_OK) {
        bk_printf("Error: ipc send failed, %d\n", ret);
        return OPRT_COM_ERROR;
    }
    return OPRT_OK;
}

