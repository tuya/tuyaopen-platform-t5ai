#ifndef __LOW_POWER_CORE_H__
#define __LOW_POWER_CORE_H__

typedef enum
{
	LOW_PWR_CORE_STATE_ENTER_DEEPSLEEP = 0,
	LOW_PWR_CORE_CTRL_CP2_STATE,    // 1
	LOW_PWR_CORE_CP2_RECOVERY,      // 2
	LOW_PWR_CORE_RTC_DEEPSLEEP,     // 3
	LOW_PWR_CORE_GET_CP_DATA,       // 4
	LOW_PWR_CORE_POWER_CTRL,        // 5
	LOW_PWR_CORE_CLK_CTRL,          // 6
	LOW_PWR_CORE_SLEEP_CTRL,        // 7
	LOW_PWR_CORE_FREQ_CTRL,         // 8
	LOW_PWR_CORE_EXTERNAL_LDO,      // 9
	LOW_PWR_CORE_PSRAM_POWER,       //10
	LOW_PWR_CORE_WAKEUP_SRC_CFG,    //11
	LOW_PWR_CORE_RTC_WAKEUPED,      //12
	LOW_PWR_CORE_GPIO_WAKEUPED,     //13
	LOW_PWR_CORE_STATE_MAX
}low_pwr_core_state_e;

typedef struct
{
	low_pwr_core_state_e event;
	uint32_t 			 param1;
	uint32_t             param2;
	uint32_t             param3;
} low_pwr_core_msg_t;
/**
 * @brief low pwr core init
 *
 * aov pm init
 *
 * @attention
 * - This API is to init low pwr core
 *
 * @param
 * -void
 * @return
 * - BK_OK: succeed
 * - others: other errors.
 */
bk_err_t bk_low_pwr_core_init();
/**
 * @brief send pwr_core msg
 *
 * send pwr_core msg
 *
 * @attention
 * - This API is to send low pwr_core msg
 *
 * @param
 * -msg
 * @return
 * - BK_OK: succeed
 * - others: other errors.
 */
bk_err_t bk_low_pwr_core_send_msg(low_pwr_core_msg_t *msg);
#endif
