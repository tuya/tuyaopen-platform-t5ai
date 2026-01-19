// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#pragma once

#include "bk_dm_bluetooth_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief L2CAP operation success and failure codes
 */
typedef enum {
    BK_BT_L2CAP_SUCCESS   = 0,          /*!< Successful operation. */
    BK_BT_L2CAP_FAILURE,                /*!< Generic failure. */
    BK_BT_L2CAP_BUSY,                   /*!< Temporarily can not handle this request. */
    BK_BT_L2CAP_NO_RESOURCE,            /*!< No more resource */
    BK_BT_L2CAP_NEED_INIT,              /*!< L2CAP module shall init first */
    BK_BT_L2CAP_NEED_DEINIT,            /*!< L2CAP module shall deinit first */
    BK_BT_L2CAP_NO_CONNECTION,          /*!< Connection may have been closed */
    BK_BT_L2CAP_NO_SERVER,              /*!< No server */
} bk_bt_l2cap_status_t;

/**
 * @brief Security Setting Mask. Use these three mask mode:
 *        1. BK_BT_L2CAP_SEC_NONE
 *        2. BK_BT_L2CAP_SEC_AUTHENTICATE
 *        3. (BK_BT_L2CAP_SEC_ENCRYPT|BK_BT_L2CAP_SEC_AUTHENTICATE)
 */
#define BK_BT_L2CAP_SEC_NONE            0x0000    /*!< No security */
#define BK_BT_L2CAP_SEC_AUTHORIZE       0x0001    /*!< Authorization required */
#define BK_BT_L2CAP_SEC_AUTHENTICATE    0x0012    /*!< Authentication required */
#define BK_BT_L2CAP_SEC_ENCRYPT         0x0024    /*!< Encryption required */
typedef uint32_t bk_bt_l2cap_cntl_flags_t;

/**
 * @brief L2CAP callback function events
 */
typedef enum {
    BK_BT_L2CAP_INIT_EVT                 = 0,      /*!< When L2CAP is initialized, the event comes */
    BK_BT_L2CAP_UNINIT_EVT               = 1,      /*!< When L2CAP is deinitialized, the event comes */
    BK_BT_L2CAP_OPEN_EVT                 = 16,     /*!< When L2CAP Client connection open, the event comes */
    BK_BT_L2CAP_CLOSE_EVT                = 17,     /*!< When L2CAP connection closed, the event comes */
    BK_BT_L2CAP_START_EVT                = 18,     /*!< When L2CAP server started, the event comes */
    BK_BT_L2CAP_CL_INIT_EVT              = 19,     /*!< When L2CAP client initiated a connection, the event comes */
    BK_BT_L2CAP_SRV_STOP_EVT             = 36,     /*!< When L2CAP server stopped, the event comes */
} bk_bt_l2cap_cb_event_t;

/**
 * @brief L2CAP callback parameters union
 */
typedef union {
    /**
     * @brief BK_BT_L2CAP_INIT_EVT
     */
    struct l2cap_init_evt_param {
        bk_bt_l2cap_status_t  status;         /*!< status */
    } init;                                    /*!< L2CAP callback param of BK_BT_L2CAP_INIT_EVT */

    /**
     * @brief BK_BT_L2CAP_UNINIT_EVT
     */
    struct l2cap_uninit_evt_param {
        bk_bt_l2cap_status_t  status;         /*!< status */
    } uninit;                                  /*!< L2CAP callback param of BK_BT_L2CAP_UNINIT_EVT */

    /**
     * @brief BK_BT_L2CAP_OPEN_EVT
     */
    struct l2cap_open_evt_param {
        bk_bt_l2cap_status_t  status;         /*!< status */
        uint32_t               handle;         /*!< The connection handle */
        int                    fd;             /*!< File descriptor, lcid*/
        bk_bd_addr_t          rem_bda;        /*!< The peer address */
        int32_t                tx_mtu;         /*!< The transmit MTU */
    } open;                                    /*!< L2CAP callback param of BK_BT_L2CAP_OPEN_EVT */

    /**
     * @brief BK_BT_L2CAP_CLOSE_EVT
     */
    struct l2cap_close_evt_param {
        bk_bt_l2cap_status_t  status;         /*!< status */
        uint32_t               handle;         /*!< The connection handle */
        bool                   async;          /*!< FALSE, if local initiates disconnect */
        int                   fd;              /*!< File descriptor, lcid*/
    } close;                                   /*!< L2CAP callback param of BK_BT_L2CAP_CLOSE_EVT */

    /**
     * @brief BK_BT_L2CAP_START_EVT
     */
    struct l2cap_start_evt_param {
        bk_bt_l2cap_status_t  status;         /*!< status */
        uint32_t               handle;         /*!< The connection handle */
        uint8_t                sec_id;         /*!< security ID used by this server */
    } start;                                   /*!< L2CAP callback param of BK_BT_L2CAP_START_EVT */

    /**
     * @brief BK_BT_L2CAP_CL_INIT_EVT
     */
    struct l2cap_cl_init_evt_param {
        bk_bt_l2cap_status_t  status;         /*!< status */
        uint32_t               handle;         /*!< The connection handle */
        uint8_t                sec_id;         /*!< security ID used by this server */
    } cl_init;                                 /*!< L2CAP callback param of BK_BT_L2CAP_CL_INIT_EVT */

    /**
     * @brief BK_BT_L2CAP_SRV_STOP_EVT
     */
    struct l2cap_srv_stop_evt_param {
        bk_bt_l2cap_status_t  status;         /*!< status */
        uint16_t                psm;            /*!< local psm */
    } srv_stop;                                /*!< L2CAP callback param of BK_BT_L2CAP_SRV_STOP_EVT */

} bk_bt_l2cap_cb_param_t;

/**
 * @brief       L2CAP callback function type.
 *
 * @param       event:      Event type
 * @param       param:      Point to callback parameter, currently is union type
 */
typedef void (* bk_bt_l2cap_cb_t)(bk_bt_l2cap_cb_event_t event, bk_bt_l2cap_cb_param_t *param);

/**
 * @brief       This function is called to init callbacks with L2CAP module.
 *
 * @param[in]   callback:   pointer to the init callback function.
 *
 * @return
 *              - BT_OK: success
 *              - other: failed
 */
bt_err_t bk_bt_l2cap_register_callback(bk_bt_l2cap_cb_t callback);

/**
 * @brief       This function is called to init L2CAP module.
 *              When the operation is completed, the callback function will be called with BK_BT_L2CAP_INIT_EVT.
 *              This function should be called after bk_bluedroid_enable() completes successfully.
 *
 * @return
 *              - BT_OK: success
 *              - other: failed
 */
bt_err_t bk_bt_l2cap_init(void);

/**
 * @brief       This function is called to uninit l2cap module.
 *              The operation will close all active L2CAP connection first, then the callback function will be called
 *              with BK_BT_L2CAP_CLOSE_EVT, and the number of BK_BT_L2CAP_CLOSE_EVT is equal to the number of connection.
 *              When the operation is completed, the callback function will be called with BK_BT_L2CAP_UNINIT_EVT.
 *              This function should be called after bk_bt_l2cap_init() completes successfully.
 *
 * @return
 *              - BT_OK: success
 *              - other: failed
 */
bt_err_t bk_bt_l2cap_deinit(void);

/**
 * @brief       This function makes an L2CAP connection to a remote BD Address.
 *              When the connection is initiated or failed to initiate, the callback is called with BK_BT_L2CAP_CL_INIT_EVT.
 *              When the connection is established or failed, the callback is called with BK_BT_L2CAP_OPEN_EVT.
 *              This function must be called after bk_bt_l2cap_init() successful and before bk_bt_l2cap_deinit().
 *
 * @param[in]   cntl_flag:    Lower 16-bit security settings mask.
 * @param[in]   remote_psm:   Remote device bluetooth Profile PSM.
 * @param[in]   peer_bd_addr: Remote device bluetooth device address.
 *
 * @return
 *              - BT_OK: success
 *              - other: failed
 */
bt_err_t bk_bt_l2cap_connect(bk_bt_l2cap_cntl_flags_t cntl_flag, uint16_t remote_psm, bk_bd_addr_t peer_bd_addr);

/**
 * @brief       This function create a L2CAP server and starts listening for an
 *              L2CAP connection request from a remote Bluetooth device.
 *              When the server is started successfully, the callback is called with BK_BT_L2CAP_START_EVT.
 *              When the connection is established, the callback is called with BK_BT_L2CAP_OPEN_EVT.
 *              This function must be called after bk_bt_l2cap_init() successful and before bk_bt_l2cap_deinit().
 *
 * @param[in]   cntl_flag:    Lower 16-bit security settings mask.
 * @param[in]   local_psm:    Dynamic PSM.
 *
 * @return
 *              - BT_OK: success
 *              - other: failed
 */
bt_err_t bk_bt_l2cap_start_srv(bk_bt_l2cap_cntl_flags_t cntl_flag, uint16_t local_psm);

/**
 * @brief       This function stops all L2CAP servers.
 *              The operation will close all active L2CAP connection first, then the callback function will be called
 *              with BK_BT_L2CAP_CLOSE_EVT, and the number of BK_BT_L2CAP_CLOSE_EVT is equal to the number of connection.
 *              When the operation is completed, the callback is called with BK_BT_L2CAP_SRV_STOP_EVT.
 *              This function must be called after bk_bt_l2cap_init() successful and before bk_bt_l2cap_deinit().
 *
 * @return
 *              - BT_OK: success
 *              - other: failed
 */

bt_err_t bk_bt_l2cap_stop_all_srv(void);

/**
 * @brief       This function stops a specific L2CAP server.
 *              The operation will close all active L2CAP connection first on the specific L2CAP server, then the callback function will
 *              be called with BK_BT_L2CAP_CLOSE_EVT, and the number of BK_BT_L2CAP_CLOSE_EVT is equal to the number of connection.
 *              When the operation is completed, the callback is called with BK_BT_L2CAP_SRV_STOP_EVT.
 *              This function must be called after bk_bt_l2cap_init() successful and before bk_bt_l2cap_deinit().
 *
 * @param[in]   local_psm:    Dynamic PSM.
 *
 * @return
 *              - BT_OK: success
 *              - other: failed
 */
bt_err_t bk_bt_l2cap_stop_srv(uint16_t local_psm);


/**
 * @brief       L2CAP callback function type.
 *
 * @param       fd:         File descriptor.
 * @param       data:       pointer to data.
 * @param       len:        data length
 */
typedef void (* bk_bt_l2cap_data_cb_t)(int fd, void *data, uint16_t size);

int bk_bt_l2cap_read(int fd, void *data, uint16_t size);
int bk_bt_l2cap_write(int fd, const void *data, uint16_t size);
bk_err_t bk_bt_l2cap_close(int fd);

bk_err_t bk_bt_l2cap_set_data_callback(bk_bt_l2cap_data_cb_t callback);



#ifdef __cplusplus
}
#endif
