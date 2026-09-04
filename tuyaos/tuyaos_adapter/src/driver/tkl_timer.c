#include "tkl_timer.h"
#include <driver/timer.h>

/* private macros */
#define TIMER_DEV_NUM       3

/*
 *  SMP只能使用定时器3/4/5，对上层屏蔽细节，i
 *  上层代码传递定时器id 0/1/2，这里转换为对应的4/5/6*/
#define TIMER_DEV_OFFSET    3
#define BK_TIMER_IDX(x) (x + TIMER_DEV_OFFSET)
#define TY_TIMER_IDX(x) (x - TIMER_DEV_OFFSET)

/* private variables */
static TUYA_TIMER_BASE_CFG_T cfg_save[TIMER_DEV_NUM] = {
    {TUYA_TIMER_MODE_ONCE, NULL, NULL},
    {TUYA_TIMER_MODE_ONCE, NULL, NULL},
    {TUYA_TIMER_MODE_ONCE, NULL, NULL},
};

/* extern function */
// extern OSStatus bk_timer_read_cnt(uint8_t timer_id, uint32_t *timer_cnt);
// extern OSStatus bk_timer_initialize_us(uint8_t timer_id, uint32_t time_us, void *callback);

/**
 * @brief timer cb
 *
 * @param[in] args: hw timer id
 *
 * @return none
 */
static void __tkl_hw_timer_cb(void *args)
{
    TUYA_TIMER_NUM_E timer_id = (TUYA_TIMER_NUM_E)args;
    if ((timer_id < TIMER_DEV_NUM) || (timer_id > (TIMER_DEV_NUM + TIMER_DEV_OFFSET))) {
        return;
    }

    int idx = TY_TIMER_IDX(timer_id);

    if(cfg_save[idx].cb){
        cfg_save[idx].cb(cfg_save[idx].args);
    }
    return;
}

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

    cfg_save[timer_id].args = cfg->args;
    cfg_save[timer_id].cb = cfg->cb;
    cfg_save[timer_id].mode = cfg->mode;
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
    if (timer_id >= TIMER_DEV_NUM || us == 0) {
        bk_printf("start failed, invalid parameter, %d %d\r\n", timer_id, us);
        return OPRT_NOT_SUPPORTED;
    }

    int idx = BK_TIMER_IDX(timer_id);
    bk_timer_set_period_us(idx, 0);
    if(!(us % 1000)) {
        bk_timer_start(idx, (uint32_t)(us / 1000), (timer_isr_t)__tkl_hw_timer_cb);
    } else {
        bk_timer_start_us(idx, us, (timer_isr_t)__tkl_hw_timer_cb);
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
        bk_printf("stop failed, invalid parameter, %d\r\n", timer_id);
        return OPRT_NOT_SUPPORTED;
    }

    int idx = BK_TIMER_IDX(timer_id);
    bk_timer_stop(idx);
    bk_timer_set_period_us(idx, 0);

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
    if (timer_id >= TIMER_DEV_NUM) {
        bk_printf("deinit failed, invalid parameter, %d\r\n", timer_id);
        return OPRT_NOT_SUPPORTED;
    }

    bk_timer_stop(BK_TIMER_IDX(timer_id));
    return OPRT_OK;
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
    uint32_t count = 0;
    if (timer_id >= TIMER_DEV_NUM) {
        return OPRT_NOT_SUPPORTED;
    }

    count = bk_timer_get_cnt(BK_TIMER_IDX(timer_id));
    if (us != NULL) {
        *us = count / 26;
    } else {
        return OPRT_INVALID_PARM;
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
    uint32_t count = 0;
    if (timer_id >= TIMER_DEV_NUM) {
        return OPRT_NOT_SUPPORTED;
    }

    count = bk_timer_get_cnt(BK_TIMER_IDX(timer_id));
    if (us != NULL) {
        *us = count / 26;
    } else {
        return OPRT_INVALID_PARM;
    }

    return OPRT_OK;
}


