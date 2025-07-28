/**
 * @file tkl_bluetooth_mesh_device.h
 * @brief This is tuya tal_adc file
 * @version 1.0
 * @date 2021-09-10
 *
 * @copyright Copyright 2021-2031 Tuya Inc. All Rights Reserved.
 *
 */

#ifndef __TKL_BLUETOOTH_MESH_DEVICE_H__
#define __TKL_BLUETOOTH_MESH_DEVICE_H__

#include "tkl_bluetooth_mesh_def.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Send mesh data to unicast/group addr.
 * @param   [in] src_addr  element addr of node
 * @param   [in] dst_addr  destinat addr
 * @param   [in] opcode  mesh op code
 * @param   [in] data  data point
 * @param   [in] data_len  data point
 * @param   [in] appkey_idx  use which appkey index
 * @return  tx result
 * */
OPERATE_RET tkl_mesh_access_normal_data_send(USHORT_T src_addr, USHORT_T dst_addr, UINT_T opcode, UCHAR_T *data, USHORT_T data_len, USHORT_T appkey_idx);

/**
 * @brief   Send mesh data to unicast/group addr.
 * @param   [in] src_addr  element addr of node
 * @param   [in] dst_addr  destinat addr
 * @param   [in] opcode  mesh op code
 * @param   [in] data  data point
 * @param   [in] data_len  data point
 * @return  tx result
 * */
OPERATE_RET tkl_mesh_access_rsp_data_send(USHORT_T src_addr, USHORT_T dst_addr, UINT_T opcode, UCHAR_T *data, USHORT_T data_len);

/**
 * @brief   Init the mesh node composition data(head).
 * @param   [in] company_id  company identifier assigned by the Bluetooth SIG
 * @param   [in] product_id  product identifier
 * @param   [in] version_id  product version identifier
 * @return  NULL
 * */
VOID tkl_mesh_composition_data_set(USHORT_T company_id, USHORT_T product_id, USHORT_T version_id);

/**
 * @brief   Init the mesh node uuid.
 * @param   [in] uuid  mesh universally unique identifier
 * @return  NULL
 * */
VOID tkl_mesh_uuid_set(UCHAR_T* uuid);

/**
 * @brief   Function to subscribe a group address for a model of the element.
 * @param   [in] opcode  to indicate subscribe or delete
 * @param   [in] ele_index  the index of element
 * @param   [in] group_addr  group address
 * @return  SUCCESS
 *          ERROR
 *          INSUFFICIENT_RESOURCES
 * */
OPERATE_RET tkl_mesh_group_addr_sub_set(USHORT_T opcode, USHORT_T ele_addr, USHORT_T sub_addr, UINT_T model_id);

/**
 * @brief   Function to get the subscribe addr list of a model of the element.
 * @param   [in] ele_index  the index of element
 * @param   [in] group_addr  group address
 * @return  subscribe addr list
 * */
USHORT_T* tkl_mesh_group_addr_sub_list_get(USHORT_T ele_idx, UINT_T model_id);

/**
 * @brief   Set the network layer retransmit parameters.
 * @param   [in] cnt  transmit cnt = (cnt + 1) times
 * @param   [in] step  retransmission interval = (net_trans_steps + 1) * 10ms
 * @return  NULL
 * */
VOID tkl_mesh_network_transmit_set(UCHAR_T cnt, UCHAR_T step);

/**
 * @brief   Get the network layer retransmit parameters.
 * @param   [out] cnt  transmit cnt = (cnt + 1) times
 * @param   [out] step  retransmission interval = (net_trans_steps + 1) * 10ms
 * @return  NULL
 * */
VOID tkl_mesh_network_transmit_get(UCHAR_T* cnt, UCHAR_T* step);

/**
 * @brief   Set the node PB-GATT ADV use identity type.
 * @return  NULL
 * */
VOID tkl_mesh_mode_identity_set(VOID);

/**
 * @brief   Set the network layer relay retransmit parameters.
 * @param   [in] cnt  transmit cnt = (cnt + 1) times
 * @param   [in] step  retransmission interval = (net_trans_steps + 1) * 10ms
 * @return  NULL
 * */
VOID tkl_mesh_network_relay_retransmit_set(UCHAR_T cnt, UCHAR_T step);

/**
 * @brief   Get the network layer retransmit parameters.
 * @param   [out] cnt  transmit cnt = (cnt + 1) times
 * @param   [out] step  retransmission interval = (net_trans_steps + 1) * 10ms
 * @return  NULL
 * */
VOID tkl_mesh_network_relay_retransmit_get(UCHAR_T* cnt, UCHAR_T* step);

/**
 * @brief   Set the node feature.
 * @param   [in] featrue  Relay, proxy, friend or Low Power.
 * @param   [in] enable  Enable or Disable
 * @return  SUCCESS
 *          ERROR
 * */
OPERATE_RET tkl_mesh_node_features_set(MESH_FEATURE_T featrue, UCHAR_T enable);

/**
 * @brief   Get the node feature.
 * @param   [in] featrue  Relay, proxy, friend or Low Power.
 * @return  Enable : 1
 *          Disable : 0
 * */
UCHAR_T tkl_mesh_node_features_get(MESH_FEATURE_T featrue);

/**
 * @brief   Set the node defalut ttl.
 * @param   [in] ttl(time to live).
 * @return  SUCCESS
 *          ERROR
 * */
OPERATE_RET tkl_mesh_node_default_ttl_set(UCHAR_T ttl);

/**
 * @brief   Get the node defalut ttl.
 * @return  ttl
 * */
UCHAR_T tkl_mesh_node_default_ttl_get(void);

/**
 * @brief   Save all mesh param into flash.
 * @return  ttl
 * */
VOID tkl_mesh_node_param_save_all(VOID);

/**
 * @brief   Get the mesh node provision state.
 * @return  the provision state
 * */
UCHAR_T tkl_mesh_get_if_prov_success(VOID);

/**
 * @brief   Get the mesh node primary element address.
 * @return  the primary element address
 * */
USHORT_T tkl_mesh_primary_ele_addr_get(VOID);

/**
 * @brief   Set the mesh node primary element address.
 * @param   [in] addr  unicast address
 * @param   [in] flash_save_en  save in flash or only update in ram
 * @return  NULL
 * */
VOID tkl_mesh_primary_ele_addr_set(USHORT_T addr, int flash_save_en);

/**
 * @brief   Set the mesh node device key.
 * @param   [in] key  point of device key
 * @return  NULL
 * */
VOID tkl_mesh_device_key_update(UCHAR_T *key);

/**
 * @brief   get the mesh node device key.
 * @param   [in] key  point of device key
 * @return  NULL
 * */
VOID tkl_mesh_device_key_get(UCHAR_T *key);

/**
 * @brief   Reset the seq cache.
 * @param   [in] addr  the src addr
 * @return  NULL
 * */
VOID tkl_mesh_seq_cache_reset(USHORT_T addr);


/**
 * @brief   Reset the mesh node into unprovision state.
 *          The unprovision state should store in flash,and the clear the network parameters.
 * @return  NULL
 * */
VOID tkl_mesh_network_reset(VOID);

/**
 * @brief   Reset the mesh node into unprovision state.
 *          The unprovision state only keep in ram,and must not clear the network parameters.
 * @return  NULL
 * */
VOID tkl_mesh_network_reset_in_ram(VOID);

/**
 * @brief   Recover the mesh node into provision state.
 *          Node recover into provision state with the network parameters in flash.
 * @return  NULL
 * */
VOID tkl_mesh_network_recover(VOID);

/**
 * @brief   Close the mesh node mesh beacon.
 *          Only use in unprovision state.
 * @param   [in] enable  enable or disable the adv
 * @return  NULL
 * */
VOID tkl_mesh_node_unprovision_beacon_enable(UCHAR_T enable);

/**
 * @brief   Close the mesh node gatt adv.
 *          Only use in unprovision state.
 * @param   [in] enable  enable or disable the adv
 * @return  NULL
 * */
VOID tkl_mesh_node_pbgatt_adv_enable(UCHAR_T enable);

/**
 * @brief   Update the mesh network provision data and the app key data.
 *          Use this function node can provision itself into provision state.
 * @param   [in] prov_en    0 device is unprovision state, only update in ram
                            1 device is provisioned state, save in flash
 * @param   [in] prov_data  network provision data
 * @param   [in] app_key  app key data
 * @return  SUCCESS
 *          ERROR
 * */
OPERATE_RET tkl_mesh_provision_data_set(UCHAR_T prov_en, TKL_NET_PROV_DATA_T *prov_data, TKL_APP_KEY_DATA_T *app_key);

/**
 * @brief   Bind all the model use the appkey index input.
 * @param   [in] appkey_idx  app key index
 * @return  SUCCESS
 *          ERROR
 * */
OPERATE_RET tkl_mesh_model_appkey_bind_all(UINT16_T appkey_idx);

/**
 * @brief   Function to register a elemennt.
 * @param   [in] element_index  index of element
 * @return  SUCCESS
 *          ERROR
 * */
OPERATE_RET tkl_mesh_element_register(USHORT_T element_index);

/**
 * @brief   Function to register a model into the element.
 * @param   [in] element_index  index of element
 * @param   [in] pmodel_info  model information
 * @return  SUCCESS
 *          ERROR
 * */
OPERATE_RET tkl_mesh_model_register(UCHAR_T element_index, TKL_MESH_MODEL_HADLE_T *pmodel_info);

/**
 * @brief   Function to register a mesh net state callback into tkl.
 * @param   [in] TKL_MESH_NET_STATE_CB_T  callback
 * @return  SUCCESS
 *          ERROR
 * */
OPERATE_RET tkl_mesh_net_state_cb_register(TKL_MESH_NET_STATE_CB_T mesh_net_state_cb);

#ifdef __cplusplus
}
#endif

#endif
