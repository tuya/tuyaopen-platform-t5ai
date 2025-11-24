#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "cif_main.h"

#define CTRL_IF_CMD     os_printf

struct bk_msg_hdr;

bk_err_t cif_bk_send_event(uint16_t event_id, uint8_t *event_data, uint16_t event_len);
bk_err_t cif_bk_cmd_confirm(struct bk_msg_hdr *rx_msg, uint8_t *cfm_data, uint16_t cfm_len);
bk_err_t cif_handle_bk_cmd_connect_ind(char *ssid, uint8_t rssi, uint32_t ip, uint32_t gw, uint32_t mk, uint32_t dns);
bk_err_t cif_handle_bk_cmd_disconnect_ind(bool local_generated, uint16_t reason_code);
bk_err_t cif_send_exit_sleep_cfm(void);
bk_err_t cif_handle_bk_cmd(void *cmd);
bk_err_t cif_handle_bk_cmd_enter_sleep_cfm(void);
bk_err_t cif_handle_bk_cmd_get_wlan_status_cfm(char *ssid, uint8_t rssi, uint8_t status, char *ip, char *gw, char *mk, char *dns);
bk_err_t cif_handle_bk_cmd_start_ap_cfm(void);
bk_err_t cif_handle_bk_cmd_start_ap_ind(uint8_t status);
bk_err_t cif_handle_bk_cmd_assoc_ap_ind(uint8_t* mac_addr);
bk_err_t cif_handle_bk_cmd_disassoc_ap_ind(uint8_t* mac_addr);
bk_err_t cif_handle_bk_cmd_stop_ap_ind(uint8_t status);
bk_err_t cif_handle_bk_cmd_scan_wifi_cfm(void);
bk_err_t cif_handle_bk_cmd_scan_wifi_ind(uint32_t scan_id,uint32_t scan_use_time);
bk_err_t cif_handle_bk_cmd_bcn_cc_ind(uint8_t *cc, uint8_t cc_len);
bk_err_t cif_handle_bk_cmd_csi_info_ind(void *data);
bk_err_t cif_send_customer_cmd_cfm(uint8_t *data, uint16_t len, struct bk_msg_hdr *msg);
bk_err_t cif_send_customer_event(uint8_t *data, uint16_t len);
int32_t bluetooth_controller_deinit_api(void);
#ifdef __cplusplus
}
#endif
