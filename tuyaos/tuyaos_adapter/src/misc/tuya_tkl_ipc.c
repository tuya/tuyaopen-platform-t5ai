/*
 * tuya_tkl_ipc.c
 * Copyright (C) 2024 cc <cc@tuya>
 *
 * Distributed under terms of the MIT license.
 */

#include "sdkconfig.h"
#include "tkl_ipc.h"
#include "tkl_semaphore.h"
#include "tkl_mutex.h"


static TKL_SEM_HANDLE ipc_ack_sem = NULL;
static TKL_MUTEX_HANDLE ipc_async_mutex= NULL;
static TKL_MUTEX_HANDLE ipc_sync_mutex= NULL;
static struct ipc_msg_s *ipc_req_msg = NULL;

extern void tkl_hci_ipc_func(struct ipc_msg_s *msg);
extern void tkl_sys_ipc_func(struct ipc_msg_s *msg);
// extern void tkl_wired_ipc_func(struct ipc_msg_s *msg);

static OPERATE_RET __tuya_ipc_cb(struct ipc_msg_s *msg )
{
    if(ipc_req_msg && ipc_req_msg->type == msg->type && ipc_req_msg->subtype == msg->subtype) {//res...和req是同一个msg地址
        tkl_semaphore_post(ipc_ack_sem);
    }else {
        switch (msg->type) {
            case TKL_IPC_TYPE_HCI:
                tkl_hci_ipc_func(msg);
                break;
            case TKL_IPC_TYPE_SYS:
                tkl_sys_ipc_func(msg);
                break;

#if 0
            case TKL_IPC_TYPE_WIRED:
                tkl_wired_ipc_func(msg);
                break;
#endif

#if (CONFIG_CPU_INDEX == 1) // CONFIG_SYS_CPU1
            case TKL_IPC_TYPE_TEST:
                // cli_tuya_test_ipc_func(msg);
                break;
#endif

            default:
                break;

        }
    }

    return 0;
}

OPERATE_RET tuya_ipc_init(void)
{
    TKL_IPC_CONF_T ipc_conf;

    bk_ipc_init();

    ipc_conf.cb = __tuya_ipc_cb;
    OPERATE_RET ret = tkl_ipc_init(&ipc_conf);
    if(ret)
        return ret;

    ret = tkl_semaphore_create_init(&ipc_ack_sem, 0, 1);
    if(ret)
        return ret;

    ret = tkl_mutex_create_init(&ipc_async_mutex);
    if(ret) {
        tkl_semaphore_release(ipc_ack_sem);
        ipc_ack_sem = NULL;
        return ret;
    }

    ret = tkl_mutex_create_init(&ipc_sync_mutex);
    if(ret) {
        tkl_semaphore_release(ipc_ack_sem);
        tkl_mutex_release(ipc_async_mutex);
        ipc_ack_sem = NULL;
        ipc_async_mutex = NULL;
        return ret;
    }

    // tkl_ipc_test();

    return 0;
}

OPERATE_RET tuya_ipc_send_sync(struct ipc_msg_s *msg)
{
    if(msg == NULL)
        return OPRT_INVALID_PARM;

    tkl_mutex_lock(ipc_sync_mutex);

    ipc_req_msg = msg;

    tkl_ipc_send_no_sync((uint8_t *)msg, sizeof(struct ipc_msg_s));

    // wait ack
    OPERATE_RET ret = tkl_semaphore_wait(ipc_ack_sem, 5000);

    ipc_req_msg = NULL;

    tkl_mutex_unlock(ipc_sync_mutex);

    return ret;
}


OPERATE_RET tuya_ipc_send_no_sync(struct ipc_msg_s *msg)
{
    if(msg == NULL)
        return OPRT_INVALID_PARM;

    tkl_mutex_lock(ipc_async_mutex);

    OPERATE_RET ret = tkl_ipc_send_no_sync((uint8_t *)msg, sizeof(struct ipc_msg_s));

    tkl_mutex_unlock(ipc_async_mutex);

    return ret;
}


