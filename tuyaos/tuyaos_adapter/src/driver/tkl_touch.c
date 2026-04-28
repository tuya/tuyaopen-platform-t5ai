#include "tkl_touch.h"
#include <os/os.h>
#include <math.h>
#include "driver/touch.h"
#include "driver/touch_types.h"
#include "touch_driver.h"

/***********************************************************
*************************micro define***********************
***********************************************************/
#define FAST_RESPONSE_K 16 /* Fast response IIR parameter */
#define FAST_RESPONSE_N 12 /* Fast response weight */
#define SLOW_RESPONSE_K 64 /* Slow response IIR parameter */
#define SLOW_RESPONSE_N 4  /* Slow response weight */
#define BUF_SIZE        8  /* Reduce buffer size to improve response speed */

#define TOUCH_STATIC_NOISE_THRESHOLD    touch_static_noise_threshold        /* static noise threshold */
#define TOUCH_FILTER_UPDATE_THRESHOLD   touch_filter_update_threshold       /* static noise threshold */
#define TOUCH_DETECT_THRESHOLD          touch_detect_threshold              /* static noise threshold */
#define TOUCH_VARIANCE_THRESHOLD        touch_variance_threshold            /* variance threshold */

#define DEBUG_ENABLE            0
#define TOUCH_SAMPLE_TIME       20
#define TOUCH_LONG_PRESSED_TIME 2000 /* Long press threshold time ms */ 
#define TOUCH_SHORT_PRESSED_TIME 500
#define TOUCH_STATE_STABLE_TIME 1

#define COUNT_SET_BITS(n) (__builtin_popcount((unsigned int)(n)))
/***********************************************************
***********************variable define**********************
***********************************************************/
// Internal callback function type
typedef struct {
    TUYA_TOUCH_CALLBACK callback;
    void *arg;
} touch_callback_t;

// Touch channel state structure
typedef struct {
    BOOL_T enabled;
    uint32_t calibration_value;
    TUYA_TOUCH_DETECT_RANGE_E detect_range;
} touch_channel_state_t;

// Define independent state variables for each channel
typedef struct {
    uint8_t touch_state; // 0: Not touched, 1: Touched
    uint8_t touch_flag;
    float baseline;
    float raw_buf[BUF_SIZE];
    float filtered_buf[BUF_SIZE];
    float last_filtered_value;
    uint8_t buf_index;
} touch_channel_data_t;

// Maintain independent key states for each channel
typedef struct {
    uint8_t key_state;      // Current key state
    uint8_t last_key_state; // Last key state
    uint32_t press_count;   // Press counter
    uint32_t released_count;// Release counter
} key_state_t;

static touch_callback_t touch_callbacks[TUYA_TOUCH_CHANNEL_MAX] = {0};
static touch_channel_state_t touch_channel_states[TUYA_TOUCH_CHANNEL_MAX] = {0};
static BOOL_T touch_initialized = FALSE;
static uint32_t current_enabled_channels = 0;
static touch_config_t touch_config;
static beken_thread_t __tkl_touch_scan_thread_handle = NULL;
static key_state_t key_states[TUYA_TOUCH_CHANNEL_MAX] = {0};
static touch_channel_data_t channel_data[TUYA_TOUCH_CHANNEL_MAX] = {0};
static float average_value[TUYA_TOUCH_CHANNEL_MAX];
static float median_value[TUYA_TOUCH_CHANNEL_MAX];
static float touch_static_noise_threshold, touch_filter_update_threshold, touch_detect_threshold, touch_variance_threshold;
static float __max_raw_value = 0;
/***********************************************************
***********************function define**********************
***********************************************************/
float get_median_value(float find_array[], uint8_t array_len);
float get_avg_value(float find_array[], uint8_t array_len);
float get_variance_value(float array[], uint8_t array_len);
float get_discrete_state(float value, float avg_value);
float get_max_value(float find_array[], uint8_t array_len);
int32_t find_index(float array[], uint8_t array_len, float find_value);
float get_iir_filter_value(float current_value, float last_value, uint32_t k, uint32_t n);
float get_square_wave_filter(float raw_value, float baseline_value, uint8_t touch_id);
float get_adaptive_baseline(float current_value, float old_baseline, uint8_t is_stable);
uint8_t tkl_touch_get_single_channel_status(uint8_t touch_id);
OPERATE_RET tkl_touch_get_single_calibration_value(uint32_t touch_id, float *value);

static void __touch_parameter_init(uint32_t channel_mask)
{
    uint8_t i, channel;
    float raw_data[BUF_SIZE] = {0};

    // Multiple sampling to obtain stable baseline
    for (channel = 0; channel < TUYA_TOUCH_CHANNEL_MAX; channel++) {
        if (channel_mask & (1 << channel)){
            i = 0;
            while (i < BUF_SIZE) {
                tkl_touch_get_single_calibration_value(channel, &raw_data[i++]);
            }
            float init_baseline = get_avg_value(raw_data, BUF_SIZE);
            channel_data[channel].baseline = init_baseline;

            // Initialize buffers
            for (uint8_t j = 0; j < BUF_SIZE; j++) {
                channel_data[channel].raw_buf[j] = init_baseline;
                channel_data[channel].filtered_buf[j] = init_baseline;
            }
            channel_data[channel].last_filtered_value = init_baseline;
            channel_data[channel].touch_state = 0;
            channel_data[channel].buf_index = 0;

            bk_printf("baseline[%d]: %f\r\n", channel, channel_data[channel].baseline);
        }
    }
}

uint8_t __touch_status_process(uint32_t touch_id)
{
    uint8_t rt = 0;
    static float raw_value, filtered_value;
    static float variance_value_raw;
    static float median_buf[BUF_SIZE] = {0};

    // Ensure touch_id is within valid range
    if (touch_id >= TUYA_TOUCH_CHANNEL_MAX) {
        return 0;
    }

    // Read raw data
    tkl_touch_get_single_calibration_value(touch_id, &raw_value);

    // Circular buffer index update
    channel_data[touch_id].buf_index = (channel_data[touch_id].buf_index + 1) % BUF_SIZE;
    channel_data[touch_id].raw_buf[channel_data[touch_id].buf_index] = raw_value;
    if (__max_raw_value < raw_value) {
        __max_raw_value = raw_value;
    }
    // bk_printf("max_raw_value: %f\r\n", max_raw_value);

    // Calculate raw data variance to determine signal stability
    variance_value_raw = get_variance_value(channel_data[touch_id].raw_buf, BUF_SIZE);

    // Use square wave filtering algorithm
    filtered_value = get_square_wave_filter(raw_value, channel_data[touch_id].baseline, touch_id);

    // Median filtering
    median_value[touch_id] = get_median_value(channel_data[touch_id].filtered_buf, BUF_SIZE);
    median_buf[channel_data[touch_id].buf_index] = median_value[touch_id];
    // Average filtering
    average_value[touch_id] = get_avg_value(median_buf, BUF_SIZE);
    // Maximum filtering
    float max_value = get_max_value(channel_data[touch_id].filtered_buf, BUF_SIZE / 2);

    // Touch state detection - Use hysteresis comparator to avoid jitter
    float temp = (max_value - channel_data[touch_id].baseline) > (filtered_value - channel_data[touch_id].baseline) ? 
                 (max_value - channel_data[touch_id].baseline) : (filtered_value - channel_data[touch_id].baseline);
    float touch_diff = median_value[touch_id] - channel_data[touch_id].baseline;
    touch_diff = temp > touch_diff ? temp : touch_diff;
    if (channel_data[touch_id].touch_state == 0) {
        // Currently not touched, detect touch
        if (touch_diff > TOUCH_DETECT_THRESHOLD) {
            channel_data[touch_id].touch_state = 1;
            channel_data[touch_id].touch_flag = 1;
            rt = 1;
            #if DEBUG_ENABLE
            bk_printf("Touch detected on channel %d!\r\n", touch_id);
            #endif
            get_adaptive_baseline(median_value[touch_id], channel_data[touch_id].baseline, 0);
        } else if (touch_diff < 0) {
            // get_adaptive_baseline(filtered_value, channel_data[touch_id].baseline, 2);
            channel_data[touch_id].baseline = filtered_value;
        } else {
            // Slowly update baseline when not touched
            if (variance_value_raw < TOUCH_VARIANCE_THRESHOLD) {
                channel_data[touch_id].baseline =
                get_adaptive_baseline(median_value[touch_id], channel_data[touch_id].baseline, 1);
            }
        }
    } else {
        // Currently touched, detect release
        if (touch_diff < TOUCH_DETECT_THRESHOLD) {
            channel_data[touch_id].touch_state = 0;
            channel_data[touch_id].touch_flag = 0;
            #if DEBUG_ENABLE
            bk_printf("Touch released on channel %d!\r\n", touch_id);
            #endif
            get_adaptive_baseline(median_value[touch_id], channel_data[touch_id].baseline, 2);
        } else {
            get_adaptive_baseline(median_value[touch_id], channel_data[touch_id].baseline, 0);
            rt = 1; // Continuous touch
        }
    }

#if DEBUG_ENABLE 
    printf("median_value=%lf, average_value=%lf, max_value=%f\r\n", median_value[touch_id], average_value[touch_id], max_value);
    printf("baseline=%f, raw_value=%f, iir_filter_buf=%f\r\n", channel_data[touch_id].baseline, raw_value, filtered_value);
    printf("channel=%d, key=%d, variance_value_raw=%lf\r\n", touch_id, key_states[touch_id].key_state, variance_value_raw);
    printf("touch_diff=%f, static_noise_threshold=%f, filter_update_threshold=%f, detect_threshold=%f\r\n", 
            touch_diff, TOUCH_STATIC_NOISE_THRESHOLD + channel_data[touch_id].baseline, TOUCH_FILTER_UPDATE_THRESHOLD + channel_data[touch_id].baseline, TOUCH_DETECT_THRESHOLD);
#endif
    return rt;
}

float tkl_touch_get_filter_update_threshold()
{
    return __max_raw_value;
}

// Internal interrupt handler function
static void __tkl_touch_isr(void *param)
{
    bk_touch_clear_int(0xFFFF);

    uint32_t channel = (uint32_t)param;
    uint32_t int_status = 0;

    // Get interrupt status
    int_status = bk_touch_get_int_status();
    bk_printf("Touch interrupt: channel=%d, status=0x%X\r\n", channel, int_status);

    for (uint8_t ch = 0; ch < TUYA_TOUCH_CHANNEL_MAX; ch++) {
        if (touch_callbacks[ch].callback != NULL) {
            if (int_status & (1 << ch)) {
                touch_callbacks[ch].callback(ch, TUYA_TOUCH_EVENT_PRESSED, touch_callbacks[ch].arg);
            } else {
                touch_callbacks[ch].callback(ch, TUYA_TOUCH_EVENT_RELEASED, touch_callbacks[ch].arg);
            }
        }
    }
}

static void __tkl_touch_scan_thread(void)
{
    // Calculate long press time
    if (current_enabled_channels == 0)
        return;
    uint32_t long_press_threshold = TOUCH_LONG_PRESSED_TIME / TOUCH_SAMPLE_TIME / COUNT_SET_BITS(current_enabled_channels);
    uint32_t event = 0;

    for (uint8_t i = 0; i < TUYA_TOUCH_CHANNEL_MAX; i++) {
        key_states[i].key_state = 0;
        key_states[i].last_key_state = 0;
        key_states[i].press_count = 0;
        key_states[i].released_count = TOUCH_STATE_STABLE_TIME + 1;
    }
    while (1) {
        // Traverse all enabled channels
        for (uint8_t touch_id = 0; touch_id < TUYA_TOUCH_CHANNEL_MAX; touch_id++) {
            // Check if channel is enabled
            if (!(current_enabled_channels & (1 << touch_id))) {
                continue;
            }

            // Update key state
            key_states[touch_id].key_state = __touch_status_process(touch_id);
            // State machine processing
            if (key_states[touch_id].last_key_state == 0 && key_states[touch_id].key_state == 1) {
                // Key press starts counting
                key_states[touch_id].press_count = 0;
            } else if (key_states[touch_id].last_key_state == 1 && key_states[touch_id].key_state == 0) {
                // Key release, determine short press or long press
                key_states[touch_id].released_count = 0;
                // Reset count
                key_states[touch_id].press_count = 0;
            } else if (key_states[touch_id].last_key_state == 1 && key_states[touch_id].key_state == 1) {
                // Continuous press state, check if long press threshold is reached
                key_states[touch_id].press_count++;
                if (key_states[touch_id].press_count == TOUCH_STATE_STABLE_TIME) {
                    if (touch_callbacks[touch_id].callback != NULL) {
                        touch_callbacks[touch_id].callback(touch_id, TUYA_TOUCH_EVENT_PRESSED,
                                                           touch_callbacks[touch_id].arg);
                                                           event = 1;
                    }
                }                
                if (key_states[touch_id].press_count == long_press_threshold) {
                    // Long press event
                    if (touch_callbacks[touch_id].callback != NULL) {
                        touch_callbacks[touch_id].callback(touch_id, TUYA_TOUCH_EVENT_LONG_PRESS,
                                                           touch_callbacks[touch_id].arg);
                                                           event = 2;
                    }
                } else if (key_states[touch_id].press_count > 5 * long_press_threshold) {
                    tkl_touch_get_single_calibration_value(touch_id, &(channel_data[touch_id].baseline));   
                }
            } else if (key_states[touch_id].last_key_state == 0 && key_states[touch_id].key_state == 0) {
                if (key_states[touch_id].released_count < TOUCH_STATE_STABLE_TIME) {
                    key_states[touch_id].released_count++;
                } else if (key_states[touch_id].released_count == TOUCH_STATE_STABLE_TIME) {
                    if (touch_callbacks[touch_id].callback != NULL) {
                        touch_callbacks[touch_id].callback(touch_id, TUYA_TOUCH_EVENT_RELEASED,
                                                           touch_callbacks[touch_id].arg);
                                                           event = 0;
                    }
                    key_states[touch_id].released_count++;
                } else {
                    key_states[touch_id].released_count = TOUCH_STATE_STABLE_TIME + 1;
                }
            }
            // Save current state as last state for next iteration
            key_states[touch_id].last_key_state = key_states[touch_id].key_state;
#if DEBUG_ENABLE
            printf("event=%d,key_status=%d\r\n", event, tkl_touch_get_single_channel_status(touch_id));
#endif
        }
        rtos_thread_msleep(TOUCH_SAMPLE_TIME); // Increase sampling frequency to 50Hz
    }
}

OPERATE_RET tkl_touch_init(uint32_t channel_mask, TUYA_TOUCH_CONFIG_T *cfg)
{
    uint32_t channel;

    if (NULL == cfg) {
        return OPRT_INVALID_PARM;
    }

    if (touch_initialized) {
        bk_printf("Error: Touch already initialized");
        return OPRT_OK;
    }

    // Initialize GPIO
    bk_touch_gpio_init(channel_mask);

    // Initialize filter parameters
    touch_static_noise_threshold    = cfg->threshold.touch_static_noise_threshold;
    touch_filter_update_threshold   = cfg->threshold.touch_filter_update_threshold;
    touch_detect_threshold          = cfg->threshold.touch_detect_threshold;
    touch_variance_threshold        = cfg->threshold.touch_variance_threshold;
    __touch_parameter_init(channel_mask);

    // Initialize callback function array and channel states
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

    // Convert configuration parameters
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

    // Configure touch parameters
    bk_touch_config(&touch_config);
#if (CONFIG_SOC_BK7236XX || CONFIG_SOC_BK7239XX || CONFIG_SOC_BK7286XX)
    bk_touch_set_test_mode(0, 0);
    bk_touch_set_calib_mode(0x3F, 9);
#endif

    bk_printf("Touch init success, channel_mask=0x%X\r\n", channel_mask);
    return OPRT_OK;
}

OPERATE_RET tkl_touch_deinit(uint32_t channel_mask)
{
    uint32_t channel;

    if (!touch_initialized) {
        return OPRT_OK;
    }

    // Disable interrupt and scan mode
    bk_touch_int_enable(channel_mask, 0);
    bk_touch_scan_mode_enable(0);

    // Clear callback functions and channel states
    for (channel = 0; channel < TUYA_TOUCH_CHANNEL_MAX; channel++) {
        if (channel_mask & (1 << channel)) {
            touch_callbacks[channel].callback = NULL;
            touch_callbacks[channel].arg = NULL;
            touch_channel_states[channel].enabled = FALSE;
            touch_channel_states[channel].calibration_value = 0;
        }
    }

    // Disable touch channels
    bk_touch_disable();

    current_enabled_channels &= ~channel_mask;
    if (current_enabled_channels == 0) {
        touch_initialized = FALSE;
    }

    bk_printf("Touch deinit success, channel_mask=0x%x\r\n", channel_mask);
    return OPRT_OK;
}

OPERATE_RET tkl_touch_enable(uint32_t channel_mask)
{
    bk_touch_enable(channel_mask);
    return OPRT_OK;
}

OPERATE_RET tkl_touch_disable(uint32_t channel_mask)
{
    bk_touch_disable();
    return OPRT_OK;
}

OPERATE_RET tkl_touch_register_callback(uint32_t channel_mask, TUYA_TOUCH_CALLBACK callback, void *arg)
{
    uint32_t channel;

    // Register scan thread (create only once)
    if (__tkl_touch_scan_thread_handle == NULL) {
        rtos_create_thread(&__tkl_touch_scan_thread_handle, BEKEN_DEFAULT_WORKER_PRIORITY, "touch_scan",
                           (beken_thread_function_t)__tkl_touch_scan_thread, 4096, NULL);
    }

    // Register touch interrupt
    for (channel = 0; channel < TUYA_TOUCH_CHANNEL_MAX; channel++) {
        if (channel_mask & (1 << channel)) {
            touch_callbacks[channel].callback = callback;
            touch_callbacks[channel].arg = arg;

            // Register interrupt handler function
            // bk_touch_register_touch_isr(1 << channel, __tkl_touch_isr, (void *)channel);
            // bk_touch_int_enable(1 << channel, 1);
        }
    }
    return OPRT_OK;
}

OPERATE_RET tkl_touch_get_single_calibration_value(uint32_t touch_id, float *value)
{
    uint32_t channel = 0;
    uint32_t cap_out2 = 0, cap_out0 = 0, cap_out1 = 0;
    for (channel = 0; channel < TUYA_TOUCH_CHANNEL_MAX; channel++) {
        if (channel == 6 || channel == 7 || channel == 13 || channel == 12) {
            continue;
        }
        if (touch_id == channel) {

            bk_touch_gpio_init(1 << channel);
            bk_touch_enable(1 << channel);
            // bk_touch_register_touch_isr(1 << channel, __tkl_touch_isr, NULL);

            // touch_config.detect_range = TOUCH_DETECT_RANGE_8PF;
            bk_touch_config(&touch_config);
            bk_touch_scan_mode_enable(0);
            bk_touch_calibration_start();
            cap_out0 = bk_touch_get_calib_value();
            *value = cap_out0;
            #if DEBUG_ENABLE
            printf("capout0=%f\r\n", *value);
            #endif
            if (*value >= 0x1F0) {
                touch_config.detect_range = TOUCH_DETECT_RANGE_12PF;
                bk_touch_config(&touch_config);
                bk_touch_calibration_start();
                cap_out1 = bk_touch_get_calib_value();
                *value = cap_out1;
                #if DEBUG_ENABLE
                printf("capout1=%f\r\n", *value);
                #endif
                if (*value >= 0x1F0) {
                    touch_config.detect_range = TOUCH_DETECT_RANGE_19PF;
                    bk_touch_config(&touch_config);
                    bk_touch_calibration_start();
                    cap_out2 = bk_touch_get_calib_value();
                    *value = cap_out2;
                    if (*value >= 0x1F0) {
                        touch_config.detect_range = TOUCH_DETECT_RANGE_27PF;
                        bk_touch_config(&touch_config);
                        bk_touch_calibration_start();
                        *value = bk_touch_get_calib_value();
                        if (*value >= 0x1F0) {
                            bk_printf("Error: Calibration value is out of the detect range, the channel [%d] cannot be used, "
                                "please select the other channel!",
                                channel);
                        }
                    }
                }
            }
            #if DEBUG_ENABLE
            printf("value=%f\r\n", *value);
            #endif
            if (touch_config.detect_range == TOUCH_DETECT_RANGE_8PF) {
                *value = (float)((*value) / 512.0f * 8);
            } else if (touch_config.detect_range == TOUCH_DETECT_RANGE_12PF) {
                *value = (float)((*value) / 512.0f * 12);
            } else if (touch_config.detect_range == TOUCH_DETECT_RANGE_19PF) {
                *value = (float)((*value) / 512.0f * 19);
            } else if (touch_config.detect_range == TOUCH_DETECT_RANGE_27PF) {
                *value = (float)((*value) / 512.0f * 27);
            }
            touch_channel_states[channel].calibration_value = *value;
            touch_channel_states[channel].detect_range = touch_config.detect_range;
            // bk_touch_int_enable(1 << channel, 1);
        }
    }
    return OPRT_OK;
}

OPERATE_RET tkl_touch_get_single_median_filter_value(uint8_t touch_id, float *value)
{
    if (touch_id >= TUYA_TOUCH_CHANNEL_MAX || value == NULL) {
        return OPRT_INVALID_PARM;
    }

    if (NULL != __tkl_touch_scan_thread_handle) {
        *value = median_value[touch_id];
    }
    else {
        __touch_status_process(touch_id);
        *value = median_value[touch_id];
    }
    return OPRT_OK;
}

OPERATE_RET tkl_touch_get_single_average_filter_value(uint8_t touch_id, float *value)
{
    if (touch_id >= TUYA_TOUCH_CHANNEL_MAX || value == NULL) {
        return OPRT_INVALID_PARM;
    }

    if (NULL != __tkl_touch_scan_thread_handle) {
        *value = average_value[touch_id];
    }
    else {
        __touch_status_process(touch_id);
        *value = average_value[touch_id];
    }
    return OPRT_OK;
}

uint8_t tkl_touch_get_single_channel_status(uint8_t touch_id)
{
    if (touch_id >= TUYA_TOUCH_CHANNEL_MAX) {
        bk_printf("Error: out of range\n");
        return 0;
    }
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

OPERATE_RET tkl_touch_scan_mode_multi_channel_set(uint32_t channel_mask)
{
    bk_touch_scan_mode_multi_channl_set(channel_mask);
    return OPRT_OK;
}

OPERATE_RET tkl_touch_clear_interrupt(uint32_t channel_mask)
{
    bk_touch_clear_int(channel_mask);
    return OPRT_OK;
}

OPERATE_RET tkl_touch_interrupt_enable(uint32_t channel_mask, BOOL_T enable)
{
    if (enable) {
        bk_touch_int_enable(channel_mask, 1);
    } else {
        bk_touch_int_enable(channel_mask, 0);
    }

    return OPRT_OK;
}

OPERATE_RET tkl_touch_get_channel_detect_range(uint32_t channel, TUYA_TOUCH_DETECT_RANGE_E *detect_range)
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
float get_max_value(float find_array[], uint8_t array_len)
{
    float max_value;
    uint8_t find_index = 0;

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
float get_min_value(float find_array[], uint8_t array_len)
{
    float min_value;
    uint8_t find_index = 0;

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
float get_median_value(float find_array[], uint8_t array_len)
{
    if (NULL == find_array || array_len == 0) return 0;

    float temp[BUF_SIZE];
    uint8_t i, j;
    // copy
    for (i = 0; i < array_len; i++) temp[i] = find_array[i];

    // simple sort on temp
    for (i = 0; i < array_len - 1; i++) {
        for (j = 0; j < array_len - i - 1; j++) {
            if (temp[j] > temp[j + 1]) {
                float t = temp[j];
                temp[j] = temp[j + 1];
                temp[j + 1] = t;
            }
        }
    }

    return temp[(array_len - 1) / 2];
}

/**
 * @brief get average
 *
 * @param[in] array: find the average of the array
 * @param[in] array_len: array length
 * @return average value
 */
float get_avg_value(float array[], uint8_t array_len)
{
    float avg_value;
    float sum = 0;
    uint8_t i;

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
float get_variance_value(float array[], uint8_t array_len)
{
    float variance_value = 0;
    float square_sum = 0;

    if (NULL == array || array_len <= 0) {
        return 0;
    }

    float avg_value = get_avg_value(array, array_len);

    uint8_t i;
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
float get_discrete_state(float value, float avg_value)
{
    float discrete_value = 0;

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
float get_iir_filter_value(float current_value, float last_value, uint32_t k, uint32_t n)
{
    float result = 0;
    float temp_value = 0.0;
    temp_value = (1.0 / k) * (n * (float)current_value + (k - n) * (float)last_value);
    result = (float)temp_value;
    return result;
}

/**
 * @brief Square wave filtering algorithm - Fast response to touch changes
 *
 * @param[in] raw_value: Raw data
 * @param[in] baseline_value: Baseline value
 * @param[in] touch_id: Touch channel ID
 * @return Filtered value
 */
float get_square_wave_filter(float raw_value, float baseline_value, uint8_t touch_id)
{
    float change_rate = fabs(raw_value - channel_data[touch_id].last_filtered_value);
    float filtered_result;
    
    // Select filter parameters based on change rate and current state
    if (change_rate > TOUCH_STATIC_NOISE_THRESHOLD) {
        // Detect rapid changes, use fast response filtering
        if (channel_data[touch_id].touch_state == 0 && raw_value > baseline_value + TOUCH_FILTER_UPDATE_THRESHOLD) {
            // Touch rising edge - extremely fast response
            filtered_result = get_iir_filter_value(raw_value, channel_data[touch_id].last_filtered_value, 6, 5);
        } else if (channel_data[touch_id].touch_state == 1 && raw_value < baseline_value + TOUCH_FILTER_UPDATE_THRESHOLD) {
            // Touch falling edge - extremely fast response
            filtered_result = get_iir_filter_value(raw_value, channel_data[touch_id].last_filtered_value, 6, 5);
        } else {
            // Other rapid changes
            filtered_result = get_iir_filter_value(raw_value, channel_data[touch_id].last_filtered_value,
                                                   FAST_RESPONSE_K, FAST_RESPONSE_N);
        }
    } else {
        // Slow changes, use slow filtering to maintain stability
        filtered_result = get_iir_filter_value(raw_value, channel_data[touch_id].last_filtered_value, SLOW_RESPONSE_K,
                                               SLOW_RESPONSE_N);
    }
    channel_data[touch_id].filtered_buf[channel_data[touch_id].buf_index] = filtered_result;
    channel_data[touch_id].last_filtered_value = filtered_result;
    return filtered_result;
}

/**
 * @brief Adaptive baseline update
 *
 * @param[in] current_value: Current filtered value
 * @param[in] old_baseline: Old baseline value
 * @param[in] is_stable: Whether it is a stable state
 * @return New baseline value
 */
float get_adaptive_baseline(float current_value, float old_baseline, uint8_t is_stable)
{
#if DEBUG_ENABLE
    printf("is_stable=%d\r\n", is_stable);
#endif
    if (is_stable == 2) {
        // Fastly update baseline in stable state
        return get_iir_filter_value(current_value, old_baseline, FAST_RESPONSE_K, FAST_RESPONSE_N);
    } else if (is_stable == 1) {
        // Slowly update baseline in stable state
        return get_iir_filter_value(current_value, old_baseline, 256, 1);
    } else {
        // Do not update baseline in unstable state
        // return get_iir_filter_value(current_value, old_baseline, 512, 1);
        return old_baseline;
    }
}

int32_t find_index(float array[], uint8_t array_len, float find_value)
{
    uint8_t i;

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
