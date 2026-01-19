#pragma once

#include "bk_dm_gap_bt_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief           register callback function. This function should be called after bk_bluedroid_enable() completes successfully
 *
 * @return
 *                  - BK_OK : Succeed
 *                  - BK_FAIL: others
 */
bk_err_t bk_bt_gap_register_callback(bk_bt_gap_cb_t callback);

/**
 * @brief           Set discoverability and connectability mode for legacy bluetooth.
 *
 * @param[in]       c_mode : one of the enums of bk_bt_connection_mode_t
 *
 * @param[in]       d_mode : one of the enums of bk_bt_discovery_mode_t
 *
 * @return
 *                  - BK_OK : Succeed
 *                  - BK_ERR_INVALID_ARG: if argument invalid
 *                  - BK_ERR_INVALID_STATE: if bluetooth stack is not yet enabled
 *                  - BK_FAIL: others
 */
bk_err_t bk_bt_gap_set_scan_mode(bk_bt_conn_mode_t c_mode, bk_bt_disc_mode_t d_mode);

/**
 * @brief           Get discoverability and connectability mode of legacy bluetooth.
 *
 * @return
 *                  - BK_OK : Succeed
 *                  - BK_ERR_INVALID_ARG: if argument invalid
 *                  - BK_ERR_INVALID_STATE: if bluetooth stack is not yet enabled
 *                  - BK_FAIL: others
 */
bk_err_t bk_bt_gap_get_scan_mode();

/**
 * @brief           This function starts Inquiry and Name Discovery.
 *                  When Inquiry is halted and cached results do not contain device name, then Name Discovery will connect to the peer target to get the device name.
 *                  bk_bt_gap_cb_t will be called with BK_BT_GAP_DISC_STATE_CHANGED_EVT when Inquriry is started or Name Discovery is completed.
 *                  bk_bt_gap_cb_t will be called with BK_BT_GAP_DISC_RES_EVT each time the two types of discovery results are got.
 *
 * @param[in]       mode - Inquiry mode
 *
 * @param[in]       inq_len - Inquiry duration in 1.28 sec units, ranging from 0x01 to 0x30. This parameter only specifies the total duration of the Inquiry process,
 *                          - when this time expires, Inquiry will be halted.
 *
 * @param[in]       num_rsps - Number of responses that can be received before the Inquiry is halted, value 0 indicates an unlimited number of responses.
 *
 * @return
 *                  - BK_OK : Succeed
 *                  - BK_ERR_INVALID_STATE: if bluetooth stack is not yet enabled
 *                  - BK_ERR_INVALID_ARG: if invalid parameters are provided
 *                  - BK_FAIL: others
 */
bk_err_t bk_bt_gap_start_discovery(bk_bt_inq_mode_t mode, uint8_t inq_len, uint8_t num_rsps);

/**
 * @brief           Cancel Inquiry and Name Discovery.
 *                  bk_bt_gap_cb_t will be called with BK_BT_GAP_DISC_STATE_CHANGED_EVT if Inquiry or Name Discovery is cancelled by
 *                  calling this function.
 *
 * @return
 *                  - BK_OK : Succeed
 *                  - BK_ERR_INVALID_STATE: if bluetooth stack is not yet enabled
 *                  - BK_FAIL: others
 */
bk_err_t bk_bt_gap_cancel_discovery(void);

/**
 * @brief           Start SDP to get remote services.
 *                  bk_bt_gap_cb_t will be called with BK_BT_GAP_RMT_SRVCS_EVT after service discovery ends.
 *
 * @return
 *                  - BK_OK : Succeed
 *                  - BK_ERR_INVALID_STATE: if bluetooth stack is not yet enabled
 *                  - BK_FAIL: others
 */
bk_err_t bk_bt_gap_get_remote_services(bk_bd_addr_t remote_bda);

/**
 * @brief           Start SDP to look up the service matching uuid on the remote device.
 *
 *                  bk_bt_gap_cb_t will be called with BK_BT_GAP_RMT_SRVC_REC_EVT after service discovery ends
 * @return
 *                  - BK_OK : Succeed
 *                  - BK_ERR_INVALID_STATE: if bluetooth stack is not yet enabled
 *                  - BK_FAIL: others
 */
bk_err_t bk_bt_gap_get_remote_service_record(bk_bd_addr_t remote_bda, bk_bt_uuid_t *uuid);

/**
 * @brief           This function is called to config EIR data.
 *
 *                  bk_bt_gap_cb_t will be called with BK_BT_GAP_CONFIG_EIR_DATA_EVT after config EIR ends.
 *
 * @param[in]       eir_data - pointer of EIR data content
 * @return
 *                  - BK_OK : Succeed
 *                  - BK_ERR_INVALID_STATE: if bluetooth stack is not yet enabled
 *                  - BK_ERR_INVALID_ARG: if param is invalid
 *                  - BK_FAIL: others
 */
bk_err_t bk_bt_gap_config_eir_data(bk_bt_eir_data_t *eir_data);

/**
 * @brief           This function is called to set class of device.
 *                  The structure bk_bt_gap_cb_t will be called with BK_BT_GAP_SET_COD_EVT after set COD ends.
 *                  Some profile have special restrictions on class of device, changes may cause these profile do not work.
 *
 * @param[in]       cod - class of device
 * @param[in]       mode - setting mode
 *
 * @return
 *                  - BK_OK : Succeed
 *                  - BK_ERR_INVALID_STATE: if bluetooth stack is not yet enabled
 *                  - BK_ERR_INVALID_ARG: if param is invalid
 *                  - BK_FAIL: others
 */
bk_err_t bk_bt_gap_set_cod(bk_bt_cod_t cod, bk_bt_cod_mode_t mode);

/**
 * @brief           This function is called to read RSSI delta by address after connected. The RSSI value returned by BK_BT_GAP_READ_RSSI_DELTA_EVT.
 *
 *
 * @param[in]       remote_addr - remote device address, corrbkonding to a certain connection handle
 * @return
 *                  - BK_OK : Succeed
 *                  - BK_FAIL: others
 *
 */
bk_err_t bk_bt_gap_read_rssi_delta(bk_bd_addr_t remote_addr);

/**
* @brief           Removes a device from the security database list of
*                  peer device.
*
* @param[in]       bd_addr : BD address of the peer device
*
* @return          - BK_OK : success
*                  - BK_FAIL  : failed
*
*/
bk_err_t bk_bt_gap_remove_bond_device(bk_bd_addr_t bd_addr);

/**
* @brief           Get the device number from the security database list of peer device.
*                  It will return the device bonded number immediately.
*
* @return          - >= 0 : bonded devices number
*                  - BK_FAIL  : failed
*
*/
int bk_bt_gap_get_bond_device_num(void);

/**
* @brief           Get the device from the security database list of peer device.
*                  It will return the device bonded information immediately.
*
* @param[inout]    dev_num: Indicate the dev_list array(buffer) size as input.
*                           If dev_num is large enough, it means the actual number as output.
*                           Suggest that dev_num value equal to bk_ble_get_bond_device_num().
*
* @param[out]      dev_list: an array(buffer) of `bk_bd_addr_t` type. Use for storing the bonded devices address.
*                            The dev_list should be allocated by who call this API.
*
* @return
*                  - BK_OK : Succeed
*                  - BK_ERR_INVALID_STATE: if bluetooth stack is not yet enabled
*                  - BK_FAIL: others
*/
bk_err_t bk_bt_gap_get_bond_device_list(int *dev_num, bk_bd_addr_t *dev_list);

/**
* @brief            Set pin type and default pin code for legacy pairing.
*
* @param[in]        pin_type:       Use variable or fixed pin.
*                                   If pin_type is BK_BT_PIN_TYPE_VARIABLE, pin_code and pin_code_len
*                                   will be ignored, and BK_BT_GAP_PIN_REQ_EVT will come when control
*                                   requests for pin code.
*                                   Else, will use fixed pin code and not callback to users.
*
* @param[in]        pin_code_len:   Length of pin_code
*
* @param[in]        pin_code:       Pin_code
*
* @return           - BK_OK : success
*                   - BK_ERR_INVALID_STATE: if bluetooth stack is not yet enabled
*                   - other  : failed
*/
bk_err_t bk_bt_gap_set_pin(bk_bt_pin_type_t pin_type, uint8_t pin_code_len, bk_bt_pin_code_t pin_code);

/**
* @brief            Reply the pin_code to the peer device for legacy pairing
*                   when BK_BT_GAP_PIN_REQ_EVT is coming.
*
* @param[in]        bd_addr:        BD address of the peer
*
* @param[in]        accept:         Pin_code reply successful or declined.
*
* @param[in]        pin_code_len:   Length of pin_code
*
* @param[in]        pin_code:       Pin_code
*
* @return           - BK_OK : success
*                   - BK_ERR_INVALID_STATE: if bluetooth stack is not yet enabled
*                   - other  : failed
*/
bk_err_t bk_bt_gap_pin_reply(bk_bd_addr_t bd_addr, bool accept, uint8_t pin_code_len, bk_bt_pin_code_t pin_code);

/**
* @brief            Set a GAP security parameter value. Overrides the default value.
*
* @param[in]        param_type : the type of the param which is to be set
*
* @param[in]        value  : the param value
*
* @param[in]        len : the length of the param value
*
* @return           - BK_OK : success
*                   - BK_ERR_INVALID_STATE: if bluetooth stack is not yet enabled
*                   - other  : failed
*
*/
bk_err_t bk_bt_gap_set_security_param(bk_bt_sp_param_t param_type,
                                        void *value, uint8_t len);

/**
* @brief            Reply the key value to the peer device in the legacy connection stage.
*
* @param[in]        bd_addr : BD address of the peer
*
* @param[in]        accept : passkey entry successful or declined.
*
* @param[in]        passkey : passkey value, must be a 6 digit number, can be lead by 0.
*
* @return           - BK_OK : success
*                   - BK_ERR_INVALID_STATE: if bluetooth stack is not yet enabled
*                   - other  : failed
*
*/
bk_err_t bk_bt_gap_ssp_passkey_reply(bk_bd_addr_t bd_addr, bool accept, uint32_t passkey);


/**
* @brief            Reply the confirm value to the peer device in the legacy connection stage.
*
* @param[in]        bd_addr : BD address of the peer device
*
* @param[in]        accept : numbers to compare are the same or different
*
* @return           - BK_OK : success
*                   - BK_ERR_INVALID_STATE: if bluetooth stack is not yet enabled
*                   - other  : failed
*
*/
bk_err_t bk_bt_gap_ssp_confirm_reply(bk_bd_addr_t bd_addr, bool accept);

/**
* @brief            Set the AFH channels
*
* @param[in]        channels :  The n th such field (in the range 0 to 78) contains the value for channel n :
*                               0 means channel n is bad.
*                               1 means channel n is unknown.
*                               The most significant bit is reserved and shall be set to 0.
*                               At least 20 channels shall be marked as unknown.
*
* @return           - BK_OK : success
*                   - BK_ERR_INVALID_STATE: if bluetooth stack is not yet enabled
*                   - other  : failed
*
*/
bk_err_t bk_bt_gap_set_afh_channels(bk_bt_gap_afh_channels channels);

/**
* @brief            Read the remote device name
*
* @param[in]        remote_bda: The remote device's address
*
* @return           - BK_OK : success
*                   - BK_ERR_INVALID_STATE: if bluetooth stack is not yet enabled
*                   - other  : failed
*
*/
bk_err_t bk_bt_gap_read_remote_name(bk_bd_addr_t remote_bda);

/**
* @brief            Connect the remote device
*
* @param[in]        addr: The remote device's address
*
* @param[in]        allow_role_switch: Allow the role switch
*
* @return           - BK_OK : success
*                   - BK_ERR_INVALID_STATE: if bluetooth stack is not yet enabled
*                   - other  : failed
*
*/
bt_err_t bk_bt_gap_connect(bk_bd_addr_t addr, uint8_t allow_role_switch);

/**
* @brief            Disconnect the remote device
*
* @param[in]        addr: The remote device's address
*
* @param[in]        reason: The disconnect reason
*
* @return           - BK_OK : success
*                   - BK_ERR_INVALID_STATE: if bluetooth stack is not yet enabled
*                   - other  : failed
*
*/
bt_err_t bk_bt_gap_disconnect(bk_bd_addr_t addr, uint8_t reason);

/**
 *
 * @brief           This API can be used to request cancellation of the ongoing connection creation process.
 *
 * @param[in]       addr: The remote device's address
 * @return
 *
 *                  - BK_ERR_BT_SUCCESS: cancel request is sent to lower layer successfully
 *                  - others: fail
 *
 */
bt_err_t bk_bt_gap_create_conn_cancel(uint8_t *addr);

/**
 *
 * @brief           This api can be used to set the value for the Page_Timeout configuration parameter
 *
 * @param[in]       timeout: Page Timeout measured in number of Baseband slots, unit of 0.625ms
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: success
 *                  -  others: fail
 *
 */
bt_err_t bk_bt_gap_set_page_timeout(uint16_t timeout);

/**
 *
 * @brief           This api can be used to set the value for the Page_Scan_Interval and Page_Scan_Window configuration parameters
 *
 * @param[in]       interval: defines the amount of time between consecutive page scans, unit of 0.625ms,(Range: 0x0012 to 0x1000, only even values are valid)
 *
 * @param[in]       window: defines the amount of time for the duration of the page scan, unit of 0.625ms,(Range: 0x0011 to 0x1000, can only be less
 *                          than or equal to the Page_Scan_Interval)
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: success
 *                  -  others: fail
 *
 */
bt_err_t bk_bt_gap_set_page_scan_activity(uint16_t interval, uint16_t window);

/**
 *
 * @brief           This api can be used to start authentication request
 *
 * @param[in]       addr: The remote device's address
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: success
 *                  -  others: fail
 *
 */
bt_err_t bk_bt_gap_authentication_request(uint8_t *addr);

/**
 * @brief           Set auto enter sniff policy.
 *                  bk_bt_gap_cb_t will be called with BK_BT_GAP_SET_AUTO_SNIFF_CMPL_EVT after service discovery ends.
 *
 * @param[in]       remote_bda: The remote device's address
 * @param[in]       sec: How long time (second) enter sniff once no ACL data send or recv. When equal 0, will disable auto enter sniff.
 * @param[in]       config: Sniff config. when is NULL, last config will be:
 *                  max_interval: 500 msec
 *                  min interval: 250 msec
 *                  attempt: 5 msec
 *                  timeout: 1.25 msec
 *
 * @return
 *                  - BK_OK : Succeed
 *                  - BK_FAIL: others
 */
bk_err_t bk_bt_gap_set_auto_sniff_policy(bk_bd_addr_t remote_bda, uint32_t sec, bk_bt_gap_sniff_config *config);

/**
 *
 * @brief           This api can be used to start authentication request
 *
 * @param[in]       addr: The remote device's address
 *
 * @return
 *                  - BK_ERR_BT_SUCCESS: success
 *                  -  others: fail
 *
 */
bt_err_t bk_bt_gap_authentication_request(uint8_t *addr);


#ifdef __cplusplus
}
#endif

