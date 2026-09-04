#include <common/bk_include.h>
#include <common/bk_err.h>
#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <string.h>

#include "FreeRTOSConfig.h"
#include "FreeRTOS.h"
#include "event_groups.h"

#include <modules/pm.h>

#include <components/log.h>
#include <driver/mailbox_channel.h>
#include "avdk_crc.h"
#include "bk_api_ipc.h"
#include "bk_api_ipc_test.h"

#include <driver/pwr_clk.h>

#if (CONFIG_CACHE_ENABLE || CONFIG_DCACHE)
#include "cache.h"
#endif

/* Redmine #8131 (item 3): the earlier attempt here did directed cache-line
 * clean/invalidate on the caller's 'struct ipc_msg_s' when it lived on a
 * Cacheable PSRAM task stack. That was unsafe (whole-line invalidate can drop
 * neighbouring live stack data) and did not even cover the real Tuya sync path,
 * which reaches bk_ipc_send() via tkl_ipc_send_no_sync() with flags=0 (async
 * transport). The coherency for ipc_msg_s is now handled by a non-cacheable
 * heap bounce inside tuya_ipc_send_sync() (tuya_tkl_ipc.c), so no cache
 * maintenance is required in this transport layer. */

#if (CONFIG_PSRAM_AS_SYS_MEMORY && CONFIG_FREERTOS_SMP && CONFIG_DCACHE)
/* Redmine #8131: does 'p' point into the Cacheable PSRAM task-stack heap? Such a
 * buffer is seen by the AP and CP through non-coherent private D-caches, so a
 * cross-core IPC that shares the bare pointer would read stale data. Only the
 * task-stack heap is cacheable; SRAM and the general PSRAM heap are already
 * non-cacheable (coherent) and need no bounce. */
static inline int bk_ipc_data_in_cacheable_stack(const void *p)
{
#ifdef CONFIG_AP_PSRAM_STACK_HEAP_ADDR
	uint32_t a = (uint32_t)p;
	return (a >= (uint32_t)CONFIG_AP_PSRAM_STACK_HEAP_ADDR) &&
	       (a <  (uint32_t)(CONFIG_AP_PSRAM_STACK_HEAP_ADDR + CONFIG_AP_PSRAM_STACK_HEAP_SIZE));
#else
	(void)p;
	return 0;
#endif
}
#endif

BK_SECTION_DEF(ipc_chan_reg, bk_ipc_chan_cfg_t);

#define IPC_SUPPORTED_CRC

#define IPC_EVENT_SEND              (1 << 0)
#define IPC_EVENT_RECV              (1 << 1)
#define IPC_EVENT_FREE              (1 << 2)
#define IPC_EVENT_LOCAL_SHUTDOWN    (1 << 4)

#define IPC_EVENT_WAITING_BITS      (IPC_EVENT_SEND | IPC_EVENT_RECV | IPC_EVENT_FREE | IPC_EVENT_LOCAL_SHUTDOWN)

typedef enum
{
    IPC_TYPE_RESERVED = 0,
    IPC_TYPE_CMD,
    IPC_TYPE_ACK,
    IPC_TYPE_SYS,
} ipc_header_type_t;

typedef enum
{
    IPC_CPU_SHUTDOWN = 0,
    IPC_CPU_STARTUP,
} ipc_cpu_state_t;

typedef union
{
    struct
    {
        uint8_t source : 3;
        uint8_t type : 5;
    } ;
    uint8_t data;
} ipc_header_t;

typedef union
{
    struct
    {
        uint8_t crc : 8;
    } ;
    uint32_t data;
} ipc_result_t;


#define TAG "bk_ipc"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

#if CONFIG_SOC_SMP
#include "spinlock.h"
static SPINLOCK_SECTION volatile spinlock_t bk_ipc_spin_lock = SPIN_LOCK_INIT;
#endif // CONFIG_SOC_SMP
static inline uint32_t bk_ipc_enter_critical()
{
	uint32_t flags = rtos_disable_int();

#if CONFIG_SOC_SMP
	spin_lock(&bk_ipc_spin_lock);
#endif // CONFIG_SOC_SMP

	return flags;
}

static inline void bk_ipc_exit_critical(uint32_t flags)
{
#if CONFIG_SOC_SMP
	spin_unlock(&bk_ipc_spin_lock);
#endif // CONFIG_SOC_SMP

	rtos_enable_int(flags);
}

bk_ipc_info_t *bk_ipc_info = NULL;


bk_ipc_core_t bk_ipc_cpu_id_get(void)
{
    bk_ipc_core_t id = 0xFF;

    id = IPC_CORE_CPU1; //rtos_get_core_id();

    return id;
}

bk_ipc_info_t *bk_ipc_info_get(bk_ipc_route_t route)
{
    if (route == IPC_ROUTE_CPU0_CPU1)
    {
        return bk_ipc_info;
    }

    return NULL;
}


bk_ipc_mailbox_state_t bk_ipc_mailbox_state_get(void)
{
    bk_err_t ret = BK_OK;
    uint8_t state = 0;
    uint8_t channel_id = 0;

    channel_id  = MB_CHNL_MIPC_SYNC;

    ret = mb_chnl_ctrl(channel_id, MB_CHNL_GET_STATUS, &state);

    if (ret != BK_OK)
    {
        LOGE("%s, get state error: %X\n", __func__, ret);
        return IPC_MSTATE_ERROR;
    }

    if (state == 0 /*CHNL_STATE_ILDE*/)
    {
        return IPC_MSTATE_IDLE;
    }
    else if (state == 1/*CHNL_STATE_BUSY*/)
    {
        return IPC_MSTATE_BUSY;
    }
    else
    {
        LOGE("%s, get unknow state: %X\n", __func__, state);
    }

    return IPC_MSTATE_ERROR;
}

static void bk_ipc_event_notify(bk_ipc_info_t *ipc_info, uint32_t event)
{
    uint32_t flags  = bk_ipc_enter_critical();

    ipc_info->waiting_event |= event;
    rtos_set_semaphore(&ipc_info->waiting_sem);

    bk_ipc_exit_critical(flags);
}

bk_ipc_data_t *bk_ipc_data_pop(LIST_HEADER_T *list)
{
    uint32_t flags  = bk_ipc_enter_critical();
    LIST_HEADER_T *pos, *n;
    bk_ipc_data_t *node = NULL;
    bk_ipc_data_t *tmp = NULL;

    if (!list_empty(list))
    {
        list_for_each_safe(pos, n, list)
        {
            tmp = list_entry(pos, bk_ipc_data_t, list);
            if (tmp != NULL)
            {
                node = tmp;
                list_del(pos);
                break;
            }
        }
    }
    bk_ipc_exit_critical(flags);

    return node;
}

void bk_ipc_data_clear(LIST_HEADER_T *list)
{
    uint32_t flags  = bk_ipc_enter_critical();
    LIST_HEADER_T *pos, *n;
    bk_ipc_data_t *tmp = NULL;

    if (!list_empty(list))
    {
        list_for_each_safe(pos, n, list)
        {
            tmp = list_entry(pos, bk_ipc_data_t, list);
            if (tmp != NULL)
            {
                list_del(pos);
                os_free(tmp);
            }
        }
    }
    bk_ipc_exit_critical(flags);
}

bk_err_t bk_ipc_list_insert(LIST_HEADER_T *list, LIST_HEADER_T *node)
{
    uint32_t flags  = bk_ipc_enter_critical();
    list_add_tail(node, list);
    bk_ipc_exit_critical(flags);
    return BK_OK;
}


bk_err_t bk_ipc_data_push(LIST_HEADER_T *list, bk_ipc_data_t *data)
{
    bk_err_t ret = BK_OK;

    if (data == NULL)
    {
        LOGE("%s bk_ipc_data_t NULL\n", __func__);
        return -1;
    }

    uint32_t flags  = bk_ipc_enter_critical();
    list_add_tail(&data->list, list);
    bk_ipc_exit_critical(flags);
    return ret;
}

bk_ipc_handle_t *bk_ipc_get_handle_by_name(LIST_HEADER_T *list, char *name)
{
    if (name == NULL || strlen(name) == 0)
    {
        LOGE("%s invalid channel: %s\n", __func__, name);
        return NULL;
    }

    if (list == NULL || list_empty(list))
    {
        LOGV("%s invalid list\n", __func__);
        return NULL;
    }

    uint32_t flags  = bk_ipc_enter_critical();
    LIST_HEADER_T *pos, *n;
    bk_ipc_handle_t *node = NULL;
    bk_ipc_handle_t *tmp = NULL;

    list_for_each_safe(pos, n, list)
    {
        tmp = list_entry(pos, bk_ipc_handle_t, list);
        if (tmp != NULL && tmp->cfg->name)
        {
            if (0 == strcmp(tmp->cfg->name, name))
            {
                node = tmp;
                break;
            }
        }
    }

    bk_ipc_exit_critical(flags);
    return node;
}


bk_err_t bk_ipc_channel_list_remove(LIST_HEADER_T *list, char *name)
{
    bk_err_t ret = BK_FAIL;

    if (name == NULL || strlen(name) == 0)
    {
        LOGE("%s invalid channel: %s\n", __func__, name);
        return ret;
    }

    if (list == NULL || list_empty(list))
    {
        LOGE("%s invalid list\n", __func__);
        return ret;
    }

    uint32_t flags  = bk_ipc_enter_critical();
    LIST_HEADER_T *pos, *n;
    bk_ipc_handle_t *tmp = NULL;

    list_for_each_safe(pos, n, list)
    {
        tmp = list_entry(pos, bk_ipc_handle_t, list);
        if (tmp != NULL && tmp->cfg->name)
        {
            if (0 == strcmp(tmp->cfg->name, name))
            {
                list_del(pos);
                ret = BK_OK;
                break;
            }
        }
    }

    bk_ipc_exit_critical(flags);
    return ret;
}

static int bk_ipc_send_async(bk_ipc_data_t *ipc_data)
{
    mb_chnl_cmd_t mb_cmd;
    bk_err_t ret = BK_OK;
    uint8_t channel = 0;

    mb_cmd.hdr.cmd = bk_ipc_cpu_id_get();
    mb_cmd.param1 = (uint32_t)ipc_data;
    mb_cmd.param2 = 0;
    mb_cmd.param3 = 0;

    channel = MB_CHNL_MIPC_SYNC;

    ret = mb_chnl_write(channel, &mb_cmd);

    rtos_deinit_semaphore(&ipc_data->sem);
    ipc_data->sem = NULL;

    return ret;
}

static inline int bk_ipc_send_original(bk_ipc_info_t *ipc_info, ipc_header_t *header,
                                         bk_ipc_data_t *ipc_data, uint32_t status, uint32_t reserved)
{
    mb_chnl_cmd_t mb_cmd;
    uint8_t channel = 0;
    int ret = BK_OK;

    mb_cmd.hdr.cmd = header->data;
    mb_cmd.param1 = (uint32_t)ipc_data;
    mb_cmd.param2 = status;
    mb_cmd.param3 = reserved;

    channel = MB_CHNL_MIPC_SYNC;

    ret = rtos_get_semaphore(&ipc_info->sem, BEKEN_WAIT_FOREVER);

    if (ret != BK_OK)
    {
        LOGE("%s wait comm semaphore failed\n", __func__);
        return -1;
    }

    ret = mb_chnl_write(channel, &mb_cmd);

    if (ret != BK_OK)
    {
        LOGE("%s mb_chnl_write failed\n", __func__);
        rtos_set_semaphore(&ipc_info->sem);
        return -1;
    }

    if ((uint32_t)ipc_data == IPC_CPU_SHUTDOWN && bk_ipc_cpu_id_get() == IPC_CORE_CPU1)
    {
        // wait tx complete, make sure cpu0 have received shutdown notify
        ret = rtos_get_semaphore(&ipc_info->sem, BEKEN_WAIT_FOREVER);

        if (ret != BK_OK)
        {
            LOGE("%s wait shutdown tx_cmpt failed\n", __func__);
            return -1;
        }
    }

    return ret;
}

static inline int bk_ipc_send_mailbox(bk_ipc_info_t *ipc_info, ipc_header_t *header,
                                        bk_ipc_data_t *ipc_data)
{
    ipc_result_t result;

#ifdef IPC_SUPPORTED_CRC
    if (header->type != IPC_TYPE_SYS && ipc_data)
    {
        result.crc = hnd_crc8((uint8_t *)&ipc_data->result,
                              sizeof(bk_ipc_data_t) - sizeof(LIST_HEADER_T) - sizeof(ipc_data->result),
                              0xFF);
    }
    else
    {
        result.data = 0;
    }
#endif


    return bk_ipc_send_original(ipc_info, header, ipc_data, result.data, 0);
}


static int bk_ipc_send_sync(bk_ipc_info_t *ipc_info, bk_ipc_data_t *ipc_data)
{
    bk_err_t ret = BK_OK;
    ipc_header_t header;

    ret = rtos_init_semaphore_ex(&ipc_data->sem, 1, 0);

    if (ret != BK_OK)
    {
        LOGE("%s init semaphore failed 0x%x\n", __func__, ret);
        goto out;
    }

    header.source = bk_ipc_cpu_id_get();
    header.type = IPC_TYPE_CMD;
    ret = bk_ipc_send_mailbox(ipc_info, &header, ipc_data);

    if (ret != BK_OK)
    {
        LOGE("%s write mailbox failed\n", __func__);
        goto out;
    }

    ret = rtos_get_semaphore(&ipc_data->sem, BEKEN_WAIT_FOREVER);

    if (ret != BK_OK)
    {
        LOGE("%s wait local semaphore failed\n", __func__);
        goto out;
    }

out:

    if (ipc_data->sem)
    {
        rtos_deinit_semaphore(&ipc_data->sem);
    }
    ipc_data->sem = NULL;

    return ret;
}

int bk_ipc_send(bk_ipc_t *ipc, void *data, uint32_t size, uint32_t flags, uint32_t *result)
{
    bk_err_t ret = BK_OK;
    bk_ipc_data_t *ipc_data;
    bk_ipc_handle_t *bk_ipc_handle = (bk_ipc_handle_t *)(*ipc);
    bk_ipc_info_t *ipc_info = bk_ipc_handle->ipc;
    void *xfer = data;
#if (CONFIG_PSRAM_AS_SYS_MEMORY && CONFIG_FREERTOS_SMP && CONFIG_DCACHE)
    void *bounce = NULL;
#endif

    LOGV("%s %d ++\n", __func__, __LINE__);

    if (ipc_info == NULL)
    {
        LOGE("%s ipc not ready\n", __func__);
        return -1;
    }

    ipc_data = (bk_ipc_data_t *)os_malloc(sizeof(bk_ipc_data_t));

    if (ipc_data == NULL)
    {
        LOGE("%s ipc_data malloc failed\n", __func__);
        return -1;
    }

    os_memset(ipc_data, 0, sizeof(bk_ipc_data_t));

#if (CONFIG_PSRAM_AS_SYS_MEMORY && CONFIG_FREERTOS_SMP && CONFIG_DCACHE)
    /* Redmine #8131: the transport shares the caller's 'data' pointer with the CP
     * (bare physical address). When the caller passes a buffer on its Cacheable
     * PSRAM task stack, the CP reads a stale copy and, for a SYNC exchange where
     * the CP writes the response in place, the caller reads a stale response.
     * For the SYNC branch the caller blocks for the whole exchange, so it is safe
     * to bounce through a non-cacheable (coherent) heap buffer and copy the
     * response back afterwards. The earlier directed cache-line maintenance was
     * removed because a whole-line invalidate could drop neighbouring live stack
     * data; a copy bounce has no such hazard. */
    if ((flags & MIPC_CHAN_SEND_FLAG_SYNC) && (data != NULL) && (size != 0) &&
        bk_ipc_data_in_cacheable_stack(data))
    {
        bounce = os_malloc(size);
        if (bounce != NULL)
        {
            os_memcpy(bounce, data, size);
            xfer = bounce;
        }
        else
        {
            /* Redmine #8131: a Cacheable PSRAM stack buffer MUST NOT be shared with
             * the CP without the coherent bounce. Degrading to the raw stack pointer
             * would resurface the stale-request/stale-response coherency failure this
             * bounce exists to prevent, so fail the call instead. */
            LOGE("%s bounce malloc(%u) failed, aborting sync send\n", __func__, size);
            ret = BK_ERR_NO_MEM;
            goto out;
        }
    }
#endif

    ipc_data->data = xfer;
    ipc_data->size = size;
    ipc_data->handle = bk_ipc_handle;
    ipc_data->flags = flags;

    if (flags & MIPC_CHAN_SEND_FLAG_SYNC)
    {
        LOGV("%s mailbox send wait +++\n", __func__);
        ret = bk_ipc_send_sync(ipc_info, ipc_data);
        LOGV("%s mailbox send wait ---\n", __func__);

        if (ret != BK_OK)
        {
            LOGE("%s ipc send sync failed 0x%x\n", __func__, ret);
            goto out;
        }

#if (CONFIG_PSRAM_AS_SYS_MEMORY && CONFIG_FREERTOS_SMP && CONFIG_DCACHE)
        /* Copy the CP's in-place response from the coherent bounce back to the
         * caller's buffer before returning. */
        if (bounce != NULL)
        {
            os_memcpy(data, bounce, size);
        }
#endif
    }
    else
    {
        bk_ipc_data_push(&ipc_info->local_list, ipc_data);
        bk_ipc_event_notify(ipc_info, IPC_EVENT_SEND);
    }

    if (result)
    {
        *result = ipc_data->result;
    }

out:

#if (CONFIG_PSRAM_AS_SYS_MEMORY && CONFIG_FREERTOS_SMP && CONFIG_DCACHE)
    if (bounce != NULL)
    {
        os_free(bounce);
        bounce = NULL;
    }
#endif

    if ((flags & MIPC_CHAN_SEND_FLAG_SYNC) && (ipc_data != NULL))
    {
        os_free(ipc_data);
        ipc_data = NULL;
    }

    if (ret != BK_OK && result)
    {
        *result = 0;
    }

    LOGV("%s %d --\n", __func__, __LINE__);

    return ret;
}

bk_err_t bk_ipc_obj_extract(ipc_obj_t obj)
{
    bk_ipc_data_t *bk_ipc_data = (bk_ipc_data_t *)obj;
    bk_ipc_data->flags |= MIPC_CHAN_HAND_FLAG_ASYNC;
    return BK_OK;
}

void *bk_ipc_obj_convert(ipc_obj_t obj, uint32_t *size)
{
    bk_ipc_data_t *bk_ipc_data = (bk_ipc_data_t *)obj;

    if (bk_ipc_data == NULL)
    {
        *size = 0;
        LOGE("%s inviald obj\n", __func__);
        return NULL;
    }

    *size = bk_ipc_data->size;
    return bk_ipc_data->data;
}

bk_err_t bk_ipc_obj_free(ipc_obj_t obj, uint32_t result)
{
    bk_ipc_data_t *bk_ipc_data = (bk_ipc_data_t *)obj;
    bk_ipc_info_t *ipc_info = NULL;
    ipc_header_t header;

    if (bk_ipc_data == NULL)
    {
        LOGE("%s inviald obj\n", __func__);
        return BK_FAIL;
    }

    ipc_info = (bk_ipc_info_t *)bk_ipc_data->result;

    if (bk_ipc_data == NULL)
    {
        LOGE("%s inviald ipc_info\n", __func__);
        return BK_FAIL;
    }

    bk_ipc_data->result = result;

    LOGV("send ack\n");

    header.source = bk_ipc_cpu_id_get();
    header.type = IPC_TYPE_ACK;
    return bk_ipc_send_mailbox(ipc_info, &header, bk_ipc_data);
}

static void bk_ipc_crc_check(bk_ipc_data_t *data, uint32_t result)
{
    ipc_result_t ipc_result;
    uint8_t crc = hnd_crc8((uint8_t *)&data->result,
                           sizeof(bk_ipc_data_t) - sizeof(LIST_HEADER_T) - sizeof(data->result),
                           0xFF);

    ipc_result.data = result;

    if (crc != ipc_result.crc)
    {
        LOGE("%s crc check failed\n", __func__);
        return;
    }

    LOGV("%s crc check : %02X %02X\n", __func__, crc, ipc_result.crc);
}

static void bk_ipc_mailbox_rx_isr(void *param, mb_chnl_cmd_t *cmd_buf)
{
    bk_ipc_data_t *data = (bk_ipc_data_t *)cmd_buf->param1;
    bk_ipc_handle_t *handle;
    ipc_header_t header;
    bk_ipc_info_t *ipc_info = (bk_ipc_info_t *)param;

    /* Redmine #8131: this RX ISR consumes the peer's payload. A whole-cache
     * flush_all_dcache() here was both wrong scope (unbounded ISR latency) and
     * wrong direction (clean could evict this core's live data over the peer's).
     * The bk_ipc_data_t header itself lives in non-cacheable heap, so only the
     * referenced payload needs a targeted invalidate before the peer's bytes are
     * read. Gate on the real D-cache switch, not the stale CONFIG_CACHE_ENABLE. */
#if (CONFIG_PSRAM_AS_SYS_MEMORY && CONFIG_FREERTOS_SMP && CONFIG_DCACHE)
    if ((data != NULL) && (data->data != NULL) && (data->size != 0))
    {
        invalidate_dcache(data->data, data->size);
    }
#endif

    LOGV("%s %d\n", __func__, __LINE__);

    header.data = cmd_buf->hdr.cmd;

    if (header.source == bk_ipc_cpu_id_get())
    {
        LOGE("%s error, should not recv the message form itself\n", __func__, header.source);
    }

    switch (header.type)
    {
        case IPC_TYPE_CMD:
        {
            if (data == NULL)
            {
                LOGE("%s, ipc data NULL\n", __func__);
                break;
            }

            handle = data->handle;

            if (handle == NULL)
            {
                LOGE("%s, ipc handle NULL\n", __func__);
                break;
            }

#ifdef IPC_SUPPORTED_CRC
            bk_ipc_crc_check(data, cmd_buf->param2);
#endif

            LOGV("%s got data from CPU %d\n", __func__, cmd_buf->hdr.cmd & 0x3);

            bk_ipc_data_push(&ipc_info->remote_list, data);
            bk_ipc_event_notify(ipc_info, IPC_EVENT_RECV);
        }
        break;

        case IPC_TYPE_ACK:
        {
#ifdef IPC_SUPPORTED_CRC
            bk_ipc_crc_check(data, cmd_buf->param2);
#endif

            if (data->flags & MIPC_CHAN_SEND_FLAG_SYNC)
            {
                LOGV("%s set sync sem\n", __func__);
                bk_err_t ret = rtos_set_semaphore(&data->sem);

                if (ret != BK_OK)
                {
                    LOGE("%s, set semaphore failed\n", __func__);
                }
            }
            else
            {
                bk_ipc_data_push(&ipc_info->free_list, data);
                bk_ipc_event_notify(ipc_info, IPC_EVENT_FREE);
            }
        }
        break;

        case IPC_TYPE_SYS:
        {
            LOGD("recv cpu%d state changed: %d\n", header.source, cmd_buf->param1);
            ipc_info->cpu_state[header.source] = cmd_buf->param1 & 0xFF;

            if (cmd_buf->param1 == IPC_CPU_STARTUP)
            {

            }
        }
        break;

        default:
        {

        }
        break;
    }
}


static void bk_ipc_mailbox_tx_isr(void *param)
{

}

static void bk_ipc_mailbox_tx_cmpl_isr(void *param, mb_chnl_ack_t *ack_buf)
{
    LOGV("%s %d\n", __func__, __LINE__);
    bk_ipc_info_t *ipc_info = (bk_ipc_info_t *)param;

    if (ipc_info)
    {
        rtos_set_semaphore(&ipc_info->sem);
    }
}

static void bk_ipc_slave_core_state_change(bk_ipc_info_t *ipc_info, uint32_t state)
{
    ipc_header_t header;
    bk_err_t ret = BK_FAIL;

    header.source = bk_ipc_cpu_id_get();
    header.type = IPC_TYPE_SYS;
    ret = bk_ipc_send_mailbox(ipc_info, &header, (void *)state);

    if (ret != BK_OK)
    {
        LOGE("%s failed\n", __func__);
    }
}

static void bk_ipc_thread_entry(beken_thread_arg_t param)
{
    bk_err_t ret = BK_OK;
    bk_ipc_data_t *data = NULL;
    bk_ipc_info_t *ipc_info = (bk_ipc_info_t *)param;

    ipc_info->thread_running = true;

    do
    {
        uint32_t bits = 0;
        ret = rtos_get_semaphore(ipc_info->waiting_sem, BEKEN_WAIT_FOREVER);
        if (ret != BK_OK)
        {
            LOGE("%s, rtos_get_semaphore fail\n", __func__);
            continue;
        }
        uint32_t flags  = bk_ipc_enter_critical();
        bits = ipc_info->waiting_event;
        ipc_info->waiting_event = 0;
        bk_ipc_exit_critical(flags);

        int loop_again = 0;

        do {
            loop_again = 0;

            if (bits & IPC_EVENT_FREE)
            {
                do
                {
                    data = bk_ipc_data_pop(&ipc_info->free_list);

                    if (data == NULL)
                    {
                        LOGV("%s free data error, should not be NULL\n", __func__);
                        break;
                    }

                    bk_ipc_handle_t *local_handle = bk_ipc_get_handle_by_name(&ipc_info->channel_list, data->handle->cfg->name);

                    if (local_handle && local_handle->cfg->tx_cb)
                    {
                        ipc_obj_t ipc_obj = (ipc_obj_t)data;
                        ret = local_handle->cfg->tx_cb(ipc_obj);
                    }
                    os_free(data);
                    data = NULL;

                }
                while (true);
            }

            if (bits & IPC_EVENT_RECV)
            {
                do
                {
                    data = bk_ipc_data_pop(&ipc_info->remote_list);

                    if (data == NULL)
                    {
                        break;
                    }
                    loop_again = 1;
                    bk_ipc_handle_t *local_handle = bk_ipc_get_handle_by_name(&ipc_info->channel_list, data->handle->cfg->name);

                    if (local_handle == NULL)
                    {
                        ipc_header_t header;

                        LOGE("%s, not register local channel: %s\n", __func__, data->handle->cfg->name);

                        header.source = bk_ipc_cpu_id_get();
                        header.type = IPC_TYPE_ACK;
                        data->result = BK_ERR_NO_DEV;

                        if (bk_ipc_send_mailbox(ipc_info, &header, data))
                        {
                            LOGE("%s, send ack failed\n", __func__);
                        }
                        break;
                    }

                    ipc_header_t header;

                    if (local_handle->cfg->rx_cb)
                    {
                        data->result = (uint32_t)ipc_info;
                        ipc_obj_t ipc_obj = (ipc_obj_t)data;
                        ret = local_handle->cfg->rx_cb(data->data, data->size, local_handle->cfg->param, ipc_obj);
                    }

                    if (data->flags & MIPC_CHAN_HAND_FLAG_ASYNC)
                    {
                        break;
                    }

                    data->result = ret;

                    header.source = bk_ipc_cpu_id_get();
                    header.type = IPC_TYPE_ACK;
                    ret = bk_ipc_send_mailbox(ipc_info, &header, data);

                    if (ret != BK_OK)
                    {
                        LOGE("%s send mailbox ack failed\n", __func__);
                    }
                }
                while (0);
            }

            if (bits & IPC_EVENT_SEND)
            {
                do
                {
                    ipc_header_t header;
                    data = bk_ipc_data_pop(&ipc_info->local_list);

                    if (data == NULL)
                    {
                        break;
                    }

                    loop_again = 1;

                    header.source = bk_ipc_cpu_id_get();
                    header.type = IPC_TYPE_CMD;
                    ret = bk_ipc_send_mailbox(ipc_info, &header, data);

                    if (ret != BK_OK)
                    {
                        LOGE("%s send async mailbox message to cpu %d failed\n", __func__, bk_ipc_cpu_id_get());
                    }
                }
                while (0);
            }

            if (bits & IPC_EVENT_LOCAL_SHUTDOWN)
            {
                bk_ipc_slave_core_state_change(ipc_info, IPC_CPU_SHUTDOWN);
            }
        } while(loop_again);


    } while (ipc_info->thread_running);

    ipc_info->thread = NULL;
    rtos_delete_thread(NULL);
}

static bk_err_t bk_ipc_cpu1_shutdown_callback(void *param)
{
    bk_ipc_info_t *ipc_info = (bk_ipc_info_t *)param;

    bk_ipc_event_notify(ipc_info, IPC_EVENT_LOCAL_SHUTDOWN);
    return 0;
}

bk_ipc_info_t *bk_ipc_core_init(uint8_t channel)
{
    bk_err_t ret = BK_OK;
    bk_ipc_info_t *ipc_info = NULL;

    LOGD("%s\n", __func__);

    ipc_info = (bk_ipc_info_t *)os_malloc(sizeof(bk_ipc_info_t));

    if (ipc_info == NULL)
    {
        LOGE("%s bk_ipc_info malloc failed\n", __func__);
        goto out;
    }

    os_memset(ipc_info, 0, sizeof(bk_ipc_info_t));

    ipc_info->state = IPC_MSTATE_IDLE;
    INIT_LIST_HEAD(&ipc_info->channel_list);
    INIT_LIST_HEAD(&ipc_info->local_list);
    INIT_LIST_HEAD(&ipc_info->remote_list);
    INIT_LIST_HEAD(&ipc_info->free_list);

    ret = rtos_init_semaphore_ex(&ipc_info->sem, 1, 1);

    if (ret != BK_OK)
    {
        LOGE("%s init semaphore failed 0x%x\n", __func__, ret);
        goto out;
    }

    ret = rtos_init_semaphore(&ipc_info->waiting_sem, 1);

    if (ret != BK_OK)
    {
        LOGE("%s init waiting_event failed\n", __func__);
        goto out;
    }

    for (int i = 0; i < IPC_CPU_COUNT; i++)
    {
        if (i == bk_ipc_cpu_id_get())
        {
            ipc_info->cpu_state[i] = IPC_CPU_STARTUP;
            continue;
        }

        ret = rtos_init_semaphore(&ipc_info->cpu_wait[i], 1);

        if (ret != BK_OK)
        {
            LOGE("%s init cpu_wait failed\n", __func__);
            break;
        }

        rtos_init_mutex(&ipc_info->wait_lock[i]);
    }

    rtos_init_mutex(&ipc_info->boot_lock);

    LOGD("open channel: %d on CPU: %d\n", channel, bk_ipc_cpu_id_get());
    mb_chnl_open(channel, ipc_info);
    mb_chnl_ctrl(channel, MB_CHNL_SET_RX_ISR, bk_ipc_mailbox_rx_isr);
    mb_chnl_ctrl(channel, MB_CHNL_SET_TX_ISR, bk_ipc_mailbox_tx_isr);
    mb_chnl_ctrl(channel, MB_CHNL_SET_TX_CMPL_ISR, bk_ipc_mailbox_tx_cmpl_isr);


    for (int i = 0; i < BK_IPC_CHANNEL_REGISTER_COUNT; i++)
    {
        bk_ipc_chan_cfg_t *cfg = BK_IPC_CHANNEL_REGISTER_GET(i);

        if (channel == MB_CHNL_MIPC_SYNC)
        {
            if (cfg->route != IPC_ROUTE_CPU0_CPU1)
            {
                continue;
            }
        }

        bk_ipc_handle_t *handle = (bk_ipc_handle_t *)os_malloc(sizeof(bk_ipc_handle_t));

        if (handle == NULL)
        {
            LOGE("%s malloc bk_ipc_handle_t failed\n", __func__);
            BK_ASSERT(0);
            break;
        }

        handle->cfg = cfg;
        handle->ipc = ipc_info;

        *cfg->ipc = handle;

        bk_ipc_list_insert(&ipc_info->channel_list, &handle->list);

        LOGD("channel: %s\n", cfg->name);
    }

    ret = rtos_create_thread(&ipc_info->thread,
                             4,
                             "ipc thread",
                             (beken_thread_function_t)bk_ipc_thread_entry,
                             CONFIG_MAILBOX_IPC_API_TASK_STACK_SIZE,
                             ipc_info);

    LOGD("%s success\n", __func__);

    return ipc_info;

out:

    if (ipc_info)
    {
        os_free(ipc_info);
        ipc_info = NULL;
    }

    return NULL;
}

int bk_ipc_init(void)
{
    bk_ipc_info = bk_ipc_core_init(MB_CHNL_MIPC_SYNC);

    stop_cpu1_register_notification(bk_ipc_cpu1_shutdown_callback, bk_ipc_info);
    bk_ipc_slave_core_state_change(bk_ipc_info, IPC_CPU_STARTUP);

// Modified by TUYA Start
// #if (BK_IPC_UT_TEST)
//     bk_ipc_test_init();
// #endif
// Modified by TUYA End

    return BK_OK;
}

int bk_ipc_deinit(void)
{
    bk_ipc_slave_core_state_change(bk_ipc_info, IPC_CPU_SHUTDOWN);
    return 0;
}

