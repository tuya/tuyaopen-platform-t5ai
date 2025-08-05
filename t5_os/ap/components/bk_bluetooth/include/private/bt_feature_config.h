#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>


typedef struct
{
    uint8_t _is_gatt_discovery_auto;                //only valid when use single ble host.
    uint8_t _ignore_smp_key_distr_all_zero;         //only valid when use single ble host.
    uint8_t _strict_smp_key_distr_check_except_all_zero;
    uint8_t _ignore_smp_already_pair;               //only valid when use single ble host.
    uint8_t _send_peripheral_feature_req_auto;
    uint8_t _stop_smp_when_pair_err;
    uint8_t _enable_smp_sec_req_evt;

    uint8_t _support_lpo_rosc;
} bt_feature_struct_t;
