#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "bk_dm_gap_ble_types.h"

#ifdef __cplusplus
extern "C" {
#endif



/**
 * @defgroup dm_gap_v1 GAP API
 * @{
 */


/**
 * @defgroup dm_gap_defines_v1 Defines
 * @{
 */


/**
 * @brief           This function is called to occur gap event, such as scan result
 *
 * @param[in]       callback: callback function
 *
 * @return
 *                  - BK_OK : success
 *                  - other  : failed
 *
 */
ble_err_t bk_ble_gap_register_callback(bk_ble_gap_cb_t callback);

/**
 * @brief           Update connection parameters, can only be used when connection is up.
 *
 * @param[in]       params   -  connection update parameters
 *
 * @return
 *                  - BK_OK : success
 *                  - other  : failed
 *
 */
ble_err_t bk_ble_gap_update_conn_params(bk_ble_conn_update_params_t *params);


/**
 * @brief           This function is to set maximum LE data packet size
 *
 * @param[in]       remote_device   -  remote device
 * @param[in]       tx_data_length   -  data len
 *
 * @return
 *                  - BK_OK : success
 *                  - other  : failed
 *
 */
ble_err_t bk_ble_gap_set_pkt_data_len(bk_bd_addr_t remote_device, uint16_t tx_data_length);

/**
 * @brief           This function sets the static Random Address and Non-Resolvable Private Address for the application
 *
 * @param[in]       rand_addr: the random address which should be setting
 *
 * @return
 *                  - BK_OK : success
 *                  - other  : failed
 *
 */
ble_err_t bk_ble_gap_set_rand_addr(bk_bd_addr_t rand_addr);

#if BEKEN_API_NEED
/**
 * @brief           This function clears the random address for the application
 *
 * @return
 *                  - BK_OK : success
 *                  - other  : failed
 *
 */
ble_err_t bk_ble_gap_clear_rand_addr(void);
#endif


/**
 * @brief           Enable/disable privacy on the local device
 *
 * @param[in]       privacy_enable   - enable/disable privacy on remote device.
 *
 * @return
 *                  - BK_OK : success
 *                  - other  : failed
 *
 */
ble_err_t bk_ble_gap_config_local_privacy(bool privacy_enable);

/**
 * @brief           set local gap appearance icon
 *
 *
 * @param[in]       icon   - External appearance value, these values are defined by the Bluetooth SIG, please refer to
 *                  https://specificationrefs.bluetooth.com/assigned-values/Appearance%20Values.pdf
 *
 * @return
 *                  - BK_OK : success
 *                  - other  : failed
 *
 */
ble_err_t bk_ble_gap_config_local_icon(uint16_t icon);

/**
* @brief            Add or remove device from white list
*
* @param[in]        add_remove: the value is true if added the ble device to the white list, and false remove to the white list.
* @param[in]        remote_bda: the remote device address add/remove from the white list.
* @param[in]        wl_addr_type: whitelist address type
* @return
*                     - BK_OK : success
*                     - other  : failed
*
*/
ble_err_t bk_ble_gap_update_whitelist(bool add_remove, bk_bd_addr_t remote_bda, bk_ble_wl_addr_type_t wl_addr_type);

/**
* @brief            Clear all white list
*
* @return
*                     - BK_OK : success
*                     - other  : failed
*
*/
ble_err_t bk_ble_gap_clear_whitelist(void);

/**
* @brief            Get the whitelist size in the controller
*
* @param[out]       length: the white list length.
* @return
*                     - BK_OK : success
*                     - other  : failed
*
*/
ble_err_t bk_ble_gap_get_whitelist_size(uint16_t *length);
#if (BLE_42_FEATURE_SUPPORT == TRUE)
#if BEKEN_API_NEED
/**
* @brief            This function is called to set the preferred connection
*                   parameters when default connection parameter is not desired before connecting.
*                   This API can only be used in the master role.
*
* @param[in]        bd_addr: BD address of the peripheral
* @param[in]        min_conn_int: minimum preferred connection interval
* @param[in]        max_conn_int: maximum preferred connection interval
* @param[in]        slave_latency: preferred slave latency
* @param[in]        supervision_tout: preferred supervision timeout
*
* @return
*                   - BK_OK : success
*                   - other  : failed
*
*/
ble_err_t bk_ble_gap_set_prefer_conn_params(bk_bd_addr_t bd_addr,
                                            uint16_t min_conn_int, uint16_t max_conn_int,
                                            uint16_t slave_latency, uint16_t supervision_tout);
#endif
#endif // #if (BLE_42_FEATURE_SUPPORT == TRUE)
/**
 * @brief           Set device name to the local device
 *
 * @param[in]       name   -  device name.
 *
 * @return
 *                  - BK_OK : success
 *                  - other  : failed
 *
 */
ble_err_t bk_ble_gap_set_device_name(const char *name);


/**
 * @brief           Get device name of the local device
 *
 * @param[out]       name   -  device name.
 * @param[inout]       size   -  name output max size, and real size after return.
 *
 * @return
 *                  - BK_OK : success
 *                  - other  : failed
 *
 */
ble_err_t bk_ble_gap_get_device_name(char *name, uint32_t *size);

/**
 * @brief          This function is called to get local used address and address type.
 *                 uint8_t *bk_bt_dev_get_address(void) get the public address
 *
 * @param[in]       local_used_addr - current local used ble address (six bytes)
 * @param[in]       addr_type   - ble address type
 *
 * @return          - BK_OK : success
 *                  - other  : failed
 *
 */
ble_err_t bk_ble_gap_get_local_used_addr(bk_bd_addr_t local_used_addr, uint8_t *addr_type);

#if BEKEN_API_NEED
/**
 * @brief          This function is called to get ADV data for a specific type.
 *
 * @param[in]       adv_data - pointer of ADV data which to be resolved
 * @param[in]       type   - finding ADV data type
 * @param[out]      length - return the length of ADV data not including type
 *
 * @return          pointer of ADV data
 *
 */
uint8_t *bk_ble_resolve_adv_data(uint8_t *adv_data, uint8_t type, uint8_t *length);
#endif

/**
 * @brief           This function is called to read the RSSI of remote device.
 *                  The address of link policy results are returned in the gap callback function with
 *                  BK_GAP_BLE_READ_RSSI_COMPLETE_EVT event.
 *
 * @param[in]       remote_addr : The remote connection device address.
 *
 * @return
 *                  - BK_OK : success
 *                  - other  : failed
 */
ble_err_t bk_ble_gap_read_rssi(bk_bd_addr_t remote_addr);

#if (SMP_INCLUDED == TRUE)
/**
* @brief             Set a GAP security parameter value. Overrides the default value.
*
*                    Secure connection is highly recommended to avoid some major
*                    vulnerabilities like 'Impersonation in the Pin Pairing Protocol'
*                    (CVE-2020-26555) and 'Authentication of the LE Legacy Pairing
*                    Protocol'.
*
*                    To accept only `secure connection mode`, it is necessary do as following:
*
*                    1. Set bit `BK_LE_AUTH_REQ_SC_ONLY` (`param_type` is
*                    `BK_BLE_SM_AUTHEN_REQ_MODE`), bit `BK_LE_AUTH_BOND` and bit
*                    `BK_LE_AUTH_REQ_MITM` is optional as required.
*
*                    2. Set to `BK_BLE_ONLY_ACCEPT_SPECIFIED_AUTH_ENABLE` (`param_type` is
*                    `BK_BLE_SM_ONLY_ACCEPT_SPECIFIED_SEC_AUTH`).
*
*                    3. Set to `bk_ble_key_distr_mask_t` (`param_type` is
*                    `BK_BLE_SM_SET_INIT_KEY` or `BK_BLE_SM_SET_RSP_KEY`).
*
* @param[in]       param_type : the type of the param which to be set
* @param[in]       value  : the param value
* @param[in]       len : the length of the param value
*
* @return            - BK_OK : success
*                       - other  : failed
*
*/
ble_err_t bk_ble_gap_set_security_param(bk_ble_sm_param_t param_type, void *value, uint8_t len);


/**
* @brief             Grant security request access.
*
* @param[in]       bd_addr : BD address of the peer
* @param[in]       accept  :  accept the security request or not
*
* @return            - BK_OK : success
*                    - other  : failed
*
*/
ble_err_t bk_ble_gap_security_rsp(bk_bd_addr_t bd_addr,  bool accept);


/**
* @brief             Set a gap parameter value. Use this function to change
*                    the default GAP parameter values.
*
* @param[in]       bd_addr : the address of the peer device need to encryption
* @param[in]       sec_act  : This is the security action to indicate
*                                   what kind of BLE security level is required for
*                                   the BLE link if the BLE is supported
*
* @return            - BK_OK : success
*                       - other  : failed
*
*/
ble_err_t bk_ble_set_encryption(bk_bd_addr_t bd_addr, bk_ble_sec_act_t sec_act);

/**
* @brief          Reply the key value to the peer device in the legacy connection stage.
*
* @param[in]      bd_addr : BD address of the peer
* @param[in]      accept : passkey entry successful or declined.
* @param[in]      passkey : passkey value, must be a 6 digit number,
*                                     can be lead by 0.
*
* @return            - BK_OK : success
*                  - other  : failed
*
*/
ble_err_t bk_ble_passkey_reply(bk_bd_addr_t bd_addr, bool accept, uint32_t passkey);


/**
* @brief           Reply the confirm value to the peer device in the secure connection stage.
*
* @param[in]       bd_addr : BD address of the peer device
* @param[in]       accept : numbers to compare are the same or different.
*
* @return            - BK_OK : success
*                       - other  : failed
*
*/
ble_err_t bk_ble_confirm_reply(bk_bd_addr_t bd_addr, bool accept);


/**
* @brief           Reply the save key.
*
* @param[in]       bd_addr : BD address of the peer device
* @param[in]       accept : true or false
* @param[in]       type : key type.
* @param[in]       data : key data, when type is:
*                         BK_LE_KEY_LENC, data is bk_ble_lenc_keys_t.
*                         When accept is false, data and len will be ignore.
* @param[in]       len : key len
*
* @return            - BK_OK : success
*                       - other  : failed
*
*/
ble_err_t bk_ble_pair_key_reply(bk_bd_addr_t bd_addr, bool accept, bk_ble_key_type_t type, void *data, uint32_t len);

/**
* @brief           Removes a device from the security database list of
*                  peer device. It manages unpairing event while connected.
*
* @param[in]       bd_addr : BD address of the peer device
*
* @return            - BK_OK : success
*                       - other  : failed
*
*/
ble_err_t bk_ble_remove_bond_device(bk_bd_addr_t bd_addr);

/**
* @brief           Get the device number from the security database list of peer device.
*                  It will return the device bonded number immediately.
*
* @return          - >= 0 : bonded devices number.
*                  - BK_FAIL  : failed
*
*/
int bk_ble_get_bond_device_num(void);


/**
* @brief           Get the device from the security database list of peer device.
*                  It will return the device bonded information immediately.
* @param[inout]    dev_num: Indicate the dev_list array(buffer) size as input.
*                           If dev_num is large enough, it means the actual number as output.
*                           Suggest that dev_num value equal to bk_ble_get_bond_device_num().
*
* @param[out]      dev_list: an array(buffer) of `bk_ble_bond_dev_t` type. Use for storing the bonded devices address.
*                            The dev_list should be allocated by who call this API.
* @return          - BK_OK : success
*                  - other  : failed
*
*/
ble_err_t bk_ble_get_bond_device_list(int *dev_num, bk_ble_bond_dev_t *dev_list);

#if BEKEN_API_NEED
/**
* @brief           This function is called to provide the OOB data for
*                  SMP in response to BK_GAP_BLE_OOB_REQ_EVT
*
* @param[in]       bd_addr: BD address of the peer device.
* @param[in]       TK: Temporary Key value, the TK value shall be a 128-bit random number
* @param[in]       len: length of temporary key, should always be 128-bit
*
* @return          - BK_OK : success
*                  - other  : failed
*
*/
ble_err_t bk_ble_oob_req_reply(bk_bd_addr_t bd_addr, uint8_t *TK, uint8_t len);
#endif
#endif /* #if (SMP_INCLUDED == TRUE) */

/**
* @brief           This function is to connect the physical connection of the peer device
*
*
*
* @param[in]       param : connect param
*
* @attention       1. local_addr_type can be BLE_ADDR_TYPE_PUBLIC ~ BLE_ADDR_TYPE_RPA_RANDOM.
* @attention       2. if local_addr_type is BLE_ADDR_TYPE_PUBLIC, and initiator_filter_policy policy not enable, host will use local public addr/peer_addr/peer_addr_type to connect.
* @attention       3. if local_addr_type is BLE_ADDR_TYPE_RANDOM, and initiator_filter_policy policy not enable, host will use peer_addr/peer_addr_type, random addr that set in bk_ble_gap_set_adv_rand_addr to connect.
* @attention       4. if local_addr_type is BLE_ADDR_TYPE_RPA_PUBLIC, host will try search bond list's nominal addr by peer_addr/peer_addr_type, if found and BK_LE_KEY_PID | BK_LE_KEY_LID
*                     present then host will generate RPA addr to connect peer RPA ADV, otherwise see attention 2.
* @attention       5. if local_addr_type is BLE_ADDR_TYPE_RPA_RANDOM, host will try search bond list's nominal addr by peer_addr/peer_addr_type, if found and BK_LE_KEY_PID | BK_LE_KEY_LID
*                     present then host will generate RPA addr to connect peer RPA ADV, otherwise see attention 3.
*
* @return            - BK_OK : success
*                    - other  : failed
*
*/
ble_err_t bk_ble_gap_connect(bk_gap_create_conn_params_t *param);

/**
* @brief           This function is to disconnect the physical connection of the peer device
*
*
*
* @param[in]       remote_device : BD address of the peer device
*
* @return            - BK_OK : success
*                    - other  : failed
*
*/
ble_err_t bk_ble_gap_disconnect(bk_bd_addr_t remote_device);

/**
* @brief           This function is to cancel the physical connection that have not connect completed
*
* @return            - BK_OK : success
*                    - other  : failed
*
*/
ble_err_t bk_ble_gap_cancel_connect(void);

/**
* @brief           This function is called to read the connection
*                  parameters information of the device
*
* @param[in]       bd_addr: BD address of the peer device.
* @param[out]      conn_params: the connection parameters information
*
* @return          - BK_OK : success
*                  - other  : failed
*
*/
ble_err_t bk_ble_get_current_conn_params(bk_bd_addr_t bd_addr, bk_gap_conn_params_t *conn_params);

/**
* @brief            BLE set channels
*
* @param[in]        channels :   The n th such field (in the range 0 to 36) contains the value for the link layer channel index n.
*                                0 means channel n is bad.
*                                1 means channel n is unknown.
*                                The most significant bits are reserved and shall be set to 0.
*                                At least one channel shall be marked as unknown.
*
* @return           - BK_OK : success
*                   - BK_ERR_INVALID_STATE: if bluetooth stack is not yet enabled
*                   - other  : failed
*
*/
ble_err_t bk_gap_ble_set_channels(bk_ble_gap_channels channels);

#if BEKEN_API_NEED
/**
* @brief           This function is called to authorized a link after Authentication(MITM protection)
*
* @param[in]       bd_addr: BD address of the peer device.
* @param[out]      authorize: Authorized the link or not.
*
* @return          - BK_OK : success
*                  - other  : failed
*
*/
ble_err_t bk_gap_ble_set_authorization(bk_bd_addr_t bd_addr, bool authorize);
#endif

#if (BLE_50_FEATURE_SUPPORT == TRUE)

/**
* @brief           This function is used to read the current transmitter PHY
*                  and receiver PHY on the connection identified by remote address.
*
* @param[in]       bd_addr : BD address of the peer device
*
* @return            - BK_OK : success
*                    - other  : failed
*
*/
ble_err_t bk_ble_gap_read_phy(bk_bd_addr_t bd_addr);

/**
* @brief           This function is used to allows the Host to specify its preferred values
*                  for the transmitter PHY and receiver PHY to be used for all subsequent connections
*                  over the LE transport.
*
* @param[in]       tx_phy_mask : indicates the transmitter PHYs that the Host prefers the Controller to use
* @param[in]       rx_phy_mask : indicates the receiver PHYs that the Host prefers the Controller to use
*
* @return            - BK_OK : success
*                    - other  : failed
*
*/
ble_err_t bk_ble_gap_set_preferred_default_phy(bk_ble_gap_phy_mask_t tx_phy_mask, bk_ble_gap_phy_mask_t rx_phy_mask);
/**
* @brief           This function is used to set the PHY preferences for the connection identified by the remote address.
*                  The Controller might not be able to make the change (e.g. because the peer does not support the requested PHY)
*                  or may decide that the current PHY is preferable.
*
* @param[in]       bd_addr : remote address
* @param[in]       all_phys_mask : a bit field that allows the Host to specify
* @param[in]       tx_phy_mask : a bit field that indicates the transmitter PHYs that the Host prefers the Controller to use
* @param[in]       rx_phy_mask : a bit field that indicates the receiver PHYs that the Host prefers the Controller to use
* @param[in]       phy_options : a bit field that allows the Host to specify options for PHYs
*
* @return            - BK_OK : success
*                    - other  : failed
*
*/
ble_err_t bk_ble_gap_set_preferred_phy(bk_bd_addr_t bd_addr,
                                       bk_ble_gap_all_phys_t all_phys_mask,
                                       bk_ble_gap_phy_mask_t tx_phy_mask,
                                       bk_ble_gap_phy_mask_t rx_phy_mask,
                                       bk_ble_gap_prefer_phy_options_t phy_options);

/**
* @brief           This function is used by the Host to set the random device address specified by the Random_Address parameter.
*
* @param[in]       instance  : Used to identify an advertising set
* @param[in]       rand_addr : Random Device Address
*
* @return            - BK_OK : success
*                    - other  : failed
*
*/
ble_err_t bk_ble_gap_set_adv_rand_addr(uint8_t instance, bk_bd_addr_t rand_addr);

/**
* @brief           This function is used by the Host to set the advertising parameters.
*
* @param[in]       instance : identifies the advertising set whose parameters are being configured.
* @param[in]       params   : advertising parameters
*
* @attention       1. local_addr_type can be BLE_ADDR_TYPE_PUBLIC ~ BLE_ADDR_TYPE_RPA_RANDOM.
* @attention       2. if local_addr_type is BLE_ADDR_TYPE_PUBLIC, host will use public addr to send adv.
* @attention       3. if local_addr_type is BLE_ADDR_TYPE_RANDOM, host will use random addr that set in bk_ble_gap_set_adv_rand_addr to send adv.
* @attention       4. if local_addr_type is BLE_ADDR_TYPE_RPA_PUBLIC, host will try search bond list by peer_addr/peer_addr_type, if found and BK_LE_KEY_PID | BK_LE_KEY_LID
*                     present then host will generate RPA addr to send ADV, otherwise see attention 2.
* @attention       5. if local_addr_type is BLE_ADDR_TYPE_RPA_RANDOM, host will try search bond list by peer_addr/peer_addr_type, if found and BK_LE_KEY_PID | BK_LE_KEY_LID
*                     present then host will generate RPA addr to send ADV, otherwise see attention 3.
*
*
* @return            - BK_OK : success
*                    - other  : failed
*
*/
ble_err_t bk_ble_gap_set_adv_params(uint8_t instance, const bk_ble_gap_ext_adv_params_t *params);

/**
* @brief           This function is used to set the data used in advertising PDUs that have a data field
*
* @param[in]       instance : identifies the advertising set whose data are being configured
* @param[in]       length   : data length
* @param[in]       data     : data information
*
* @return            - BK_OK : success
*                    - other  : failed
*
*/
ble_err_t bk_ble_gap_set_adv_data(bk_ble_adv_data_t *adv_data);

/**
* @brief           This function is used to set the data used in advertising PDUs that have a data field
*
* @param[in]       instance : identifies the advertising set whose data are being configured
* @param[in]       length   : data length
* @param[in]       data     : data information
*
* @return            - BK_OK : success
*                    - other  : failed
*
*/
ble_err_t bk_ble_gap_set_adv_data_raw(uint8_t instance, uint16_t length, const uint8_t *data);

/**
* @brief           This function is used to provide scan response data used in scanning response PDUs
*
* @param[in]       instance : identifies the advertising set whose response data are being configured.
* @param[in]       length : responsedata length
* @param[in]       scan_rsp_data : response data information
*
* @return            - BK_OK : success
*                    - other  : failed
*
*/
ble_err_t bk_ble_gap_set_scan_rsp_data_raw(uint8_t instance, uint16_t length,
                                           const uint8_t *scan_rsp_data);
/**
* @brief           This function is used to request the Controller to enable one or more
*                  advertising sets using the advertising sets identified by the instance parameter.
*
* @param[in]       num_adv : Number of advertising sets to enable or disable
* @param[in]       ext_adv : adv parameters
*
* @return            - BK_OK : success
*                    - other  : failed
*
*/
ble_err_t bk_ble_gap_adv_start(uint8_t num_adv, const bk_ble_gap_ext_adv_t *ext_adv);

/**
* @brief           This function is used to request the Controller to disable one or more
*                  advertising sets using the advertising sets identified by the instance parameter.
*
* @param[in]       num_adv : Number of advertising sets to enable or disable
* @param[in]       ext_adv_inst : ext adv instance
*
* @return            - BK_OK : success
*                    - other  : failed
*
*/
ble_err_t bk_ble_gap_adv_stop(uint8_t num_adv, const uint8_t *ext_adv_inst);

/**
* @brief           This function is used to remove an advertising set from the Controller.
*
* @param[in]       instance : Used to identify an advertising set
*
* @return            - BK_OK : success
*                    - other  : failed
*
*/
ble_err_t bk_ble_gap_adv_set_remove(uint8_t instance);

/**
* @brief           This function is used to remove all existing advertising sets from the Controller.
*
*
* @return            - BK_OK : success
*                    - other  : failed
*
*/
ble_err_t bk_ble_gap_adv_set_clear(void);

/**
* @brief           This function is used by the Host to set the parameters for periodic advertising.
*
* @param[in]       instance : identifies the advertising set whose periodic advertising parameters are being configured.
* @param[in]       params : periodic adv parameters
*
* @return            - BK_OK : success
*                    - other  : failed
*
*/
ble_err_t bk_ble_gap_set_periodic_adv_params(uint8_t instance, const bk_ble_gap_periodic_adv_params_t *params);

/**
* @brief           This function is used to set the data used in periodic advertising PDUs.
*
* @param[in]       instance : identifies the advertising set whose periodic advertising parameters are being configured.
* @param[in]       length : the length of periodic data
* @param[in]       data : periodic data information
*
* @return            - BK_OK : success
*                    - other  : failed
*
*/
ble_err_t bk_ble_gap_set_periodic_adv_data_raw(uint8_t instance, uint16_t length, const uint8_t *data);
/**
* @brief           This function is used to request the Controller to enable the periodic advertising for the advertising set specified
*
* @param[in]       instance : Used to identify an advertising set
*
* @return            - BK_OK : success
*                    - other  : failed
*
*/
ble_err_t bk_ble_gap_periodic_adv_start(uint8_t instance);

/**
* @brief           This function is used to request the Controller to disable the periodic advertising for the advertising set specified
*
* @param[in]       instance : Used to identify an advertising set
*
* @return            - BK_OK : success
*                    - other  : failed
*
*/
ble_err_t bk_ble_gap_periodic_adv_stop(uint8_t instance);

/**
* @brief           This function is used to synchronize with periodic advertising from an advertiser and begin receiving periodic advertising packets.
*
* @param[in]       params : sync parameters
*
* @return            - BK_OK : success
*                    - other  : failed
*
*/
ble_err_t bk_ble_gap_periodic_adv_create_sync(const bk_ble_gap_periodic_adv_sync_params_t *params);

/**
* @brief           This function is used to cancel the LE_Periodic_Advertising_Create_Sync command while it is pending.
*
*
* @return            - BK_OK : success
*                    - other  : failed
*
*/
ble_err_t bk_ble_gap_periodic_adv_sync_cancel(void);

/**
* @brief           This function is used to stop reception of the periodic advertising identified by the Sync Handle parameter.
*
* @param[in]       sync_handle : identify the periodic advertiser
*
* @return            - BK_OK : success
*                    - other  : failed
*
*/
ble_err_t bk_ble_gap_periodic_adv_sync_terminate(uint16_t sync_handle);

/**
* @brief           This function is used to add a single device to the Periodic Advertiser list stored in the Controller
*
* @param[in]       addr_type : address type
* @param[in]       addr : Device Address
* @param[in]       sid : Advertising SID subfield in the ADI field used to identify the Periodic Advertising
*
* @return            - BK_OK : success
*                    - other  : failed
*
*/
ble_err_t bk_ble_gap_periodic_adv_add_dev_to_list(bk_ble_addr_type_t addr_type,
                                                  bk_bd_addr_t addr,
                                                  uint8_t sid);

/**
* @brief           This function is used to remove one device from the list of Periodic Advertisers stored in the Controller.
*                  Removals from the Periodic Advertisers List take effect immediately.
*
* @param[in]       addr_type : address type
* @param[in]       addr : Device Address
* @param[in]       sid : Advertising SID subfield in the ADI field used to identify the Periodic Advertising
*
* @return            - BK_OK : success
*                    - other  : failed
*
*/
ble_err_t bk_ble_gap_periodic_adv_remove_dev_from_list(bk_ble_addr_type_t addr_type,
                                                       bk_bd_addr_t addr,
                                                       uint8_t sid);
/**
* @brief           This function is used to remove all devices from the list of Periodic Advertisers in the Controller.
*
* @return            - BK_OK : success
*                    - other  : failed
*
*/
ble_err_t bk_ble_gap_periodic_adv_clear_dev(void);

/**
* @brief           This function is used to set the extended scan parameters to be used on the advertising channels.
*
* @param[in]       params : scan parameters
*
* @return            - BK_OK : success
*                    - other  : failed
*
*/
ble_err_t bk_ble_gap_set_scan_params(const bk_ble_ext_scan_params_t *params);

/**
* @brief           This function is used to enable scanning.
*
* @param[in]       duration : Scan duration
* @param[in]       period  : Time interval from when the Controller started its last Scan Duration until it begins the subsequent Scan Duration.
*
* @return            - BK_OK : success
*                    - other  : failed
*
*/
ble_err_t bk_ble_gap_start_scan(uint32_t duration, uint16_t period);

/**
* @brief           This function is used to disable scanning.
*
* @return            - BK_OK : success
*                    - other  : failed
*
*/
ble_err_t bk_ble_gap_stop_scan(void);

/**
* @brief           This function is used to set aux connection parameters
*
* @param[in]       addr : device address
* @param[in]       phy_mask : indicates the PHY(s) on which the advertising packets should be received on the primary advertising channel and the PHYs for which connection parameters have been specified.
* @param[in]       phy_1m_conn_params : Scan connectable advertisements on the LE 1M PHY. Connection parameters for the LE 1M PHY are provided.
* @param[in]       phy_2m_conn_params : Connection parameters for the LE 2M PHY are provided.
* @param[in]       phy_coded_conn_params : Scan connectable advertisements on the LE Coded PHY. Connection parameters for the LE Coded PHY are provided.
*
* @return            - BK_OK : success
*                    - other  : failed
*
*/
ble_err_t bk_ble_gap_prefer_connect_params_set(bk_bd_addr_t addr,
                                               bk_ble_gap_phy_mask_t phy_mask,
                                               const bk_ble_gap_conn_params_t *phy_1m_conn_params,
                                               const bk_ble_gap_conn_params_t *phy_2m_conn_params,
                                               const bk_ble_gap_conn_params_t *phy_coded_conn_params);

/**
* @brief           This function is used to create bond to peer, this function must used after connected.
*
* @param[in]       bd_addr : peer address
* @return            - BK_OK : success
*                    - other  : failed
*
*/
ble_err_t bk_ble_gap_create_bond(bk_bd_addr_t bd_addr);



/**
* @brief           This function is used to add, remove or clean bond dev info. BK_BLE_GAP_BOND_DEV_LIST_OPERATEION_COMPLETE_EVT evt will report when completed.
* @attention       1. Because app layer save bond info, when ble power on, app layer must add bond dev info one by one.
* @attention       2. When app layer remove bond info from it self storage, app layer should call BK_GAP_BOND_DEV_LIST_OPERATION_REMOVE or BK_GAP_BOND_DEV_LIST_OPERATION_CLEAN after it done.
* @attention       3. This func can't be call when ble connection/adv/scan is exist.

* @param[in]       op : operation
* @param[in]       device : device info
* @return            - BK_OK : success
*                    - other  : failed
*
*/
ble_err_t bk_ble_gap_bond_dev_list_operation(bk_gap_bond_dev_list_operation_t op, bk_ble_bond_dev_t *device);

/**
* @brief           This function is used to generate rpa. BK_BLE_GAP_GENERATE_RPA_COMPLETE_EVT evt will report when completed.
*
* @param[in]       irk : the key which used to generate rpa, when this argument is NULL, host will use current local irk to generate. If local irk is not exist(suchas ir not set), will return fail.
*
* @return            - BK_OK : success
*                    - other  : failed
*
*/
ble_err_t bk_ble_gap_generate_rpa(uint8_t *irk);

#endif //#if (BLE_50_FEATURE_SUPPORT == TRUE)

/// @}

/// @}

#ifdef __cplusplus
}
#endif

