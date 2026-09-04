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
#include "tkl_memory.h"


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

    /* Redmine #8131 (item 3): the CP writes the response back into this same
     * physical buffer (matched by address/type in __tuya_ipc_cb) and the caller
     * reads it on its own core. If 'msg' lives on a Cacheable PSRAM task stack,
     * the AP and CP see the buffer through non-coherent private D-caches ->
     * stale request on the CP / stale response on the caller (worst case the
     * ack is never matched and the 5s wait times out). Bounce the exchange
     * through a non-cacheable (coherent) heap buffer for its whole lifetime. */
    struct ipc_msg_s *bmsg = tkl_system_malloc(sizeof(struct ipc_msg_s));
    if (bmsg == NULL) {
        tkl_mutex_unlock(ipc_sync_mutex);
        return OPRT_MALLOC_FAILED;
    }
    tkl_system_memcpy(bmsg, msg, sizeof(struct ipc_msg_s));

    /* Redmine #8131: the struct copy above is shallow - req_param/res_param still
     * point at the caller's (possibly Cacheable PSRAM stack) buffers, which the CP
     * would dereference through a non-coherent private D-cache. Bounce the
     * referenced payloads through the same coherent heap so no bare cacheable-stack
     * pointer crosses cores. (Any further pointer nesting inside the payload, e.g.
     * ipc_msg_param_s.p1/p2, remains the RPC contract's responsibility.) */
    uint8_t *req_bounce = NULL;
    uint8_t *res_bounce = NULL;
    if ((msg->req_param != NULL) && (msg->req_len > 0)) {
        req_bounce = tkl_system_malloc(msg->req_len);
        if (req_bounce == NULL) {
            tkl_system_free(bmsg);
            tkl_mutex_unlock(ipc_sync_mutex);
            return OPRT_MALLOC_FAILED;
        }
        tkl_system_memcpy(req_bounce, msg->req_param, msg->req_len);
        bmsg->req_param = req_bounce;
    }
    if ((msg->res_param != NULL) && (msg->res_len > 0)) {
        res_bounce = tkl_system_malloc(msg->res_len);
        if (res_bounce == NULL) {
            if (req_bounce != NULL)
                tkl_system_free(req_bounce);
            tkl_system_free(bmsg);
            tkl_mutex_unlock(ipc_sync_mutex);
            return OPRT_MALLOC_FAILED;
        }
        tkl_system_memcpy(res_bounce, msg->res_param, msg->res_len);
        bmsg->res_param = res_bounce;
    }

    ipc_req_msg = bmsg;

    tkl_ipc_send_no_sync((uint8_t *)bmsg, sizeof(struct ipc_msg_s));

    // wait ack
    OPERATE_RET ret = tkl_semaphore_wait(ipc_ack_sem, 5000);

    ipc_req_msg = NULL;

    /* Copy any response payload back into the caller's buffer, then restore the
     * caller's original pointer fields before copying the header back so the
     * caller never receives (and later frees) a bounce address. */
    if (res_bounce != NULL) {
        tkl_system_memcpy(msg->res_param, res_bounce, msg->res_len);
    }
    bmsg->req_param = msg->req_param;
    bmsg->res_param = msg->res_param;
    tkl_system_memcpy(msg, bmsg, sizeof(struct ipc_msg_s));

    if (res_bounce != NULL)
        tkl_system_free(res_bounce);
    if (req_bounce != NULL)
        tkl_system_free(req_bounce);
    tkl_system_free(bmsg);

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


