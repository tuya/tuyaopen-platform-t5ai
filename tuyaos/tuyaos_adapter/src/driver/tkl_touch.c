#include "tkl_touch.h"

#include <os/mem.h>
#include <os/str.h>
#include <os/os.h>
#include <math.h>
#include "driver/touch.h"
#include "driver/touch_types.h"
#include "sys_driver.h"
#include "touch_driver.h"
#include "aon_pmu_driver.h"
#include "driver/timer.h"
#include "bk_saradc.h"
#include "driver/adc.h"
#include "tal_api.h"
#include "tkl_output.h"

/***********************************************************
*************************micro define***********************
***********************************************************/

#define FAST_RESPONSE_K 16 /* 快速响应IIR参数 */
#define FAST_RESPONSE_N 12 /* 快速响应权重 */
#define SLOW_RESPONSE_K 64 /* 慢速响应IIR参数 */
#define SLOW_RESPONSE_N 4  /* 慢速响应权重 */
#define GET_DATA_COUNT  5  /* init baseline get raw data count */

#define UPDATE_BASELINE_MAX 3    /* 更新基线计数最大次数 */
#define SNR                 0.8  /* 信噪比 */
#define CHANGE_THRESHOLD    0.6f /* 变化检测阈值 */
#define BUF_SIZE            16   /* 减小缓冲区提高响应速度 */

#define THRESHOLD_TOUCH_ON  0.6f /* 触摸检测阈值 */
#define THRESHOLD_TOUCH_OFF 0.5f /* 触摸释放阈值 (滞回) */
#define THRESHOLD_VAR       0.3f /* 方差阈值 */

#define DEBUG_ENABLE            0
#define TOUCH_SAMPLE_TIME       20
#define TOUCH_LONG_PRESSED_TIME 2000 /* 长按阈值时间 ms */ 
/***********************************************************
***********************variable define**********************
***********************************************************/
// 内部回调函数类型
typedef struct {
    TUYA_TOUCH_CALLBACK callback;
    VOID *arg;
} touch_callback_t;

// 触摸通道状态结构体
typedef struct {
    BOOL_T enabled;
    UINT32_T calibration_value;
    TUYA_TOUCH_DETECT_RANGE_E detect_range;
} touch_channel_state_t;

// 为每个通道定义独立的状态变量
typedef struct {
    UINT8_T touch_state; // 0: 未触摸, 1: 触摸
    UINT8_T touch_flag;
    float baseline;
    float raw_buf[BUF_SIZE];
    float filtered_buf[BUF_SIZE];
    float last_filtered_value;
    UINT8_T buf_index;
} touch_channel_data_t;

// 为每个通道维护独立的按键状态
typedef struct {
    UINT8_T key_state;      // 当前按键状态
    UINT8_T last_key_state; // 上一次按键状态
    UINT32_T press_count;   // 按下计数器
} key_state_t;

static touch_callback_t touch_callbacks[TUYA_TOUCH_CHANNEL_MAX] = {0};
static touch_channel_state_t touch_channel_states[TUYA_TOUCH_CHANNEL_MAX] = {0};
static BOOL_T touch_initialized = FALSE;
static UINT32_T current_enabled_channels = 0;
static touch_config_t touch_config;
static beken_thread_t __tkl_touch_scan_thread_handle = NULL;
static key_state_t key_states[TUYA_TOUCH_CHANNEL_MAX] = {0};
static touch_channel_data_t channel_data[TUYA_TOUCH_CHANNEL_MAX] = {0};
static float average_value[BUF_SIZE];
static float median_value[BUF_SIZE];
/***********************************************************
***********************function define**********************
***********************************************************/
float get_median_value(float find_array[], UINT8_T array_len);
double get_avg_value(float find_array[], UINT8_T array_len);
double get_variance_value(float array[], UINT8_T array_len);
double get_discrete_state(float value, double avg_value);
float get_iir_filter_value(float current_value, float last_value, UINT32_T k, UINT32_T n);
float get_square_wave_filter(float raw_value, float baseline_value, UINT8_T touch_id);
float get_adaptive_baseline(float current_value, float old_baseline, UINT8_T is_stable);
OPERATE_RET tkl_touch_get_single_calibration_value(UINT32_T channel_mask, float *value);
INT32_T find_index(float array[], UINT8_T array_len, float find_value);


void touch_parameter_init(UINT32_T channel_mask)
{
    UINT8_T i = 0;
    float raw_data[GET_DATA_COUNT] = {0};

    // 多次采样获得稳定基线
    while (i < GET_DATA_COUNT) {
        tkl_touch_get_single_calibration_value(channel_mask, &raw_data[i++]);
        rtos_thread_msleep(10); // 短暂延时确保采样稳定
    }

    /* 初始化基线和缓冲区 */
    for (i = 0; i < TUYA_TOUCH_CHANNEL_MAX; i++) {
        if (channel_mask & (1 << i)) {
            float init_baseline = get_avg_value(raw_data, GET_DATA_COUNT);
            channel_data[i].baseline = init_baseline;

            // 初始化缓冲区
            for (UINT8_T j = 0; j < BUF_SIZE; j++) {
                channel_data[i].raw_buf[j] = init_baseline;
                channel_data[i].filtered_buf[j] = init_baseline;
            }
            channel_data[i].last_filtered_value = init_baseline;
            channel_data[i].touch_state = 0;
            channel_data[i].buf_index = 0;

            PR_DEBUG("baseline[%d]: %f\r\n", i, channel_data[i].baseline);
        }
    }
}


UINT8_T __touch_status_process(UINT32_T touch_id)
{
    UINT8_T rt = 0;
    static float raw_value, filtered_value;
    static double variance_value_raw;
    
    // 确保touch_id在有效范围内
    if (touch_id >= TUYA_TOUCH_CHANNEL_MAX) {
        return 0;
    }

    // 1、读取原始数据
    tkl_touch_get_single_calibration_value(1 << touch_id, &raw_value);

    // 环形缓冲区索引更新
    channel_data[touch_id].buf_index = (channel_data[touch_id].buf_index + 1) % BUF_SIZE;
    channel_data[touch_id].raw_buf[channel_data[touch_id].buf_index] = raw_value;

    // 计算原始数据方差判断信号稳定性
    variance_value_raw = get_variance_value(channel_data[touch_id].raw_buf, BUF_SIZE);

    // 使用方波滤波算法
    filtered_value = get_square_wave_filter(raw_value, channel_data[touch_id].baseline, touch_id);

    // 触摸状态检测 - 使用滞回比较器避免抖动
    float temp = filtered_value - channel_data[touch_id].baseline;
    float touch_diff = median_value[channel_data[touch_id].buf_index] - channel_data[touch_id].baseline;
    touch_diff = temp > touch_diff ? temp : touch_diff;
    if (channel_data[touch_id].touch_state == 0) {
        // 当前未触摸状态，检测触摸
        if (touch_diff > THRESHOLD_TOUCH_ON) {
            channel_data[touch_id].touch_state = 1;
            channel_data[touch_id].touch_flag = 1;
            rt = 1;
            PR_DEBUG("Touch detected on channel %d!", touch_id);
            get_adaptive_baseline(filtered_value, channel_data[touch_id].baseline, 0);
        } else if (touch_diff < 0) {
            // get_adaptive_baseline(filtered_value, channel_data[touch_id].baseline, 2);
            channel_data[touch_id].baseline = filtered_value;
        } else {
            // 未触摸时缓慢更新基线
            if (variance_value_raw < THRESHOLD_VAR) {
                channel_data[touch_id].baseline =
                get_adaptive_baseline(filtered_value, channel_data[touch_id].baseline, 1);
            }
        }
    } else {
        // 当前触摸状态，检测释放
        if (touch_diff < THRESHOLD_TOUCH_OFF) {
            channel_data[touch_id].touch_state = 0;
            channel_data[touch_id].touch_flag = 0;
            PR_DEBUG("Touch released on channel %d!", touch_id);
            get_adaptive_baseline(filtered_value, channel_data[touch_id].baseline, 2);
        } else {
            get_adaptive_baseline(filtered_value, channel_data[touch_id].baseline, 0);
            rt = 1; // 持续触摸
        }
    }

#if DEBUG_ENABLE 
    printf("median_value=%lf, average_value=%lf\r\n", median_value[channel_data[touch_id].buf_index], average_value[channel_data[touch_id].buf_index]);
    printf("raw_buf=%f, iir_filter_buf=%f\r\n", channel_data[touch_id].raw_buf[channel_data[touch_id].buf_index], filtered_value);
    printf("baseline=%f, raw_value=%f\r\n", channel_data[touch_id].baseline, raw_value);
    printf("variance_value_raw=%lf,touch_state=%d,touch_flag=%d\r\n", variance_value_raw, channel_data[touch_id].touch_state, channel_data[touch_id].touch_flag);
    printf("channel=%d,key=%d\r\n", touch_id, key_states[touch_id].key_state);
    printf("touch_diff=%f\r\n", touch_diff);
#endif
    return rt;
}


// 内部中断处理函数
static void __tkl_touch_isr(void *param)
{
    bk_touch_clear_int(0xFFFF);

    UINT32_T channel = (UINT32_T)param;
    UINT32_T int_status = 0;

    // 获取中断状态
    int_status = bk_touch_get_int_status();
    PR_DEBUG("Touch interrupt: channel=%d, status=0x%x", channel, int_status);

    for (uint8_t ch = 0; ch < TUYA_TOUCH_CHANNEL_MAX; ch++) {
        if (touch_callbacks[ch].callback != NULL) {
            if (int_status & (1 << ch)) {
                touch_callbacks[ch].callback(ch, TUYA_TOUCH_EVENT_DOWN, touch_callbacks[ch].arg);
            } else {
                touch_callbacks[ch].callback(ch, TUYA_TOUCH_EVENT_UP, touch_callbacks[ch].arg);
            }
        }
    }
}

int count_set_bits(uint32_t n) {
    int count = 0;
    while (n) {
        n &= n - 1;  // 清除最低位的 1
        count++;
    }
    return count;
}

static void __tkl_touch_scan_thread(void)
{
    // 计算长按时间
    UINT32_T long_press_threshold = TOUCH_LONG_PRESSED_TIME / TOUCH_SAMPLE_TIME / 4 / count_set_bits(current_enabled_channels);

    while (1) {
        // 遍历所有启用的通道
        for (UINT8_T touch_id = 0; touch_id < TUYA_TOUCH_CHANNEL_MAX; touch_id++) {
            // 检查通道是否启用
            if (!(current_enabled_channels & (1 << touch_id))) {
                continue;
            }

            // 获取当前通道的触摸状态
            UINT8_T key = 0;
            if (__touch_status_process(touch_id)) {
                key = 1;
            }

            // 更新按键状态
            key_states[touch_id].key_state = key;

            // 状态机处理
            if (key_states[touch_id].last_key_state == 0 && key_states[touch_id].key_state == 1) {
                // 按键按下开始计数
                key_states[touch_id].press_count = 0;
                if (touch_callbacks[touch_id].callback != NULL) {
                        touch_callbacks[touch_id].callback(touch_id, TUYA_TOUCH_EVENT_DOWN,
                                                           touch_callbacks[touch_id].arg);
                }
            } else if (key_states[touch_id].last_key_state == 1 && key_states[touch_id].key_state == 0) {
                // 按键释放，判断是短按还是长按
                if (key_states[touch_id].press_count < long_press_threshold) {
                    // 短按事件
                    if (touch_callbacks[touch_id].callback != NULL) {
                        touch_callbacks[touch_id].callback(touch_id, TUYA_TOUCH_EVENT_UP,
                                                           touch_callbacks[touch_id].arg);
                    }
                }
                // 重置计数
                key_states[touch_id].press_count = 0;
            } else if (key_states[touch_id].last_key_state == 1 && key_states[touch_id].key_state == 1) {
                // 持续按下状态，检查是否达到长按阈值
                key_states[touch_id].press_count++;
                if (key_states[touch_id].press_count == long_press_threshold) {
                    // 长按事件
                    if (touch_callbacks[touch_id].callback != NULL) {
                        touch_callbacks[touch_id].callback(touch_id, TUYA_TOUCH_EVENT_LONG_PRESS,
                                                           touch_callbacks[touch_id].arg);
                    }
                }
            }

            // 保存当前状态作为下一次的上一次状态
            key_states[touch_id].last_key_state = key_states[touch_id].key_state;
        }

        rtos_thread_msleep(TOUCH_SAMPLE_TIME); // 提高采样频率到50Hz
    }
}

OPERATE_RET tkl_touch_init(UINT32_T channel_mask, TUYA_TOUCH_CONFIG_T *cfg)
{
    UINT32_T channel;

    if (NULL == cfg) {
        return OPRT_INVALID_PARM;
    }

    if (touch_initialized) {
        PR_ERR("Touch already initialized");
        return OPRT_OK;
    }

    // 初始化GPIO
    bk_touch_gpio_init(channel_mask);

    // 初始化滤波参数
    touch_parameter_init(channel_mask);

    // 初始化回调函数数组和通道状态
    for (channel = 0; channel < TUYA_TOUCH_CHANNEL_MAX; channel++) {
        touch_callbacks[channel].callback = NULL;
        touch_callbacks[channel].arg = NULL;
        touch_channel_states[channel].enabled = FALSE;
        touch_channel_states[channel].calibration_value = 0;
        touch_channel_states[channel].detect_range = TUYA_TOUCH_DETECT_RANGE_8PF;

        if (channel_mask & (1 << channel)) {
            touch_channel_states[channel].enabled = TRUE;
        }
    }
    bk_touch_enable(channel_mask);

    current_enabled_channels = channel_mask;
    touch_initialized = TRUE;

    // 转换配置参数
    switch (cfg->sensitivity_level) {
    case TUYA_TOUCH_SENSITIVITY_LEVEL_0:
        touch_config.sensitivity_level = TOUCH_SENSITIVITY_LEVLE_0;
        break;
    case TUYA_TOUCH_SENSITIVITY_LEVEL_1:
        touch_config.sensitivity_level = TOUCH_SENSITIVITY_LEVLE_1;
        break;
    case TUYA_TOUCH_SENSITIVITY_LEVEL_2:
        touch_config.sensitivity_level = TOUCH_SENSITIVITY_LEVLE_2;
        break;
    case TUYA_TOUCH_SENSITIVITY_LEVEL_3:
        touch_config.sensitivity_level = TOUCH_SENSITIVITY_LEVLE_3;
        break;
    default:
        return OPRT_INVALID_PARM;
    }

    switch (cfg->detect_threshold) {
    case TUYA_TOUCH_DETECT_THRESHOLD_0:
        touch_config.detect_threshold = TOUCH_DETECT_THRESHOLD_0;
        break;
    case TUYA_TOUCH_DETECT_THRESHOLD_1:
        touch_config.detect_threshold = TOUCH_DETECT_THRESHOLD_1;
        break;
    case TUYA_TOUCH_DETECT_THRESHOLD_2:
        touch_config.detect_threshold = TOUCH_DETECT_THRESHOLD_2;
        break;
    case TUYA_TOUCH_DETECT_THRESHOLD_3:
        touch_config.detect_threshold = TOUCH_DETECT_THRESHOLD_3;
        break;
    case TUYA_TOUCH_DETECT_THRESHOLD_4:
        touch_config.detect_threshold = TOUCH_DETECT_THRESHOLD_4;
        break;
    case TUYA_TOUCH_DETECT_THRESHOLD_5:
        touch_config.detect_threshold = TOUCH_DETECT_THRESHOLD_5;
        break;
    case TUYA_TOUCH_DETECT_THRESHOLD_6:
        touch_config.detect_threshold = TOUCH_DETECT_THRESHOLD_6;
        break;
    default:
        return OPRT_INVALID_PARM;
    }

    switch (cfg->detect_range) {
    case TUYA_TOUCH_DETECT_RANGE_8PF:
        touch_config.detect_range = TOUCH_DETECT_RANGE_8PF;
        break;
    case TUYA_TOUCH_DETECT_RANGE_12PF:
        touch_config.detect_range = TOUCH_DETECT_RANGE_12PF;
        break;
    case TUYA_TOUCH_DETECT_RANGE_19PF:
        touch_config.detect_range = TOUCH_DETECT_RANGE_19PF;
        break;
    case TUYA_TOUCH_DETECT_RANGE_27PF:
        touch_config.detect_range = TOUCH_DETECT_RANGE_27PF;
        break;
    default:
        return OPRT_INVALID_PARM;
    }

    // 配置触摸参数
    bk_touch_config(&touch_config);
#if (CONFIG_SOC_BK7236XX || CONFIG_SOC_BK7239XX || CONFIG_SOC_BK7286XX)
    bk_touch_set_test_mode(0, 0);
    bk_touch_set_calib_mode(0x3F, 9);
#endif

    PR_DEBUG("Touch init success, channel_mask=0x%x", channel_mask);
    return OPRT_OK;
}

OPERATE_RET tkl_touch_deinit(UINT32_T channel_mask)
{
    UINT32_T channel;

    if (!touch_initialized) {
        return OPRT_OK;
    }

    // 禁用中断和扫描模式
    bk_touch_int_enable(channel_mask, 0);
    bk_touch_scan_mode_enable(0);

    // 清除回调函数和通道状态
    for (channel = 0; channel < TUYA_TOUCH_CHANNEL_MAX; channel++) {
        if (channel_mask & (1 << channel)) {
            touch_callbacks[channel].callback = NULL;
            touch_callbacks[channel].arg = NULL;
            touch_channel_states[channel].enabled = FALSE;
            touch_channel_states[channel].calibration_value = 0;
        }
    }

    // 禁用触摸通道
    bk_touch_disable();

    current_enabled_channels &= ~channel_mask;
    if (current_enabled_channels == 0) {
        touch_initialized = FALSE;
    }

    PR_DEBUG("Touch deinit success, channel_mask=0x%x", channel_mask);
    return OPRT_OK;
}

OPERATE_RET tkl_touch_enable(UINT32_T channel_mask)
{
    bk_touch_enable(channel_mask);
    return OPRT_OK;
}

OPERATE_RET tkl_touch_disable(UINT32_T channel_mask)
{
    bk_touch_disable();
    return OPRT_OK;
}

OPERATE_RET tkl_touch_register_callback(UINT32_T channel_mask, TUYA_TOUCH_CALLBACK callback, VOID *arg)
{
    UINT32_T channel;

    // 注册扫描线程（只创建一次）
    if (__tkl_touch_scan_thread_handle == NULL) {
        rtos_create_thread(&__tkl_touch_scan_thread_handle, BEKEN_DEFAULT_WORKER_PRIORITY, "touch_scan",
                           (beken_thread_function_t)__tkl_touch_scan_thread, 4096, NULL);
    }

    // 注册触摸中断
    for (channel = 0; channel < TUYA_TOUCH_CHANNEL_MAX; channel++) {
        if (channel_mask & (1 << channel)) {
            touch_callbacks[channel].callback = callback;
            touch_callbacks[channel].arg = arg;

            // 注册中断处理函数
            bk_touch_register_touch_isr(1 << channel, __tkl_touch_isr, (void *)channel);
            bk_touch_int_enable(1 << channel, 1);
        }
    }
    return OPRT_OK;
}

OPERATE_RET tkl_touch_get_single_calibration_value(UINT32_T channel_mask, float *value)
{
    UINT32_T touch_id = 0;
    UINT32_T touch_crg_max = 0;
    UINT32_T multi_chann_value = 0xCF3F;
    UINT32_T cap_out, cap_out0, cap_out1;
    for (touch_id = 0; touch_id < 16; touch_id++) {
        if (touch_id == 6 || touch_id == 7 || touch_id == 13 || touch_id == 12) {
            continue;
        }
        if (channel_mask & (1 << touch_id)) {

            bk_touch_gpio_init(1 << touch_id);
            bk_touch_enable(1 << touch_id);
            bk_touch_register_touch_isr(1 << touch_id, __tkl_touch_isr, NULL);

            touch_config.detect_range = TOUCH_DETECT_RANGE_8PF;
            bk_touch_config(&touch_config);
            // PR_DEBUG("touch id is %d", touch_id);
            bk_touch_scan_mode_enable(0);
            bk_touch_calibration_start();
            *value = bk_touch_get_calib_value();
            cap_out0 = *value;
            // printf("capout0=%d\r\n", *value);
            if (*value >= 0x1F0) {
                touch_config.detect_range = TOUCH_DETECT_RANGE_12PF;
                bk_touch_config(&touch_config);
                bk_touch_calibration_start();
                *value = bk_touch_get_calib_value();
                cap_out1 = *value;
                // printf("capout1=%d\r\n", *value);
                if (*value >= 0x1F0) {
                    touch_config.detect_range = TOUCH_DETECT_RANGE_19PF;
                    bk_touch_config(&touch_config);
                    bk_touch_calibration_start();
                    *value = bk_touch_get_calib_value();
                    if (*value >= 0x1F0) {
                        touch_config.detect_range = TOUCH_DETECT_RANGE_27PF;
                        bk_touch_config(&touch_config);
                        bk_touch_calibration_start();
                        *value = bk_touch_get_calib_value();
                        if (*value >= 0x1F0) {
                            PR_ERR("Calibration value is out of the detect range, the channel [%d] cannot be used, "
                                "please select the other channel!",
                                touch_id);
                        }
                    }
                }
            }

            if (touch_config.detect_range == TOUCH_DETECT_RANGE_8PF) {
                *value = (float)((*value) / 512.0f * 8);
            } else if (touch_config.detect_range == TOUCH_DETECT_RANGE_12PF) {
                *value = (float)((*value) / 512.0f * 12);
            } else if (touch_config.detect_range == TOUCH_DETECT_RANGE_19PF) {
                *value = (float)((*value) / 512.0f * 19);
            } else if (touch_config.detect_range == TOUCH_DETECT_RANGE_27PF) {
                *value = (float)((*value) / 512.0f * 27);
            }
        }
        touch_channel_states[touch_id].calibration_value = *value;
        touch_channel_states[touch_id].detect_range = touch_config.detect_range;
        bk_touch_int_enable(1 << touch_id, 1);
        // PR_DEBUG("touch[%d] crg = %d, calibration value = %x !\r\n", touch_id, touch_crg[touch_id], *value);
    }
    for (touch_id = 0; touch_id < 16; touch_id++) {
        if (touch_id == 6 || touch_id == 7 || touch_id == 13 || touch_id == 12) {
            continue;
        }

        if (touch_crg_max < touch_channel_states[touch_id].detect_range) {
            touch_crg_max = touch_channel_states[touch_id].detect_range;
        }
    }
    // PR_DEBUG("touch_crg_max = %d\r\n", touch_crg_max);

    for (touch_id = 0; touch_id < 16; touch_id++) {
        if (touch_id == 6 || touch_id == 7 || touch_id == 13 || touch_id == 12) {
            continue;
        }

        if (touch_crg_max != touch_channel_states[touch_id].detect_range) {
            bk_touch_enable(1 << touch_id);
            touch_config.detect_range = touch_crg_max;
            bk_touch_config(&touch_config);
            bk_touch_calibration_start();
        }
    }

    bk_touch_scan_mode_multi_channl_set(multi_chann_value);
    bk_touch_scan_mode_enable(1);
    bk_touch_int_enable(multi_chann_value, 1);
    return OPRT_OK;
}

UINT8_T tkl_touch_get_single_channel_status(UINT8_T touch_id)
{
    if (NULL != __tkl_touch_scan_thread_handle) {
        return channel_data[touch_id].touch_state;
    }
    else {
        return __touch_status_process(touch_id);
    }
}

OPERATE_RET tkl_touch_scan_mode_enable(BOOL_T enable)
{
    if (enable) {
        bk_touch_scan_mode_enable(1);
    } else {
        bk_touch_scan_mode_enable(0);
    }

    return OPRT_OK;
}

OPERATE_RET tkl_touch_scan_mode_multi_channel_set(UINT32_T channel_mask)
{
    bk_touch_scan_mode_multi_channl_set(channel_mask);
    return OPRT_OK;
}

OPERATE_RET tkl_touch_clear_interrupt(UINT32_T channel_mask)
{
    bk_touch_clear_int(channel_mask);
    return OPRT_OK;
}

OPERATE_RET tkl_touch_interrupt_enable(UINT32_T channel_mask, BOOL_T enable)
{
    if (enable) {
        bk_touch_int_enable(channel_mask, 1);
    } else {
        bk_touch_int_enable(channel_mask, 0);
    }

    return OPRT_OK;
}

/**
 * @brief 获取通道检测范围
 *
 * @param[in] channel 通道号
 * @param[out] detect_range 检测范围
 *
 * @return OPRT_OK: 成功，其他: 失败
 */
OPERATE_RET tkl_touch_get_channel_detect_range(UINT32_T channel, TUYA_TOUCH_DETECT_RANGE_E *detect_range)
{
    if (channel >= TUYA_TOUCH_CHANNEL_MAX || detect_range == NULL) {
        return OPRT_INVALID_PARM;
    }

    if (!touch_channel_states[channel].enabled) {
        return OPRT_NOT_SUPPORTED;
    }

    *detect_range = touch_channel_states[channel].detect_range;
    return OPRT_OK;
}

#if 1
/**
 * @brief get max value in input array
 *
 * @param[in] find_array: arrays to be found
 * @param[in] array_len: find array length
 * @return the max value in the input array
 */
float get_max_value(float find_array[], UINT8_T array_len)
{
    float max_value;
    UINT8_T find_index = 0;

    if (NULL == find_array || array_len <= 0) {
        return 0;
    }

    max_value = find_array[0];
    for (find_index = 1; find_index < array_len; find_index++) {
        if (max_value < find_array[find_index]) {
            max_value = find_array[find_index];
        }
    }

    return max_value;
}

/**
 * @brief get min value in input array
 *
 * @param[in] find_array: arrays to be found
 * @param[in] array_len: find array length
 * @return the min value in the input array
 */
float get_min_value(float find_array[], UINT8_T array_len)
{
    float min_value;
    UINT8_T find_index = 0;

    if (NULL == find_array || array_len <= 0) {
        return 0;
    }

    min_value = find_array[0];
    for (find_index = 1; find_index < array_len; find_index++) {
        if (min_value > find_array[find_index]) {
            min_value = find_array[find_index];
        }
    }

    return min_value;
}

/**
 * @brief get median value
 *
 * @param[in] find_array: arrays to be found
 * @param[in] array_len: find array length
 * @return median value
 */
float get_median_value(float find_array[], UINT8_T array_len)
{
    float temp_value;
    UINT8_T i = 0, j = 0;

    if (NULL == find_array || array_len <= 0) {
        return 0;
    }

    UINT8_T mediam_index = (array_len - 1) / 2;

    /* Bubble Sort */
    for (i = 0; i < array_len - 1; i++) {
        for (j = 1; j < array_len - i; j++) {
            if (find_array[j] < find_array[j - 1]) {
                temp_value = find_array[j - 1];
                find_array[j - 1] = find_array[j];
                find_array[j] = temp_value;
            }
        }
    }

    return find_array[mediam_index];
}

/**
 * @brief get average
 *
 * @param[in] array: find the average of the array
 * @param[in] array_len: array length
 * @return average value
 */
double get_avg_value(float array[], UINT8_T array_len)
{
    double avg_value;
    float sum = 0;
    UINT8_T i;

    if (NULL == array || array_len <= 0) {
        return 0;
    }

    for (i = 0; i < array_len; i++) {
        sum += array[i];
    }
    avg_value = (sum * 1.0) / (array_len * 1.0);
    // printf("sum %d, avg:%lf\r\n", sum, avg_value);

    return avg_value;
}

/**
 * @brief get variance value
 *
 * @param[in] array: find the variance of the array
 * @param[in] array_len: array length
 * @return variance value
 */
double get_variance_value(float array[], UINT8_T array_len)
{
    double variance_value = 0;
    long double square_sum = 0;

    if (NULL == array || array_len <= 0) {
        return 0;
    }

    double avg_value = get_avg_value(array, array_len);

    UINT8_T i;
    for (i = 0; i < array_len; i++) {
        square_sum += (array[i] - avg_value) * (array[i] - avg_value);
    }
    variance_value = (square_sum / array_len);

    return variance_value;
}

/**
 * @brief Get the discrete state of a single number
 *
 * @param[in] value:
 * @param[in] avg_value: average value
 * @return
 */
double get_discrete_state(float value, double avg_value)
{
    double discrete_value = 0;

    if (value > avg_value) {
        discrete_value = value - avg_value;
    } else {
        discrete_value = avg_value - value;
    }

    return discrete_value;
}

/**
 * @brief get iir filter value
 *
 * @param[in] current_value: now raw data
 * @param[in] last_value: last iir filter data
 * @return current iir filter value
 */
float get_iir_filter_value(float current_value, float last_value, UINT32_T k, UINT32_T n)
{
    float result = 0;
    double temp_value = 0.0;
    temp_value = (1.0 / k) * (n * (double)current_value + (k - n) * (double)last_value);
    result = (float)temp_value;
    return result;
}

/**
 * @brief 方波滤波算法 - 快速响应触摸变化
 *
 * @param[in] raw_value: 原始数据
 * @param[in] baseline_value: 基线值
 * @param[in] touch_id: 触摸通道ID
 * @return 滤波后的值
 */
float get_square_wave_filter(float raw_value, float baseline_value, UINT8_T touch_id)
{
    float change_rate = fabs(raw_value - channel_data[touch_id].last_filtered_value);
    float filtered_result;
    
    // 根据变化率和当前状态选择滤波参数
    if (change_rate > CHANGE_THRESHOLD) {
        // 检测到快速变化，使用快速响应滤波
        if (channel_data[touch_id].touch_state == 0 && raw_value > baseline_value + THRESHOLD_TOUCH_ON) {
            // 触摸上升沿 - 极快响应
            filtered_result = get_iir_filter_value(raw_value, channel_data[touch_id].last_filtered_value, 6, 5);
        } else if (channel_data[touch_id].touch_state == 1 && raw_value < baseline_value + THRESHOLD_TOUCH_OFF) {
            // 触摸下降沿 - 极快响应
            filtered_result = get_iir_filter_value(raw_value, channel_data[touch_id].last_filtered_value, 6, 5);
        } else {
            // 其他快速变化
            filtered_result = get_iir_filter_value(raw_value, channel_data[touch_id].last_filtered_value,
                                                   FAST_RESPONSE_K, FAST_RESPONSE_N);
        }
    } else {
        // 缓慢变化，使用慢速滤波保持稳定
        filtered_result = get_iir_filter_value(raw_value, channel_data[touch_id].last_filtered_value, SLOW_RESPONSE_K,
                                               SLOW_RESPONSE_N);
        
        // 均值滤波
        average_value[channel_data[touch_id].buf_index] = get_avg_value(channel_data[touch_id].filtered_buf, BUF_SIZE);
        // 中值滤波
        median_value[channel_data[touch_id].buf_index] = get_median_value(channel_data[touch_id].filtered_buf, BUF_SIZE);

        // filtered_result = median_value[channel_data[touch_id].buf_index];
        // if (channel_data[touch_id].touch_state == 1) {
        //     // 中值滤波
        //     median_value[channel_data[touch_id].buf_index] = get_median_value(channel_data[touch_id].filtered_buf, BUF_SIZE);
        //     filtered_result = median_value[channel_data[touch_id].buf_index];

        //     channel_data[touch_id].last_filtered_value = median_value[channel_data[touch_id].buf_index];
        //     return filtered_result;
        // }
    }
    channel_data[touch_id].filtered_buf[channel_data[touch_id].buf_index] = filtered_result;
    channel_data[touch_id].last_filtered_value = filtered_result;
    return filtered_result;
}

/**
 * @brief 自适应基线更新
 *
 * @param[in] current_value: 当前滤波值
 * @param[in] old_baseline: 旧基线值
 * @param[in] is_stable: 是否稳定状态
 * @return 新基线值
 */
float get_adaptive_baseline(float current_value, float old_baseline, UINT8_T is_stable)
{
#if DEBUG_ENABLE
    printf("is_stable=%d\r\n", is_stable);
#endif
    if (is_stable == 2) {
        return get_iir_filter_value(current_value, old_baseline, 8, 6);
    }
    else if (is_stable == 1) {
        // 稳定状态下缓慢更新基线
        return get_iir_filter_value(current_value, old_baseline, 256, 1);
    } else {
        // 不稳定状态下不更新基线
        return old_baseline;
    }
}

INT32_T find_index(float array[], UINT8_T array_len, float find_value)
{
    UINT8_T i;

    if (NULL == array || array_len <= 0) {
        return -1;
    }

    for (i = 0; i < array_len; i++) {
        if (array[i] == find_value) {
            return i;
        }
    }

    return -2; /*not find */
}

#endif

#if 0
float iir_x[11][3] = {0};
float iir_y[11][2] = {0};
float iir_y_x[11][3] = {0};
float iir_y_y[11][2] = {0};
uint32_t g_gain_s = 0;
UINT8 g_num = 0;
UINT8 g_touch_capa_cali_flag = 0;
beken_timer_t touch_capa_cali_tmr = {0};
beken_timer_t touch_tmr = {0};

// static beken_thread_t touch_digital_tube_disp_thread_hdl = NULL;
extern void delay(int num);
extern uint32_t s_touch_channel;

static void cli_touch_help(void)
{
    PR_DEBUG("touch_single_channel_calib_mode_test {0|1|2|...|15} {0|1|2|3}\r\n");
    PR_DEBUG("touch_single_channel_manul_mode_test {0|1|...|15} {calibration_value}\r\n");
    PR_DEBUG("touch_multi_channel_scan_mode_test {start|stop} {0|1|2|3}\r\n");
    PR_DEBUG("touch_single_channel_multi_calib_test {0|1|...|15} {0|1|2|3}");
}

static void cli_touch_isr(void *param)
{
    PR_NOTICE("touch isr in");
    uint32_t int_status = 0;
    int_status = bk_touch_get_int_status();
    printf("interrupt status = %x\r\n", int_status);
}

static void touch_cyclic_calib_timer_isr(timer_id_t chan)
{
    uint32_t cap_out = 0;
    uint32_t touch_id = 0;
    uint32_t touch_crg[16] = {0};
    uint32_t touch_crg_max = 0;
    uint32_t multi_chann_value = 0xffff;
    touch_config_t touch_config;

    PR_DEBUG("multi_channel_cyclic_calib_test start!\r\n");

    bk_touch_clear_int(multi_chann_value);
    bk_touch_int_enable(multi_chann_value, 0);
    bk_touch_scan_mode_enable(0);
    bk_touch_scan_mode_multi_channl_set(0);
    bk_touch_enable(0);

    for (touch_id = 0; touch_id < 16; touch_id++) {
        bk_touch_enable(1 << touch_id);

        touch_config.sensitivity_level = g_gain_s;
        touch_config.detect_threshold = TOUCH_DETECT_THRESHOLD_6;
        touch_config.detect_range = TOUCH_DETECT_RANGE_8PF;
        bk_touch_config(&touch_config);

        bk_touch_calibration_start();
        cap_out = bk_touch_get_calib_value();
        if (cap_out >= 0x1F0) {
            touch_config.detect_range = TOUCH_DETECT_RANGE_12PF;
            bk_touch_config(&touch_config);
            bk_touch_calibration_start();
            cap_out = bk_touch_get_calib_value();
            if (cap_out >= 0x1F0) {
                touch_config.detect_range = TOUCH_DETECT_RANGE_19PF;
                bk_touch_config(&touch_config);
                bk_touch_calibration_start();
                cap_out = bk_touch_get_calib_value();
                if (cap_out >= 0x1F0) {
                    touch_config.detect_range = TOUCH_DETECT_RANGE_27PF;
                    bk_touch_config(&touch_config);
                    bk_touch_calibration_start();
                    cap_out = bk_touch_get_calib_value();
                    if (cap_out >= 0x1F0) {
                        TOUCH_LOGE("Calibration value is out of the detect range, the channel cannot be used, please "
                                   "select the other channel!\r\n");
                        return;
                    }
                }
            }
        }
        touch_crg[touch_id] = touch_config.detect_range;
        PR_DEBUG("touch[%d] crg = %d, calibration value = %x !\r\n", touch_id, touch_crg[touch_id], cap_out);
        delay(1000);
    }

    for (touch_id = 0; touch_id < 16; touch_id++) {
        if (touch_crg_max < touch_crg[touch_id]) {
            touch_crg_max = touch_crg[touch_id];
        }
    }
    PR_DEBUG("touch_crg_max = %d\r\n", touch_crg_max);

    for (touch_id = 0; touch_id < 16; touch_id++) {
        if (touch_crg_max != touch_crg[touch_id]) {
            bk_touch_enable(1 << touch_id);
            touch_config.detect_range = touch_crg_max;
            bk_touch_config(&touch_config);
            bk_touch_calibration_start();
        }
    }
    bk_touch_scan_mode_multi_channl_set(multi_chann_value);
    bk_touch_scan_mode_enable(1);
    bk_touch_int_enable(multi_chann_value, 1);
}

static void touch_digital_tube_disp_main(void)
{
    bk_touch_digital_tube_init();

    while (1) {
        bk_touch_digital_tube_display(s_touch_channel);
    }
}

bk_err_t bk_touch_digital_tube_display_init(void)
{
    bk_err_t ret = BK_OK;

    ret = rtos_create_thread(&touch_digital_tube_disp_thread_hdl, BEKEN_DEFAULT_WORKER_PRIORITY,
                             "touch_digital_tube_disp", (beken_thread_function_t)touch_digital_tube_disp_main, 4096,
                             NULL);
    if (ret != kNoErr) {
        PR_NOTICE(NULL, "create touch digital tube disp task failed!\r\n");
        touch_digital_tube_disp_thread_hdl = NULL;
    }

    return ret;
}

void cli_touch_single_channel_calib_mode_test_cmd()
{
    uint32_t touch_id = 0;
    uint32_t cap_out = 0;
    uint32_t gain_s = 0;
    touch_config_t touch_config;

    // if (argc != 3) {
    // 	cli_touch_help();
    // 	return;
    // }

    // touch_id = os_strtoul(argv[1], NULL, 10) & 0xFF;
    // gain_s = os_strtoul(argv[2], NULL, 10) & 0xFF;
    touch_id = 1;
    gain_s = 1;
    if (touch_id >= 0 && touch_id < 16) {
        PR_DEBUG("touch single channel calib mode test %d start!\r\n", touch_id);
        bk_touch_gpio_init(1 << touch_id);
        bk_touch_enable(1 << touch_id);
        bk_touch_register_touch_isr(1 << touch_id, cli_touch_isr, NULL);

        touch_config.sensitivity_level = gain_s;
        touch_config.detect_threshold = TOUCH_DETECT_THRESHOLD_6;
        touch_config.detect_range = TOUCH_DETECT_RANGE_8PF;
        bk_touch_config(&touch_config);
#if (CONFIG_SOC_BK7236XX || CONFIG_SOC_BK7239XX || CONFIG_SOC_BK7286XX)
        bk_touch_set_test_mode(0, 0);
        bk_touch_set_calib_mode(0x3F, 9);
#endif

        bk_touch_scan_mode_enable(0);
        bk_touch_calibration_start();
        cap_out = bk_touch_get_calib_value();
        printf("cap_out0=%x\r\n", cap_out);
        if (cap_out >= 0x1F0) {
            touch_config.detect_range = TOUCH_DETECT_RANGE_12PF;
            bk_touch_config(&touch_config);
            bk_touch_calibration_start();
            cap_out = bk_touch_get_calib_value();
            printf("cap_out1=%x\r\n", cap_out);
            if (cap_out >= 0x1F0) {
                touch_config.detect_range = TOUCH_DETECT_RANGE_19PF;
                bk_touch_config(&touch_config);
                bk_touch_calibration_start();
                cap_out = bk_touch_get_calib_value();
                printf("cap_out2=%x\r\n", cap_out);
                if (cap_out >= 0x1F0) {
                    touch_config.detect_range = TOUCH_DETECT_RANGE_27PF;
                    bk_touch_config(&touch_config);
                    bk_touch_calibration_start();
                    cap_out = bk_touch_get_calib_value();
                    printf("cap_out3=%x\r\n", cap_out);
                    if (cap_out >= 0x1F0) {
                        TOUCH_LOGE("Calibration value is out of the detect range, the channel cannot be used, please "
                                   "select the other channel!\r\n");
                        // return;
                    }
                }
            }
        }
        UINT32_T touch_status = bk_touch_get_touch_status();
        PR_NOTICE("touch status : 0x%X\n", touch_status);
        bk_touch_int_enable(1 << touch_id, 1);
    } else {
        TOUCH_LOGE("unsupported touch channel selection command!\r\n");
    }
}

void cli_touch_single_channel_manul_mode_test_cmd()
{
    uint32_t calib_value = 0;
    uint32_t cap_out = 0;
    uint32_t touch_id = 0;
    touch_config_t touch_config;

    // if (argc != 3) {
    // 	cli_touch_help();
    // 	return;
    // }

    // touch_id = os_strtoul(argv[1], NULL, 16) & 0xFF;
    touch_id = 1;
    if (touch_id >= 0 && touch_id < 16) {
        PR_DEBUG("touch single channel manul mode test %d start!\r\n", touch_id);
        bk_touch_gpio_init(1 << touch_id);
        bk_touch_enable(1 << touch_id);
        bk_touch_register_touch_isr(1 << touch_id, cli_touch_isr, NULL);

        touch_config.sensitivity_level = TOUCH_SENSITIVITY_LEVLE_3;
        touch_config.detect_threshold = TOUCH_DETECT_THRESHOLD_6;
        touch_config.detect_range = TOUCH_DETECT_RANGE_27PF;
        bk_touch_config(&touch_config);
        bk_touch_scan_mode_enable(0);

        calib_value = 0x800;
        PR_DEBUG("calib_value = %x\r\n", calib_value);
        bk_touch_manul_mode_enable(calib_value);
        bk_touch_int_enable(1 << touch_id, 1);
        cap_out = bk_touch_get_calib_value();
        printf("cap_out=%x\r\n", cap_out);
        if (calib_value == cap_out) {
            PR_DEBUG("single channel manul mode test is successful!\r\n");
        } else {
            TOUCH_LOGE("single channel manul mode test is failed!\r\n");
            TOUCH_LOGE("please input larger calibration value!\r\n");
            bk_touch_manul_mode_disable();
            bk_touch_disable();
        }
    } else {
        TOUCH_LOGE("unsupported touch channel selection command!\r\n");
    }
}

void cli_touch_multi_channel_scan_mode_test_cmd()
{
    bk_err_t ret = BK_OK;

    uint32_t multi_chann_value = 0xCF3F;
    uint32_t touch_crg[16] = {0};
    uint32_t touch_crg_max = 0;
    uint32_t touch_id = 0;
    uint32_t cap_out = 0;
    uint32_t gain_s = 0;
    touch_config_t touch_config;

    // if (argc != 3) {
    // 	cli_touch_help();
    // 	return;
    // }

    // ret = bk_touch_digital_tube_display_init();
    // if (ret != BK_OK) {
    // 	PR_NOTICE(NULL, "init touch digital tube display task failed!\r\n");
    // 	return;
    // }

    // if (os_strcmp(argv[1], "start") == 0) {
    if (1) {
        PR_DEBUG("multi_channel_scan_mode_test start!\r\n");
        // gain_s = os_strtoul(argv[2], NULL, 10) & 0xFF;
        gain_s = 3;
        for (touch_id = 0; touch_id < 16; touch_id++) {
            if (touch_id == 6 || touch_id == 7 || touch_id == 13 || touch_id == 12) {
                continue;
            }
            bk_touch_gpio_init(1 << touch_id);
            bk_touch_enable(1 << touch_id);
            bk_touch_register_touch_isr(1 << touch_id, cli_touch_isr, NULL);

            touch_config.sensitivity_level = gain_s;
            touch_config.detect_threshold = TOUCH_DETECT_THRESHOLD_6;
            touch_config.detect_range = TOUCH_DETECT_RANGE_8PF;
            bk_touch_config(&touch_config);

            bk_touch_scan_mode_enable(0);
            bk_touch_calibration_start();
            cap_out = bk_touch_get_calib_value();
            if (cap_out >= 0x1F0) {
                touch_config.detect_range = TOUCH_DETECT_RANGE_12PF;
                bk_touch_config(&touch_config);
                bk_touch_calibration_start();
                cap_out = bk_touch_get_calib_value();
                if (cap_out >= 0x1F0) {
                    touch_config.detect_range = TOUCH_DETECT_RANGE_19PF;
                    bk_touch_config(&touch_config);
                    bk_touch_calibration_start();
                    cap_out = bk_touch_get_calib_value();
                    if (cap_out >= 0x1F0) {
                        touch_config.detect_range = TOUCH_DETECT_RANGE_27PF;
                        bk_touch_config(&touch_config);
                        bk_touch_calibration_start();
                        cap_out = bk_touch_get_calib_value();
                        if (cap_out >= 0x1F0) {
                            TOUCH_LOGE("Calibration value is out of the detect range, the channel [%d] cannot be used, "
                                       "please select the other channel!\r\n",
                                       touch_id);
                            // return;
                        }
                    }
                }
            }
            touch_crg[touch_id] = touch_config.detect_range;
            PR_DEBUG("touch[%d] crg = %d, calibration value = %x !\r\n", touch_id, touch_crg[touch_id], cap_out);
            delay(1000);
        }

        for (touch_id = 0; touch_id < 16; touch_id++) {
            if (touch_id == 6 || touch_id == 7 || touch_id == 13 || touch_id == 12) {
                continue;
            }

            if (touch_crg_max < touch_crg[touch_id]) {
                touch_crg_max = touch_crg[touch_id];
            }
        }
        PR_DEBUG("touch_crg_max = %d\r\n", touch_crg_max);

        for (touch_id = 0; touch_id < 16; touch_id++) {
            if (touch_id == 6 || touch_id == 7 || touch_id == 13 || touch_id == 12) {
                continue;
            }

            if (touch_crg_max != touch_crg[touch_id]) {
                bk_touch_enable(1 << touch_id);
                touch_config.detect_range = touch_crg_max;
                bk_touch_config(&touch_config);
                bk_touch_calibration_start();
            }
        }

        bk_touch_scan_mode_multi_channl_set(multi_chann_value);
        bk_touch_scan_mode_enable(1);
        bk_touch_int_enable(multi_chann_value, 1);
    }
    // else if (os_strcmp(argv[1], "stop") == 0) {
    // 	PR_DEBUG("multi_channel_scan_mode_test stop!\r\n");
    // 	bk_touch_scan_mode_enable(0);
    // 	bk_touch_disable();
    // }
}

void cli_touch_single_channel_multi_calib_test_cmd()
{
    uint32_t touch_id = 0;
    uint32_t cap_out = 0;
    uint32_t gain_s = 0;
    uint32_t count = 0;
    touch_config_t touch_config;

    // if (argc != 3) {
    // 	cli_touch_help();
    // 	return;
    // }

    // touch_id = os_strtoul(argv[1], NULL, 10) & 0xFF;
    // gain_s = os_strtoul(argv[2], NULL, 10) & 0xFF;
    touch_id = 1;
    gain_s = 3;
    if (touch_id >= 0 && touch_id < 16) {
        PR_DEBUG("touch single channel calib mode test %d start!\r\n", touch_id);
        bk_touch_gpio_init(1 << touch_id);
        bk_touch_enable(1 << touch_id);
        bk_touch_register_touch_isr(1 << touch_id, cli_touch_isr, NULL);

        touch_config.sensitivity_level = gain_s;
        touch_config.detect_threshold = TOUCH_DETECT_THRESHOLD_6;
        touch_config.detect_range = TOUCH_DETECT_RANGE_8PF;
        bk_touch_config(&touch_config);

        bk_touch_scan_mode_enable(0);
        bk_touch_calibration_start();
        cap_out = bk_touch_get_calib_value();
        PR_DEBUG("cap_out0 = %x\r\n", cap_out);
        if (cap_out >= 0x1F0) {
            touch_config.detect_range = TOUCH_DETECT_RANGE_12PF;
            bk_touch_config(&touch_config);
            bk_touch_calibration_start();
            cap_out = bk_touch_get_calib_value();
            PR_DEBUG("cap_out1 = %x\r\n", cap_out);
            if (cap_out >= 0x1F0) {
                touch_config.detect_range = TOUCH_DETECT_RANGE_19PF;
                bk_touch_config(&touch_config);
                bk_touch_calibration_start();
                cap_out = bk_touch_get_calib_value();
                PR_DEBUG("cap_out2 = %x\r\n", cap_out);
                if (cap_out >= 0x1F0) {
                    touch_config.detect_range = TOUCH_DETECT_RANGE_27PF;
                    bk_touch_config(&touch_config);
                    bk_touch_calibration_start();
                    cap_out = bk_touch_get_calib_value();
                    PR_DEBUG("cap_out3 = %x\r\n", cap_out);
                    if (cap_out >= 0x1F0) {
                        TOUCH_LOGE("Calibration value is out of the detect range, the channel cannot be used, please "
                                   "select the other channel!\r\n");
                        return;
                    }
                }
            }
        }

        for (count = 0; count < 5000; count++) {
            bk_touch_calibration_start();
            cap_out = bk_touch_get_calib_value();
            printf("%d\n", cap_out);
            delay(1000);
        }

        bk_touch_int_enable(1 << touch_id, 1);
    } else {
        TOUCH_LOGE("unsupported touch channel selection command!\r\n");
    }
}

void cli_touch_multi_channel_cyclic_calib_test_cmd()
{
    int ret = 0;
    uint32_t multi_chann_value = 0xffff;
    uint32_t touch_crg[16] = {0};
    uint32_t touch_crg_max = 0;
    uint32_t touch_id = 0;
    uint32_t cap_out = 0;
    touch_config_t touch_config;

    // if (argc != 3) {
    // 	cli_touch_help();
    // 	return;
    // }

    // if (os_strcmp(argv[1], "start") == 0) {
    if (1) {
        // g_gain_s = os_strtoul(argv[2], NULL, 10) & 0xFF;
        g_gain_s = 3;
        for (touch_id = 0; touch_id < 16; touch_id++) {
            bk_touch_gpio_init(1 << touch_id);
            bk_touch_enable(1 << touch_id);
            bk_touch_register_touch_isr(1 << touch_id, cli_touch_isr, NULL);

            touch_config.sensitivity_level = g_gain_s;
            touch_config.detect_threshold = TOUCH_DETECT_THRESHOLD_6;
            touch_config.detect_range = TOUCH_DETECT_RANGE_8PF;
            bk_touch_config(&touch_config);

            bk_touch_scan_mode_enable(0);
            bk_touch_calibration_start();
            cap_out = bk_touch_get_calib_value();
            if (cap_out >= 0x1F0) {
                touch_config.detect_range = TOUCH_DETECT_RANGE_12PF;
                bk_touch_config(&touch_config);
                bk_touch_calibration_start();
                cap_out = bk_touch_get_calib_value();
                if (cap_out >= 0x1F0) {
                    touch_config.detect_range = TOUCH_DETECT_RANGE_19PF;
                    bk_touch_config(&touch_config);
                    bk_touch_calibration_start();
                    cap_out = bk_touch_get_calib_value();
                    if (cap_out >= 0x1F0) {
                        touch_config.detect_range = TOUCH_DETECT_RANGE_27PF;
                        bk_touch_config(&touch_config);
                        bk_touch_calibration_start();
                        cap_out = bk_touch_get_calib_value();
                        if (cap_out >= 0x1F0) {
                            TOUCH_LOGE("Calibration value is out of the detect range, the channel [%d] cannot be used, "
                                       "please select the other channel!\r\n",
                                       touch_id);
                            // return;
                        }
                    }
                }
            }
            touch_crg[touch_id] = touch_config.detect_range;
            PR_DEBUG("touch[%d] crg = %d, calibration value = %x !\r\n", touch_id, touch_crg[touch_id], cap_out);
            // delay(1000);
        }

        for (touch_id = 0; touch_id < 16; touch_id++) {
            if (touch_crg_max < touch_crg[touch_id]) {
                touch_crg_max = touch_crg[touch_id];
            }
        }
        PR_DEBUG("touch_crg_max = %d\r\n", touch_crg_max);

        for (touch_id = 0; touch_id < 16; touch_id++) {
            if (touch_crg_max != touch_crg[touch_id]) {
                bk_touch_enable(1 << touch_id);
                touch_config.detect_range = touch_crg_max;
                bk_touch_config(&touch_config);
                bk_touch_calibration_start();
            }
        }

        bk_touch_scan_mode_multi_channl_set(multi_chann_value);
        bk_touch_scan_mode_enable(1);
        bk_touch_int_enable(multi_chann_value, 1);

        ret = bk_timer_start(TIMER_ID1, 10000, touch_cyclic_calib_timer_isr);
        if (ret != BK_OK) {
            PR_NOTICE(NULL, "Timer start failed\r\n");
        }
    }
}

#if 0
void touch_push(float *data_buff, float data, UINT8 num)
{
    UINT8 i;

    for(i=0;i<num;i++)
    {
        data_buff[i] = data_buff[i+1];
    }
    data_buff[num] = data;
}

void touch_saradc_iir_iir_fillter(UINT8 chan_idx)
{
    UINT16 value = 0;
    UINT32 sum = 0;
    float touch_delt[11] = {-300,-300,-300,-300,-300,-300,-300,-300,-300,-300,-300};
    uint32_t chan[11] = {2,3,4,5,8,9,10,11,12,14,15};
    float x=0.0,y1=0.0,y2=0.0,b1=1.0,b2=0.0,b3=-1.0,a1=1.0,a2=-1.8945,a3=0.9037,a4=-1.5515,a5=0.6755,s1=0.1588,s2=0.1588;

    aon_pmu_drv_touch_select(chan[chan_idx]);
    for(UINT8 i = 0; i<= 1; i++)
    {
        BK_LOG_ON_ERR(bk_adc_read(&value, ADC_READ_SEMAPHORE_WAIT_TIME));
        value = value << 1;
        sum += value;
    }

    x = ((float)sum)/2;
    //IIR1
    touch_push(iir_x[chan_idx], x, 2);
    y1 = (((b1*iir_x[chan_idx][2] + b2*iir_x[chan_idx][1] + b3*iir_x[chan_idx][0]) - a2 * iir_y[chan_idx][1] - a3 * iir_y[chan_idx][0])/a1);

    touch_push(iir_y[chan_idx], y1, 1);

    //IIR2
    y1 = y1*s1;
    touch_push(iir_y_x[chan_idx], y1, 2);
    y2 = (((b1*iir_y_x[chan_idx][2] + b2*iir_y_x[chan_idx][1] + b3*iir_y_x[chan_idx][0]) - a4 * iir_y_y[chan_idx][1] - a5 * iir_y_y[chan_idx][0])/a1);

    touch_push(iir_y_y[chan_idx], y2, 1);
    y2 = y2*s2;
    //PR_DEBUG("y2=%f,touch_chan=%d,num=%d\r\n",y2,chan_idx,num);

    //initial oscilation
    if(g_num >= 100)
    {
        //The y2 is negative
        if ((y2 < touch_delt[chan_idx]))
        {
            s_touch_channel = chan[chan_idx];
        }
    }
    rtos_delay_milliseconds(2);
}

void touch_capa_cali(void *param)
{
    uint32_t touch_chan = 2;
    UINT32 cap_out      = 0;
    int ret             = 0;
    uint32_t chan[11]   = {2,3,4,5,8,9,10,11,12,14,15};
    UINT8 j             = 0;
    touch_config_t touch_config;

    if(g_touch_capa_cali_flag == 0)
    {
        g_touch_capa_cali_flag = 1;
        for(j = 0; j < 11; j++)
        {
            touch_chan = chan[j];
            bk_touch_gpio_init(1 << touch_chan);
            bk_touch_enable(1 << touch_chan);
            bk_touch_scan_mode_enable(0);

            touch_config.sensitivity_level = TOUCH_SENSITIVITY_LEVLE_0;
            touch_config.detect_threshold = TOUCH_DETECT_THRESHOLD_6;
            touch_config.detect_range = TOUCH_DETECT_RANGE_8PF;
            bk_touch_config(&touch_config);

            bk_touch_calibration_start();
            cap_out = bk_touch_get_calib_value();
            //PR_DEBUG("cap_out=%d,touch_chan=%d\r\n",cap_out,chan[j]);
            bk_touch_manul_mode_enable(cap_out);
            bk_touch_manul_mode_disable();
        }
        if (touch_capa_cali_tmr.handle != NULL)
        {
            ret = rtos_reload_timer(&touch_capa_cali_tmr);
            BK_ASSERT(kNoErr == ret);
        }
        g_touch_capa_cali_flag = 0;
    }
    else
    {
        PR_DEBUG("touch saradc task executing!\r\n");
    }
}

void touch_adc_get(void *param)
{
    UINT8 j;
    int err;

    g_num += 1;
    if(g_touch_capa_cali_flag == 0)
    {
        g_touch_capa_cali_flag = 2;
        BK_LOG_ON_ERR(bk_adc_acquire());
        sys_drv_set_ana_pwd_gadc_buf(0);
        BK_LOG_ON_ERR(bk_adc_init(ADC_9));
        adc_config_t config = {0};

        config.chan = ADC_9;
        config.adc_mode = 3;
        config.src_clk = 1;
        config.clk = 0x31975;
        config.saturate_mode = 4;
        config.steady_ctrl= 7;
        config.adc_filter = 0;
        if(config.adc_mode == ADC_CONTINUOUS_MODE)
        {
            config.sample_rate = 0;
        }

        BK_LOG_ON_ERR(bk_adc_set_config(&config));
        BK_LOG_ON_ERR(bk_adc_enable_bypass_clalibration());
        BK_LOG_ON_ERR(bk_adc_start());

        for(j = 0; j < 11; j++)
        {
            touch_saradc_iir_iir_fillter(j);
        }
        if(g_num >= 100)
            g_num -= 1;
        bk_adc_stop();
        bk_adc_deinit(ADC_9);
        bk_adc_release();
        if (touch_tmr.handle != NULL)
        {
            err = rtos_reload_timer(&touch_tmr);
            BK_ASSERT(kNoErr == err);
        }
        g_touch_capa_cali_flag = 0;
    }
}

void cli_touch_adc_mode_test_cmd()
{
    // if (os_strcmp(argv[1], "cali") == 0)
    if (1)
    {
        UINT32 t_ms = 5000;
        int err;

        if (touch_capa_cali_tmr.handle != NULL)
        {
            err = rtos_deinit_timer(&touch_capa_cali_tmr);
            BK_ASSERT(kNoErr == err);
            touch_capa_cali_tmr.handle = NULL;
        }

        err = rtos_init_timer(&touch_capa_cali_tmr,
                              t_ms,
                              touch_capa_cali,
                              (void *)0);
        BK_ASSERT(kNoErr == err);
        err = rtos_start_timer(&touch_capa_cali_tmr);
        BK_ASSERT(kNoErr == err);

        //enable LED
        // err = bk_touch_digital_tube_display_init();
        // if (err != BK_OK)
        // {
        //     PR_DEBUG("init touch digital tube display task failed!\r\n");
        //     return;
        // }
    }
    // else if(os_strcmp(argv[1], "test") == 0)
    if (1)
    {
        UINT32 t_ms = 100;
        int err;

        if (touch_tmr.handle != NULL)
        {
            err = rtos_deinit_timer(&touch_tmr);
            BK_ASSERT(kNoErr == err);
            touch_tmr.handle = NULL;
        }

        err = rtos_init_timer(&touch_tmr,
                              t_ms,
                              touch_adc_get,
                              (void *)0);
        BK_ASSERT(kNoErr == err);
        err = rtos_start_timer(&touch_tmr);
        BK_ASSERT(kNoErr == err);
    }
}
#endif

#endif
