#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "cif_main.h"

#define CNTRL_IF_ENTER_LV_DELAY_TIME_MS             200

#define CTRL_IF_PS    os_printf

enum {
    CIF_FSM_STATE_IDLE,
    CIF_FSM_STATE_ACTIVE,
    CIF_FSM_STATE_PREPEARE_SLEEP,
    CIF_FSM_STATE_SLEEP,
    CIF_FSM_STATE_PREPARE_WAKEUP,
    CIF_FSM_STATE_WAKEUP_BY_HOST,
};

enum {
    CIF_EVT_RX_HOST_CONNECT_REQ = 0,
    CIF_EVT_RX_HOST_ASK_SLEEP_REQ,
    CIF_EVT_TX_HOST_ASK_SLEEP_CFM_FAILED,
    CIF_EVT_PREPARE_SLEEP_TIMER_TOT,
    CIF_EVT_HOST_ASK_WAKEUP_REQ,
    CIF_EVT_TX_HOST_ASK_WAKEUP_CFM_FAILED,
    CIF_EVT_WAKEUP_HOST_REQ,
    CIF_EVT_RX_WAKEUP_HOST_CFM,
    CIF_EVT_WAKEUP_HOST_TOT,
};

void cif_update_ps_state(uint8_t cif_fsm_evt);
void cif_enter_lv_timer_cb(void *Larg, void *Rarg);
void cif_host_wakeup_gpio_init(void);
void cif_start_lv_sleep(void);
bk_err_t cif_exit_sleep(void);
bk_err_t cif_power_up_host(void);
bk_err_t cif_power_down_host(void);
bk_err_t cif_exit_sleep(void);
void cif_start_deep_sleep(void);
void cif_battery_test_mode(bool test_mode);
#ifdef __cplusplus
}
#endif