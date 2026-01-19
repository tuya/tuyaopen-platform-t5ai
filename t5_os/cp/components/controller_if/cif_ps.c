#include "cif_ps.h"
#include "gpio_driver.h"
#include "driver/timer.h"
#include <driver/hal/hal_aon_rtc_types.h>
#include <driver/aon_rtc_types.h>
#include <driver/aon_rtc.h>

extern bool battery_test_mode;

extern void stack_mem_dump(uint32_t stack_top, uint32_t stack_bottom);
void cif_update_ps_state(uint8_t cif_fsm_evt)
{
    uint8_t next_state = CIF_FSM_STATE_IDLE;
    
    GLOBAL_INT_DECLARATION();
    GLOBAL_INT_DISABLE();
    next_state = cif_env.cur_ps_state;
    if (cif_env.cur_ps_state == CIF_FSM_STATE_IDLE)
    {
        if (cif_fsm_evt == CIF_EVT_RX_HOST_CONNECT_REQ)
        {
            next_state = CIF_FSM_STATE_ACTIVE;
        }
    }
    else if (cif_env.cur_ps_state == CIF_FSM_STATE_ACTIVE)
    {
        if ((cif_fsm_evt == CIF_EVT_RX_HOST_ASK_SLEEP_REQ)
                || (cif_fsm_evt == CIF_EVT_WAKEUP_HOST_TOT))
        {
            next_state = CIF_FSM_STATE_PREPEARE_SLEEP;
        }
    }
    else if (cif_env.cur_ps_state == CIF_FSM_STATE_PREPEARE_SLEEP)
    {
        if (cif_fsm_evt == CIF_EVT_PREPARE_SLEEP_TIMER_TOT)
        {
            next_state = CIF_FSM_STATE_SLEEP;
        }
        else if ((cif_fsm_evt == CIF_EVT_WAKEUP_HOST_REQ)
                    || (cif_fsm_evt == CIF_EVT_TX_HOST_ASK_SLEEP_CFM_FAILED))
        {
            next_state = CIF_FSM_STATE_ACTIVE;
        }
    }
    else if (cif_env.cur_ps_state == CIF_FSM_STATE_SLEEP)
    {
        if (cif_fsm_evt == CIF_EVT_HOST_ASK_WAKEUP_REQ)
        {
            next_state = CIF_FSM_STATE_PREPARE_WAKEUP;
        }
        else if (cif_fsm_evt == CIF_EVT_WAKEUP_HOST_REQ)
        {
            next_state = CIF_FSM_STATE_ACTIVE;
        }
    }
    else if (cif_env.cur_ps_state == CIF_FSM_STATE_PREPARE_WAKEUP)
    {
        if (cif_fsm_evt == CIF_EVT_TX_HOST_ASK_WAKEUP_CFM_FAILED)
        {
            next_state = CIF_FSM_STATE_SLEEP;
        }
        else
        {
            next_state = CIF_FSM_STATE_ACTIVE;
        }
    }
    CIF_LOGD("CIF FSM evt:%d cur state:%d next:%d\n", cif_fsm_evt, cif_env.pre_ps_state, next_state);
    cif_env.pre_ps_state = cif_env.cur_ps_state;
    cif_env.cur_ps_state = next_state;
    GLOBAL_INT_RESTORE();
}

bk_err_t cif_low_power_handler(void)
{
    return BK_OK;
}

bk_err_t cif_power_up_host(void)
{
    bk_err_t ret = BK_OK;

    if (cif_env.host_powerup == true)
    {
        CTRL_IF_PS("cif_power_down_host host alread up, return\r\n");
        return BK_FAIL;
    }
    //cif_power_up_host_by_gpio();
    bk_pm_module_vote_cpu_freq(PM_DEV_ID_CIF, PM_CPU_FRQ_480M);
    ret = cif_exit_sleep();
    cif_env.host_powerup = true;
    return ret;
}
bk_err_t cif_power_down_host(void)
{
    if (cif_env.host_powerup == false)
    {
        CTRL_IF_PS("cif_power_down_host host not up, return\r\n");
        return BK_FAIL;
    }
    bluetooth_controller_deinit_api();
    //cif_power_down_host_by_gpio();
    cif_env.host_powerup = false;
    cif_env.host_wifi_init = false;
    bk_pm_module_vote_cpu_freq(PM_DEV_ID_CIF, PM_CPU_FRQ_DEFAULT);
    return BK_OK;
}

bk_err_t cif_exit_sleep(void)
{
    bk_err_t ret;

    if (cif_env.cif_sleeping == false)
    {
        CTRL_IF_PS("cif_exit_sleep no sleeping, return\r\n");
        return BK_OK;
    }

    if (cif_env.cif_sleep_mode == PM_MODE_LOW_VOLTAGE) 
    {
        ret = bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_APP,0x0,0x0);
        if (ret != BK_OK)
        {
            CTRL_IF_PS("bk_pm_module_vote_sleep_ctrl failed ret:%d\r\n", ret);
            return ret;
        }
//        sdio_gpio_init();//restore SDIO Pin normal setting
//        ret = sdio_slave_hw_reinit();
        if (ret != BK_OK)
        {
            CTRL_IF_PS("sdio_slave_hw_reinit failed ret:%d\r\n", ret);
            return ret;
        }
        //cif_exit_lv_sleep_msg_sender();
        cif_env.cif_sleeping = false;
        CTRL_IF_PS("Controller has been woken up\r\n");
    }
    else if (cif_env.cif_sleep_mode == PM_MODE_DEEP_SLEEP) 
    {
        cif_env.cif_sleeping = false;
    }

    cif_env.cif_sleep_mode = PM_MODE_NORMAL_SLEEP;
    return BK_OK;
}

void cif_host_wakeup_gpio_isr(gpio_id_t gpio_id)
{
    #if CONFIG_CIF_HOST_POWER_UP_EN
    // Do nothing
    #else
    if (cif_env.cif_sleeping == false)
    {
        //sdio_reset();
        return;
    }
    #endif

    cif_exit_sleep();
}

void cif_start_lv_sleep(void)
{
    if (cif_env.cif_sleeping && (cif_env.cif_sleep_mode == PM_MODE_LOW_VOLTAGE))
    {
        CTRL_IF_PS("%s already in LV sleep, return\r\n",__func__);
        return;
    }
    bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_APP, 1, 0);

    // bk_gpio_register_isr(HOST_WAKEUP_CONTROLLER_GPIO_NO, cif_host_wakeup_gpio_isr);
    // bk_gpio_register_wakeup_source(HOST_WAKEUP_CONTROLLER_GPIO_NO, GPIO_INT_TYPE_RISING_EDGE);
    // bk_gpio_enable_interrupt(HOST_WAKEUP_CONTROLLER_GPIO_NO);
    // bk_pm_wakeup_source_set(PM_WAKEUP_SOURCE_INT_GPIO, NULL);
    bk_pm_sleep_mode_set(PM_MODE_LOW_VOLTAGE);

    cif_env.cif_sleeping = true;
    cif_env.cif_sleep_mode = PM_MODE_LOW_VOLTAGE;
    //sdio_enter_low_power(NULL);
    //sdio_gpio_deinit();//SDIO Pin set to Hi-Z state for power saving
}
void cif_enter_lv_timer_cb(void *Larg, void *Rarg)
{
    CTRL_IF_PS("%s\r\n",__func__);
    cif_start_lv_sleep();
    //cif_update_ps_state(CIF_EVT_PREPARE_SLEEP_TIMER_TOT);
    CTRL_IF_PS("Controller enter sleep mode requested by Host\r\n");
}

void cif_start_deep_sleep(void)
{
    if (cif_env.cif_sleeping && (cif_env.cif_sleep_mode == PM_MODE_DEEP_SLEEP))
    {
        CTRL_IF_PS("%s already in deep sleep, return\r\n",__func__);
        return;
    }

    bk_pm_module_vote_sleep_ctrl(PM_SLEEP_MODULE_NAME_APP, 1, 0);

    // bk_gpio_register_isr(HOST_WAKEUP_CONTROLLER_GPIO_NO, cif_host_wakeup_gpio_isr);
    // bk_gpio_register_wakeup_source(HOST_WAKEUP_CONTROLLER_GPIO_NO, GPIO_INT_TYPE_RISING_EDGE);
    // bk_pm_wakeup_source_set(PM_WAKEUP_SOURCE_INT_GPIO, NULL);
    bk_pm_sleep_mode_set(PM_MODE_DEEP_SLEEP);

    cif_env.cif_sleeping = true;
    cif_env.cif_sleep_mode = PM_MODE_DEEP_SLEEP;
    #if CONFIG_CIF_HOST_POWER_UP_EN
    cif_power_down_host();
    #endif
}

void cif_battery_test_mode(bool test_mode)
{
    //vol--  status:0 done:0
    if(test_mode)
    {
        battery_test_mode = true;
    }
    else
    {
        battery_test_mode = false;
    }
    BK_LOGD(NULL,"test_mode is:%d\r\n",battery_test_mode);
}

