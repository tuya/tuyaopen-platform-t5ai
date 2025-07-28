#ifndef __BK_SPP_API_H__
#define __BK_SPP_API_H__

#include "bk_dm_bluetooth_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BK_SPP_MAX_SCN                 31          /*!< SPP max SCN */
#define BK_SPP_MIN_TX_BUFFER_SIZE      100         /*!< SPP min tx buffer */
#define BK_SPP_MAX_TX_BUFFER_SIZE      (4096)  /*!< SPP max tx buffer size */


typedef enum
{
    BK_SPP_SUCCESS   = 0,          /*!< Successful operation. */
    BK_SPP_FAILURE,                /*!< Generic failure. */
    BK_SPP_BUSY,                   /*!< Temporarily can not handle this request. */
    BK_SPP_NO_DATA,                /*!< No data */
    BK_SPP_NO_RESOURCE,            /*!< No more resource */
    BK_SPP_NEED_INIT,              /*!< SPP module shall init first */
    BK_SPP_NEED_DEINIT,            /*!< SPP module shall deinit first */
    BK_SPP_NO_CONNECTION,          /*!< Connection may have been closed */
    BK_SPP_NO_SERVER,              /*!< No SPP server */
} bk_spp_status_t;

typedef enum
{
    BK_SPP_ROLE_MASTER     = 0,          /*!< Role: master */
    BK_SPP_ROLE_SLAVE      = 1,          /*!< Role: slave */
} bk_spp_role_t;


/**
 * @brief SPP callback function events
 */
typedef enum
{
    BK_SPP_INIT_EVT                    = 0,                /*!< When SPP is initialized, the event comes */
    BK_SPP_UNINIT_EVT                  = 1,                /*!< When SPP is deinitialized, the event comes */
    BK_SPP_DISCOVERY_COMP_EVT          = 8,                /*!< When SDP discovery complete, the event comes */
    BK_SPP_OPEN_EVT                    = 26,               /*!< When SPP Client connection open, the event comes */
    BK_SPP_CLOSE_EVT                   = 27,               /*!< When SPP connection closed, the event comes */
    BK_SPP_START_EVT                   = 28,               /*!< When SPP server started, the event comes */
    BK_SPP_CL_INIT_EVT                 = 29,               /*!< When SPP client initiated a connection, the event comes */
    BK_SPP_DATA_IND_EVT                = 30,               /*!< When SPP connection received data, the event comes, only for BK_SPP_MODE_CB */
    BK_SPP_WRITE_EVT                   = 33,               /*!< When SPP write operation completes, the event comes, only for BK_SPP_MODE_CB */
    BK_SPP_SRV_OPEN_EVT                = 34,               /*!< When SPP Server connection open, the event comes */
    BK_SPP_SRV_STOP_EVT                = 35,               /*!< When SPP server stopped, the event comes */
} bk_spp_cb_event_t;


/**
 * @brief SPP callback parameters union
 */
typedef union
{
    /**
     * @brief SPP_INIT_EVT
     */
    struct spp_init_evt_param
    {
        bk_spp_status_t    status;         /*!< status */
    } init;                                 /*!< SPP callback param of SPP_INIT_EVT */

    /**
     * @brief SPP_UNINIT_EVT
     */
    struct spp_uninit_evt_param
    {
        bk_spp_status_t    status;         /*!< status */
    } uninit;                               /*!< SPP callback param of SPP_UNINIT_EVT */

    /**
     * @brief SPP_DISCOVERY_COMP_EVT
     */
    struct spp_discovery_comp_evt_param
    {
        bk_spp_status_t status;                   /*!< status */
        uint8_t scn_num;                           /*!< The num of scn_num */
        uint8_t scn [ BK_SPP_MAX_SCN ];           /*!< channel # */
        const char *service_name [ BK_SPP_MAX_SCN ]; /*!< service_name */
    } disc_comp;                            /*!< SPP callback param of SPP_DISCOVERY_COMP_EVT */

    /**
     * @brief BK_SPP_OPEN_EVT
     */
    struct spp_open_evt_param
    {
        bk_spp_status_t    status;         /*!< status */
        uint32_t            handle;         /*!< The connection handle */
        bk_bd_addr_t         rem_bda;        /*!< The peer address */
    } open;                                 /*!< SPP callback param of BK_SPP_OPEN_EVT */

    /**
     * @brief BK_SPP_SRV_OPEN_EVT
     */
    struct spp_srv_open_evt_param
    {
        bk_spp_status_t    status;         /*!< status */
        uint32_t            handle;         /*!< The connection handle */
        bk_bd_addr_t         rem_bda;        /*!< The peer address */
    } srv_open;                             /*!< SPP callback param of BK_SPP_SRV_OPEN_EVT */
    /**
     * @brief BK_SPP_CLOSE_EVT
     */
    struct spp_close_evt_param
    {
        bk_spp_status_t    status;         /*!< status */
        uint32_t            handle;         /*!< The connection handle */
    } close;                                /*!< SPP callback param of BK_SPP_CLOSE_EVT */

    /**
     * @brief BK_SPP_START_EVT
     */
    struct spp_start_evt_param
    {
        bk_spp_status_t    status;         /*!< status */
        uint32_t            handle;         /*!< The connection handle */
        uint8_t             scn;            /*!< Server channel number */
    } start;                                /*!< SPP callback param of BK_SPP_START_EVT */

    /**
     * @brief BK_SPP_SRV_STOP_EVT
     */
    struct spp_srv_stop_evt_param
    {
        bk_spp_status_t    status;         /*!< status */
        uint8_t             scn;            /*!< Server channel number */
    } srv_stop;                             /*!< SPP callback param of BK_SPP_SRV_STOP_EVT */

    /**
     * @brief BK_SPP_CL_INIT_EVT
     */
    struct spp_cl_init_evt_param
    {
        bk_spp_status_t    status;         /*!< status */
        uint32_t            handle;         /*!< The connection handle */
    } cl_init;                              /*!< SPP callback param of BK_SPP_CL_INIT_EVT */

    /**
     * @brief BK_SPP_WRITE_EVT
     */
    struct spp_write_evt_param
    {
        bk_spp_status_t    status;         /*!< status */
        uint32_t            handle;         /*!< The connection handle */
        uint16_t                 len;            /*!< The length of the data written. */
    } write;                                /*!< SPP callback param of BK_SPP_WRITE_EVT */

    /**
     * @brief BK_SPP_DATA_IND_EVT
     */
    struct spp_data_ind_evt_param
    {
        bk_spp_status_t    status;         /*!< status */
        uint32_t            handle;         /*!< The connection handle */
        uint16_t            len;            /*!< The length of data */
        uint8_t             *data;          /*!< The data received */
    } data_ind;                             /*!< SPP callback param of BK_SPP_DATA_IND_EVT */
} bk_spp_cb_param_t;                       /*!< SPP callback parameter union type */

/**
 * @brief       SPP callback function type.
 *              When handle BK_SPP_DATA_IND_EVT, it is strongly recommended to cache incoming data, and process them in
 *              other lower priority application task rather than in this callback directly.
 *
 * @param       event:      Event type
 * @param       param:      Point to callback parameter, currently is union type
 */
typedef void (*bk_spp_cb_t)(bk_spp_cb_event_t event, bk_spp_cb_param_t *param);

/**
 * @brief       This function is called to init callbacks with SPP module.
 *
 * @param[in]   callback:   pointer to the init callback function.
 *
 * @return
 *              - BK_OK: success
 *              - other: failed
 */
bt_err_t bk_spp_register_callback(bk_spp_cb_t callback);

/**
 * @brief       This function is called to init SPP module.
 *              When the operation is completed, the callback function will be called with BK_SPP_INIT_EVT.
 *              This function should be called after bk_bluedroid_enable() completes successfully.
 *
 * @return
 *              - BK_OK: success
 *              - other: failed
 */
bt_err_t bk_spp_init();

/**
 * @brief       This function is called to uninit SPP module.
 *              The operation will close all active SPP connection first, then the callback function will be called
 *              with BK_SPP_CLOSE_EVT, and the number of BK_SPP_CLOSE_EVT is equal to the number of connection.
 *              When the operation is completed, the callback function will be called with BK_SPP_UNINIT_EVT.
 *              This function should be called after bk_spp_init()/bk_spp_enhanced_init() completes successfully.
 *
 * @return
 *              - BK_OK: success
 *              - other: failed
 */
bt_err_t bk_spp_deinit(void);


/**
 * @brief       This function is called to performs service discovery for the services provided by the given peer device.
 *              When the operation is completed, the callback function will be called with BK_SPP_DISCOVERY_COMP_EVT.
 *              This function must be called after bk_spp_init()/bk_spp_enhanced_init() successful and before bk_spp_deinit().
 *
 * @param[in]   bd_addr:   Remote device bluetooth device address.
 *
 * @return
 *              - BK_OK: success
 *              - other: failed
 */
bt_err_t bk_spp_start_discovery(bk_bd_addr_t bd_addr);

/**
 * @brief       This function makes an SPP connection to a remote BD Address.
 *              When the connection is initiated or failed to initiate, the callback is called with BK_SPP_CL_INIT_EVT.
 *              When the connection is established or failed, the callback is called with BK_SPP_OPEN_EVT.
 *              This function must be called after bk_spp_init()/bk_spp_enhanced_init() successful and before bk_spp_deinit().
 *
 * @param[in]   role:         Master or slave.
 * @param[in]   remote_scn:   Remote device bluetooth device SCN.
 * @param[in]   peer_bd_addr: Remote device bluetooth device address.
 *
 * @return
 *              - BK_OK: success
 *              - other: failed
 */
bt_err_t bk_spp_connect(bk_spp_role_t role, uint8_t remote_scn, uint8_t *peer_bd_addr);

/**
 * @brief       This function closes an SPP connection.
 *              When the operation is completed, the callback function will be called with BK_SPP_CLOSE_EVT.
 *              This function must be called after bk_spp_init()/bk_spp_enhanced_init() successful and before bk_spp_deinit().
 *
 * @param[in]   handle:    The connection handle.
 *
 * @return
 *              - BK_OK: success
 *              - other: failed
 */
bt_err_t bk_spp_disconnect(uint32_t handle);

/**
 * @brief       This function create a SPP server and starts listening for an
 *              SPP connection request from a remote Bluetooth device.
 *              When the server is started successfully, the callback is called with BK_SPP_START_EVT.
 *              When the connection is established, the callback is called with BK_SPP_SRV_OPEN_EVT.
 *              This function must be called after bk_spp_init()/bk_spp_enhanced_init() successful and before bk_spp_deinit().
 *
 * @param[in]   role:         Master or slave.
 * @param[in]   local_scn:    The specific channel you want to get.
 *                            If channel is 0, means get any channel.
 * @param[in]   name:         Server's name.
 *
 * @return
 *              - BK_OK: success
 *              - other: failed
 */
bt_err_t bk_spp_start_srv(bk_spp_role_t role, uint8_t local_scn, const char *name);

/**
 * @brief       This function stops all SPP servers.
 *              The operation will close all active SPP connection first, then the callback function will be called
 *              with BK_SPP_CLOSE_EVT, and the number of BK_SPP_CLOSE_EVT is equal to the number of connection.
 *              When the operation is completed, the callback is called with BK_SPP_SRV_STOP_EVT.
 *              This function must be called after bk_spp_init()/bk_spp_enhanced_init() successful and before bk_spp_deinit().
 *
 * @return
 *              - BK_OK: success
 *              - other: failed
 */

bt_err_t bk_spp_stop_srv(void);

/**
 * @brief       This function stops a specific SPP server.
 *              The operation will close all active SPP connection first on the specific SPP server, then the callback function will be called
 *              with BK_SPP_CLOSE_EVT, and the number of BK_SPP_CLOSE_EVT is equal to the number of connection.
 *              When the operation is completed, the callback is called with BK_SPP_SRV_STOP_EVT.
 *              This function must be called after bk_spp_init()/bk_spp_enhanced_init() successful and before bk_spp_deinit().
 *
 * @param[in]   scn:         Server channel number.
 *
 * @return
 *              - BK_OK: success
 *              - other: failed
 */
bt_err_t bk_spp_stop_srv_scn(uint8_t scn);

/**
 * @brief       This function is used to write data, only for BK_SPP_MODE_CB.
 *              When this function need to be called repeatedly, it is strongly recommended to call this function again after
 *              the previous event BK_SPP_WRITE_EVT is received and the parameter 'cong' is equal to false. If the previous event
 *              BK_SPP_WRITE_EVT with parameter 'cong' is equal to true, the function can only be called again when the event
 *              BK_SPP_CONG_EVT with parameter 'cong' equal to false is received.
 *              This function must be called after an connection between initiator and acceptor has been established.
 *
 * @param[in]   handle: The connection handle.
 * @param[in]   len:    The length of the data written.
 * @param[in]   p_data: The data written.
 *
 * @return
 *              - BK_OK: success
 *              - other: failed
 */
bt_err_t bk_spp_write(uint32_t handle, int len, uint8_t *p_data);

#ifdef __cplusplus
}
#endif

#endif ///__BK_SPP_API_H__
