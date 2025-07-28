/*
 * tkl_ipc.c
 * Copyright (C) 2024 cc <cc@tuya>
 *
 * Distributed under terms of the MIT license.
 */

#include <stdio.h>
#include <os/os.h>
#include "sdkconfig.h"
#include "tuya_error_code.h"
#include "bk_api_ipc.h"
#include "tkl_ipc.h"
#include "tkl_thread.h"
#include "tkl_mutex.h"
#include "tkl_system.h"
#include <driver/aon_rtc.h>

#include "FreeRTOS.h"
#include "task.h"

static uint32_t __tkl_ipc_callback(uint8_t *data, uint32_t size, void *param, ipc_obj_t ipc_obj);

BK_IPC_CHANNEL_DEF(__tkl_ipc_handle);
BK_IPC_CHANNEL_REGISTER(__tkl_ipc_handle, IPC_ROUTE_CPU0_CPU1, __tkl_ipc_callback, NULL, NULL);

static TKL_IPC_FUNC_CB upper_cb = NULL;
static TKL_MUTEX_HANDLE tkl_ipc_sync_mutex = NULL;
static TKL_MUTEX_HANDLE tkl_ipc_async_mutex = NULL;

static uint32_t __tkl_ipc_callback(uint8_t *data, uint32_t size, void *param, ipc_obj_t ipc_obj)
{
    if (upper_cb) {
        upper_cb(data, size);
    }
    return 0;
}


OPERATE_RET tkl_ipc_init(TKL_IPC_CONF_T *config)
{
    if ( config == NULL) {
        bk_printf("Error: parameter invalid %x\r\n", config);
        return OPRT_INVALID_PARM;
    }

    if (config->cb == NULL) {
        bk_printf("Warning: not set ipc cb\r\n");
        return OPRT_INVALID_PARM;
    }

    upper_cb = config->cb;
    tkl_mutex_create_init(&tkl_ipc_sync_mutex);
    tkl_mutex_create_init(&tkl_ipc_async_mutex);

    return OPRT_OK;
}

OPERATE_RET tkl_ipc_send(CONST UINT8_T *buf, UINT32_T buf_len)
{
    tkl_mutex_lock(tkl_ipc_sync_mutex);
    int ret = bk_ipc_send(&__tkl_ipc_handle, (void *)buf, buf_len, MIPC_CHAN_SEND_FLAG_SYNC, 0);
    tkl_mutex_unlock(tkl_ipc_sync_mutex);
    if (ret != 0) {
        bk_printf("ipc sync error\r\n");
        return OPRT_COM_ERROR;
    }
    return OPRT_OK;
}

OPERATE_RET tkl_ipc_send_no_sync( CONST UINT8_T *buf, UINT32_T buf_len)
{
    tkl_mutex_lock(tkl_ipc_async_mutex);
    int ret = bk_ipc_send(&__tkl_ipc_handle, (void *)buf, buf_len, 0, 0);
    tkl_mutex_unlock(tkl_ipc_async_mutex);
    if (ret != 0) {
        bk_printf("ipc async error\r\n");
        return OPRT_COM_ERROR;
    }
    return OPRT_OK;
}

