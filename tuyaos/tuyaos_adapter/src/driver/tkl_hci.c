#include <stdbool.h>
#include "tkl_hci.h"
#include "tkl_ipc.h"
#include <os/mem.h>
// #include "bt_ipc_core.h"
// #include "hci_parse.h"
/***********************************************************
 *************************micro define***********************
 ***********************************************************/
#define TKL_DEBUG 0

extern VOID tkl_data_dump(const int level,
        const char *file, const int32_t line, const char *title,
        uint8_t width, uint8_t *buf, uint16_t size);

 /***********************************************************
 *************************variable define********************
 ***********************************************************/
static TKL_HCI_FUNC_CB s_evt_cb = NULL;
static TKL_HCI_FUNC_CB s_acl_cb = NULL;
extern void bk_printf(const char *fmt, ...);

#if 1
typedef enum
{
    DATA_TYPE_COMMAND = 1,
    DATA_TYPE_ACL     = 2,
    DATA_TYPE_SCO     = 3,
    DATA_TYPE_EVENT   = 4,
} serial_data_type_t;

typedef struct __attribute__((packed))
{
    uint16_t opcode;
    uint8_t param_len;
    uint8_t param[];
}cmd_hdr_t;

typedef struct __attribute__((packed))
{
    uint8_t event_code;
    uint8_t param_len;
    uint8_t param[];
}event_hdr_t;

typedef struct __attribute__((packed))
{
    uint16_t hdl_flags;
    uint16_t datalen;
    uint8_t param[];
}acl_hdr_t;

enum
{
    HCI_COMMAND_PKT = 0x1,//A core
    HCI_ACL_DATA_PKT = 0x2,
    HCI_SCO_DATA_PKT = 0x3,
    HCI_EVENT_PKT = 0x4, //M core
    HCI_FREE_PKT = 0xa,
};

typedef union
{
	struct
	{
		uint32_t		cmd           :  8;
		uint32_t		state         :  4;
		uint32_t		Reserved      :  20;	/* reserved for system. */
	} ;
	uint32_t		data;
} mb_chnl_hdr_t;

typedef struct
{
	mb_chnl_hdr_t	hdr;

	uint32_t		param1;
	uint32_t		param2;
	uint32_t		param3;
} mb_chnl_cmd_t;

typedef struct __attribute__((packed))
{
    mb_chnl_hdr_t hdr;
    uint8_t pkt_type;
#if 0
    union
    {
        cmd_hdr_t *cmd_hdr;
        event_hdr_t *event_hdr;
    };
#else
    uint32_t hdr_ptr;
#endif
} hci_hdr_t;

typedef void (*bt_hci_send_cb_t)(uint8_t *buf, uint16_t len);
typedef int (*ble_hci_to_cp_cb)(uint8_t *buf, uint16_t len);
extern void bt_ipc_hci_send_cmd(uint16_t opcode, uint8_t *data, uint16_t len);
extern void bt_ipc_hci_send_acl_data(uint16_t hdl_flags, uint8_t *data, uint16_t len);
extern void bt_ipc_register_hci_send_callback(bt_hci_send_cb_t cb);
extern int bk_bluetooth_init(void);
extern int bk_bluetooth_deinit(void);
extern int bk_ble_host_register_hci_callback(ble_hci_to_cp_cb cb);
// {
//     hci_hdr_t msg;

//     uint16_t data_len = sizeof(cmd_hdr_t) + len;
//     cmd_hdr_t *cmd_hdr = (cmd_hdr_t *)os_malloc(data_len);

//     //LOGD("malloc ptr %p\n",cmd_hdr);

//     if (cmd_hdr == NULL)
//     {
//         bk_printf("%s, malloc failed\r\n", __func__);
//         return;
//     }
//     cmd_hdr->opcode = opcode;
//     cmd_hdr->param_len = len;
//     os_memcpy(cmd_hdr->param, data, len);

//     msg.pkt_type = HCI_COMMAND_PKT;
//     msg.hdr_ptr = (uint32_t)(uintptr_t)cmd_hdr;

//     bt_ipc_mailbox_send_msg(&msg);
// }


#endif



BOOL_T ble_init_flag = FALSE;

static int ble_hci_data_send_to_controller(uint8_t *buf, uint16_t len)
{
    uint8_t type = buf[0];

    switch (type)
    {
        case DATA_TYPE_COMMAND:
        {
            cmd_hdr_t *cmd_hdr = (cmd_hdr_t *)(buf + 1);
            bt_ipc_hci_send_cmd(cmd_hdr->opcode, cmd_hdr->param, cmd_hdr->param_len);
        }
        break;

        case DATA_TYPE_ACL:
        {
            acl_hdr_t *acl_hdr = (acl_hdr_t *)(buf + 1);
            bt_ipc_hci_send_acl_data(acl_hdr->hdl_flags, acl_hdr->param, acl_hdr->datalen);
        }
        break;

        default:
            bk_printf("unknown type (%d)", type);
            break;
    }
    return 0;
}

static void hal_hci_driver_send_to_host(uint8_t *buf, uint16_t len)
{
#if CONFIG_BT
    hal_hci_driver_send_to_host_ext(MULTI_CONTROLLER_VOTE_PRI, buf, len);
#else
    // bk_ble_hci_send_to_host(buf, len);
    uint8_t type = buf[0];

    // bk_printf("%s\n", __func__);
    // bk_printf("====================>\n");
    // tkl_data_dump(0, __FILE__,  __LINE__, "hci data", 64, (uint8_t *)buf, len);
    // bk_printf("<====================\n");

    switch (type)
    {
        case DATA_TYPE_EVENT:
        {
            if (s_evt_cb) {
                s_evt_cb((uint8_t *)(buf + 1), len);
            }
        }
        break;

        case DATA_TYPE_ACL:
        {
            if (s_acl_cb) {
                s_acl_cb((uint8_t *)(buf + 1), len);
            }
        }
        break;

        default:
            bk_printf("unknown type (%d)", type);
            break;
    }
#endif
}

OPERATE_RET tkl_hci_init(VOID)
{
    int ret;

#if TKL_DEBUG == 1
    bk_printf("trace cpu%d %s %d\n", CONFIG_CPU_INDEX, __func__, __LINE__);
#endif

    if (ble_init_flag == TRUE) {
        return OPRT_OK;
    }

    bk_bluetooth_init();


// #if CONFIG_BT
//     ret = bk_dual_host_register_hci_callback(dual_hci_data_to_cp_cb);
// #else
    // ret = bk_ble_host_register_hci_callback(ble_hci_data_send_to_controller);
// #endif

    bt_ipc_register_hci_send_callback(hal_hci_driver_send_to_host);

    ble_init_flag = TRUE;
    return OPRT_OK;
}

OPERATE_RET tkl_hci_deinit(VOID)
{
#if TKL_DEBUG == 1
    bk_printf("trace cpu%d %s %d\n", CONFIG_CPU_INDEX, __func__, __LINE__);
#endif

    int ret = OPRT_OK;

    if (ble_init_flag == FALSE) {
        bk_printf("ble not open \r\n");
        return OPRT_OK;
    }
    bt_ipc_register_hci_send_callback(NULL);
#if CONFIG_BT
    ret = bk_dual_host_register_hci_callback(NULL);
#else
    // ret = bk_ble_host_register_hci_callback(NULL);
#endif

    bk_bluetooth_deinit();

    ble_init_flag = FALSE;
    return ret;
}

OPERATE_RET tkl_hci_reset(VOID)
{
#if TKL_DEBUG == 1
    bk_printf("trace cpu%d %s %d\n", CONFIG_CPU_INDEX, __func__, __LINE__);
#endif


    return OPRT_OK;
}

OPERATE_RET tkl_hci_cmd_packet_send(const uint8_t *p_buf, uint16_t buf_len)
{
#if TKL_DEBUG == 1
    bk_printf("trace cpu%d %s %d\n", CONFIG_CPU_INDEX, __func__, __LINE__);
#endif

#if TKL_DEBUG >= 5
    bk_printf("111.%s op 0x%04X\n", __func__, (uint16_t)((((uint16_t)p_buf[1]) << 8) | p_buf[0]));
    bk_printf("====================>\n");
    // tkl_data_dump(0, __FILE__,  __LINE__, "hci data", 64, (uint8_t *)p_buf, buf_len);
    // bk_printf("<====================\n");
#elif TKL_DEBUG == 1
    bk_printf("trace cpu%d %s %d\n", CONFIG_CPU_INDEX, __func__, __LINE__);
#endif

    if (ble_init_flag == FALSE) {
        bk_printf("ble not open \r\n");
        return OPRT_OK;
    }

    cmd_hdr_t *cmd_hdr = (cmd_hdr_t *)(p_buf);
    bt_ipc_hci_send_cmd(cmd_hdr->opcode, cmd_hdr->param, cmd_hdr->param_len);

    return OPRT_OK;
}

OPERATE_RET tkl_hci_acl_packet_send(const uint8_t *p_buf, uint16_t buf_len)
{
#if TKL_DEBUG == 1
    bk_printf("trace cpu%d %s %d\n", CONFIG_CPU_INDEX, __func__, __LINE__);
#endif

#if TKL_DEBUG  >= 5
    bk_printf("%s handle 0x%04X\n", __func__, (uint16_t)((((uint16_t)p_buf[1]) << 8) | p_buf[0]));
    bk_printf("====================>\n");
    tkl_data_dump(0, __FILE__,  __LINE__, "hci data", 64, (uint8_t *)p_buf, buf_len);
    bk_printf("<====================\n");
#elif TKL_DEBUG == 1
    bk_printf("trace cpu%d %s %d\n", CONFIG_CPU_INDEX, __func__, __LINE__);
#endif
    if (ble_init_flag == FALSE) {
        bk_printf("ble not open \r\n");
        return OPRT_OK;
    }

    acl_hdr_t *acl_hdr = (acl_hdr_t *)(p_buf);
    bt_ipc_hci_send_acl_data(acl_hdr->hdl_flags, acl_hdr->param, acl_hdr->datalen);

    return OPRT_OK;
}


OPERATE_RET tkl_hci_callback_register(const TKL_HCI_FUNC_CB hci_evt_cb, const TKL_HCI_FUNC_CB acl_pkt_cb)
{
#if TKL_DEBUG == 1
    bk_printf("trace cpu%d %s %d\n", CONFIG_CPU_INDEX, __func__, __LINE__);
#endif

    s_evt_cb = hci_evt_cb;
    s_acl_cb = acl_pkt_cb;

    return OPRT_OK;
}

