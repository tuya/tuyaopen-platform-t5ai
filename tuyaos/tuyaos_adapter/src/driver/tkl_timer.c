#include "tkl_timer.h"
// #include "_stdint.h"
#include <driver/timer.h>
// #include "BkDriverTimer.h"

/* private macros */
#define TIMER_DEV_NUM       4

/* private variables */
static TUYA_TIMER_BASE_CFG_T cfg_save;

/* extern function */
// extern OSStatus bk_timer_read_cnt(uint8_t timer_id, uint32_t *timer_cnt);
// extern OSStatus bk_timer_initialize_us(uint8_t timer_id, uint32_t time_us, void *callback);

/**
 * @brief timer init
 *
 * @param[in] timer_id timer id
 * @param[in] cfg timer configure
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_timer_init(TUYA_TIMER_NUM_E timer_id, TUYA_TIMER_BASE_CFG_T *cfg)
{
    if (timer_id >= TIMER_DEV_NUM) {
        return OPRT_NOT_SUPPORTED;
    }

    cfg_save.args = cfg->args;
    cfg_save.cb = cfg->cb;
    cfg_save.mode = cfg->mode;
    bk_timer_driver_init();
    return OPRT_OK;
}

/**
 * @brief timer start
 *
 * @param[in] timer_id timer id
 * @param[in] us when to start
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_timer_start(TUYA_TIMER_NUM_E timer_id, uint32_t us)
{
    TUYA_TIMER_BASE_CFG_T *cfg;

    if (timer_id >= TIMER_DEV_NUM) {
        return OPRT_NOT_SUPPORTED;
    }

    cfg = &cfg_save;
    if ((0 == timer_id) || (1 == timer_id))
    {
        bk_timer_start(timer_id, us, (timer_isr_t)cfg->cb);
    }
    else
    {
        if (us < 1000)
        {
            /* tuya timer2~timer3 can't not set cycle less than 1ms */
            return OPRT_INVALID_PARM;
        }
        bk_timer_start((timer_id + 2), us / 1000, (timer_isr_t)cfg->cb); //bk timer2、3 被系统占用了实际 bk timerID 为 timer4~timer5
        return OPRT_OK;
    }

    return OPRT_OK;
}

/**
 * @brief timer stop
 *
 * @param[in] timer_id timer id
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_timer_stop(TUYA_TIMER_NUM_E timer_id)
{
    if (timer_id >= TIMER_DEV_NUM) {
        return OPRT_NOT_SUPPORTED;
    }

    if ((0 == timer_id) || (1 == timer_id))
    {
        bk_timer_stop(timer_id);
    }
    else
    {
        bk_timer_stop(timer_id + 2); //bk timer2、3 被系统占用了实际 bk timerID 为 timer4~timer5
        return OPRT_OK;
    }

    return OPRT_OK;
}

/**
 * @brief timer deinit
 *
 * @param[in] timer_id timer id
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_timer_deinit(TUYA_TIMER_NUM_E timer_id)
{
    return bk_timer_driver_deinit();
}

/**
 * @brief timer get
 *
 * @param[in] timer_id timer id
 * @param[out] ms timer interval
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_timer_get(TUYA_TIMER_NUM_E timer_id, uint32_t *us)
{
    uint32_t count;

    if ((0 == timer_id) || (1 == timer_id))
    {
        count = bk_timer_get_cnt(timer_id);

        if (us != NULL) {
            *us = count / 26;
        }
    }
    else
    {
        return OPRT_NOT_SUPPORTED;
    }

    return OPRT_OK;
}

/**
 * @brief current timer get
 *
 * @param[in] timer_id timer id
 * @param[out] us timer
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_timer_get_current_value(TUYA_TIMER_NUM_E timer_id, uint32_t *us)
{
    return tkl_timer_get(timer_id, us);
}

