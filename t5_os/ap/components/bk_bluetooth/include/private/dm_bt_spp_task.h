#ifndef __DM_BT_SPP_TASK_H__
#define __DM_BT_SPP_TASK_H__

#include "components/bluetooth/bk_dm_spp.h"
#include "dm_bluetooth_task.h"

enum
{
    BT_ETHERMIND_API_REQ_SUBMSG_SPP_INIT = BLUETOOTH_API_GROUP(BT_ETHERMIND_MSG_SPP_REQ),
    BT_ETHERMIND_API_REQ_SUBMSG_SPP_DEINIT,
    BT_ETHERMIND_API_REQ_SUBMSG_SPP_START_DISCOVERY,
    BT_ETHERMIND_API_REQ_SUBMSG_SPP_CONN,
    BT_ETHERMIND_API_REQ_SUBMSG_SPP_DISCONN,
    BT_ETHERMIND_API_REQ_SUBMSG_SPP_START_SRV,
    BT_ETHERMIND_API_REQ_SUBMSG_SPP_STOP_SRV,
    BT_ETHERMIND_API_REQ_SUBMSG_SPP_STOP_SRV_SCN,
    BT_ETHERMIND_API_REQ_SUBMSG_SPP_WIRTE,
};

typedef union
{
    struct sart_discovery_param
    {
        uint8_t remote_addr[6];
    } start_disc;
    struct connect_param
    {
        uint8_t sec_mask;
        uint8_t role;
        uint8_t remote_scn;
        uint8_t remote_addr[6];
    } conn;
    struct disconnect_param
    {
        uint32_t handle;
    } disconn;
    struct start_server_param
    {
        uint8_t sec_mask;
        uint8_t role;
        uint8_t local_scn;
        const char *name;
    } start_srv;
    struct stop_srv_scn_param
    {
        uint8_t scn;
    } stop_srv_scn;
    struct write_param
    {
        uint32_t handle;
        uint16_t len;
        uint8_t *p_data;
    } write;
} bt_ethermind_spp_msg_t;

void bt_spp_register_internall_callback(bk_spp_cb_t cb);

#endif