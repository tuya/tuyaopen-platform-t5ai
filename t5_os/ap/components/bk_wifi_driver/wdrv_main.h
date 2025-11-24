#pragma once

#ifdef __cplusplus
extern "C" {
#endif


#include <common/bk_include.h>
//#include "pbuf.h"
//#include "wdrv_co_list.h"

#include <string.h>
#include <os/mem.h>
#include <os/os.h>
//#include <common/bk_include.h>
#include "os/str.h"
#include "wdrv_co_list.h"


#define WDRV_TAG "WDRV"
#define WDRV_LOGI(...)       BK_LOGI(WDRV_TAG, ##__VA_ARGS__)
#define WDRV_LOGW(...)       BK_LOGW(WDRV_TAG, ##__VA_ARGS__)
#define WDRV_LOGE(...)       BK_LOGE(WDRV_TAG, ##__VA_ARGS__)
#define WDRV_LOGD(...)       BK_LOGD(WDRV_TAG, ##__VA_ARGS__)
#define WDRV_LOGV(...)       BK_LOGV(WDRV_TAG, ##__VA_ARGS__)
#define WDRV_LOG_RAW(...)    BK_LOG_RAW(WDRV_TAG, ##__VA_ARGS__)

#define PTR_FROM_U32(type, addr) ((type *)(addr))
#define PTR_TO_U32(addr) ((uint32_t)(addr))

#define WDRV_QUEUE_LEN                          192
#define WDRV_TASK_PRIO                          2

#define CPDU_LENGTH sizeof(struct cpdu_t)
#define MAX_MSDU_LENGTH 1500+14//(MTU + Ethernet header)

#define WDRV_ALIGN_BYTES (4)
#if WDRV_ALIGN_BYTES
#define WDRV_ALIGN_LENGTH(x) ((x+(WDRV_ALIGN_BYTES-1))&(~(WDRV_ALIGN_BYTES-1)))
#else
#define WDRV_ALIGN_LENGTH(x) (x)
#endif

#define MAX_NUM_CMD_BUFFERS 3
#define MAX_NUM_CMD_RX_BANK 10

#define MAX_NUM_RX_BUFFERS (CONFIG_LWIP_MEM_MAX_RX_SIZE/MAX_RX_DATA_LENGTH)
#define INIT_NUM_RX_BUFFERS 10


#define MAX_TX_DATA_LENGTH WDRV_ALIGN_LENGTH(CPDU_LENGTH + MAX_MSDU_LENGTH)
#define MAX_RX_DATA_LENGTH WDRV_ALIGN_LENGTH(CPDU_LENGTH + MAX_MSDU_LENGTH)
#define MAX_CMD_BANK_LENGTH WDRV_ALIGN_LENGTH(CPDU_LENGTH + MAX_NUM_RX_BUFFERS*4 + 8)
#define MAX_CMD_BUF_LENGTH 800
#define MAX_CMD_BUF_PAYLOAD (MAX_CMD_BUF_LENGTH - sizeof(cpdu_t) - EVENT_HEAD_LEN - 16)


#define PATTERN_BUSY 0xCAFEBABE
#define PATTERN_FREE 0xF3EEF3EE
#define EVENT_HEAD_LEN 4


#define WDRV_STATS_INC(x,num) do{uint32_t int_level = 0;int_level = rtos_disable_int();wdrv_stats_ptr->x += num;rtos_enable_int(int_level);BK_ASSERT(wdrv_stats_ptr->x >= 0);}while(0)
#define WDRV_STATS_DEC(x) do{uint32_t int_level = 0;int_level = rtos_disable_int();--wdrv_stats_ptr->x;rtos_enable_int(int_level);BK_ASSERT(wdrv_stats_ptr->x >= 0);}while(0)
#define WDRV_STATS_RESET(x,num) do{uint32_t int_level = 0;int_level = rtos_disable_int();wdrv_stats_ptr->x = num;rtos_enable_int(int_level);BK_ASSERT(wdrv_stats_ptr->x >= 0);}while(0)


#define WDRV_IRQ_DISABLE(int_level) do { int_level = rtos_disable_int(); } while(0)
#define WDRV_IRQ_ENABLE(int_level)  do { rtos_enable_int(int_level); } while(0)

#define WDRV_ENTER_TXMSG_CRITICAL(int_level) do { int_level = wdrv_txmsg_enter_critical(); } while(0)
#define WDRV_EXIT_TXMSG_CRITICAL(int_level)  do { wdrv_txmsg_exit_critical(int_level); } while(0)

#if CONFIG_SOC_SMP
#include "spinlock.h"
static SPINLOCK_SECTION volatile spinlock_t wdrv_tx_msg_spin_lock = SPIN_LOCK_INIT;
#endif // CONFIG_SOC_SMP
static inline uint32_t wdrv_txmsg_enter_critical()
{
	uint32_t flags = rtos_disable_int();

#if CONFIG_SOC_SMP
	spin_lock(&wdrv_tx_msg_spin_lock);
#endif // CONFIG_SOC_SMP

	return flags;
}

static inline void wdrv_txmsg_exit_critical(uint32_t flags)
{
#if CONFIG_SOC_SMP
	spin_unlock(&wdrv_tx_msg_spin_lock);
#endif // CONFIG_SOC_SMP

	rtos_enable_int(flags);
}


enum wdrv_task_msg_evt
{
    WDRV_TASK_MSG_CMD = 0,
    WDRV_TASK_MSG_EVENT = 1,
    WDRV_TASK_MSG_TXDATA = 2,
    WDRV_TASK_MSG_RXDATA = 3,
};

struct wdrv_msg {
    uint32_t arg;
    uint16_t  type;
    uint16_t  retry_flag;
};

enum ipc_buffer_status
{
    BUFFER_STATUS_FREE_BANK = 0,
    BUFFER_STATUS_USED_BANK = 1,
    BUFFER_STATUS_MAX
};
enum buffer_direction
{
    BUFFER_TX = 0,
    BUFFER_RX = 1,
    BUFFER_MAX
};
enum cmd_buf_type
{
    CMD_BUF = 0,
    CMD_BANK = 1,
    CMD_BUF_MAX
};
enum data_path_special_type
{
    RX_FILTER_TYPE = 1,
    RX_MONITOR_TYPE = 2,
    SPECIAL_DATA_TYPE_MAX
};
struct common_header
{
    uint16_t length;//sdio whole buffer length(include common header)
    uint8_t type:4;
    uint8_t dst_index:4;//station index connected to bk softap
    uint8_t need_free:1;
    uint8_t is_buf_bank:1;
    uint8_t vif_idx:2;
    uint8_t special_type:3;
    uint8_t rsve:1;
};

typedef struct
{
    //Common Header
    struct cpdu_t* next;

    struct common_header co_hdr;

    //IPC ADDR bank
    uint16_t length;
    uint8_t  dir:1;//(0:TX,1:RX)
    uint8_t  mem_status:1;//(0:free,1:done)
    uint8_t  rsve:6;
    uint8_t  num;
    uint32   addr[1];
}ipc_addr_bank_t;

struct wdrv_rx_bank_debug_t
{
    uint32_t rx_buf_bank[30];
    uint16_t rx_buf_bank_cnt;
};

typedef struct cpdu_t
{
    struct cpdu_t* next;

    struct common_header co_hdr;
}cpdu_t;

struct bk_msg_hdr
{
    uint32_t rsv0;
    uint16_t cmd_id;
    uint16_t cmd_sn;
    uint16_t rsv1;
    uint16_t len;//msg payload length
};
/// Temporarily rx cmd structure
struct bk_rx_msg_hdr
{
    uint16_t id;                ///< Message id.
    uint16_t cfm_sn;
    uint16_t rsv;
    uint16_t param_len;         ///< Parameter embedded struct length.
    uint32_t pattern;           ///< Used to stamp a valid MSG buffer
    uint32_t param[1];        ///< Parameter embedded struct. Must be word-aligned.
};
struct ctrl_cmd_hdr
{
    struct cpdu_t* next;
    struct common_header co_hdr;

    struct bk_rx_msg_hdr msg_hdr;
};
struct bk_msg_common
{
    int8_t status;
};

typedef struct wdrv_stats
{
    uint16_t rx_alloc_num; //CP RX buffer numbers in use. Need CONFIG_CONTROLLER_RX_DIRECT_PSH = 0
    uint16_t rx_win;       // CONFIG_CONTROLLER_RX_DIRECT_PSH = 0
    uint16_t tx_alloc_num;
    uint16_t tx_list_num;

    uint32_t wdrv_tx_cnt;
    uint32_t wdrv_tx_process_cnt;
    uint32_t wdrv_txc_cnt;

    uint32_t wdrv_rx_cnt;
    uint32_t wdrv_rxc_cnt;

    uint32_t wdrv_rx_cpy_fail;
    uint32_t wdrv_tx_snder_fail;
    uint32_t wdrv_msg_snder_fail;

    uint32_t ipc_tx_cnt;
    uint32_t ipc_txc_cnt;
    uint32_t ipc_tx_fail_cnt;
}wdrv_stats;

struct wdrv_env_t
{
    void *io_queue;
    void *handle;
    uint32_t is_init;

    wdrv_stats stat;

    //For mem mgmt
    uint32_t cmd_addr[MAX_NUM_CMD_BUFFERS];
    uint32_t cmd_idx;
    uint32_t cmd_bank[MAX_NUM_CMD_RX_BANK];
    uint32_t cmd_bank_idx;
};


extern struct wdrv_env_t wdrv_env;
extern struct wdrv_stats * wdrv_stats_ptr;
//extern struct wdrv_rx_bank_debug_t wdrv_rxbank_debug;

extern bk_err_t wdrv_init();
void wdrv_free_cmd_buffer(uint8_t* buf);
uint8_t* wdrv_get_cmd_buffer(uint8_t type);
extern bk_err_t wdrv_txbuf_push(uint8_t channel,void* head,void *tail,uint8_t num);
void wdrv_attach_rx_buffer();
bk_err_t wdrv_msg_sender(uint32_t head,enum wdrv_task_msg_evt type,uint8_t retry);
void wdrv_rx_buffer_predict(uint8_t dir);
void wdrv_print_debug_info();
extern char __wifi_start;
extern char __wifi_end;
#ifdef __cplusplus
}
#endif
