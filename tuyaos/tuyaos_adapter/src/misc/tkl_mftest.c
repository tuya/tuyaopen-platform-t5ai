#include "tkl_mftest.h"
#include "tkl_memory.h"
#include "tkl_thread.h"
#include "tkl_semaphore.h"

extern void cli_audio_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);

volatile uint32_t __aec_test_flag = 0;
static int __tkl_mf_aec_mic_test(void *args);

/**
 * @brief 模组固件升级更新结束，0x11
 * @note 设备应用层只做状态的处理，不对数据做处理
 */
static OPERATE_RET __pt_start_audio_test(uint16_t cmd, uint8_t *data, uint32_t len, uint8_t **ret_data, uint16_t *ret_len)
{
    uint8_t flag = 0;
    bk_printf("__pt_start_audio_test\r\n");
    bk_printf("=============================\r\n");
    bk_printf("mic channel %x\r\n", data[0]);
    bk_printf("=============================\r\n");

    // 采样率由 AEC_SAMPLE_RATE 决定，后续只需修改该宏即可
    char sample_rate_str[12];
    snprintf(sample_rate_str, sizeof(sample_rate_str), "%d", AEC_SAMPLE_RATE);

    // char *stop[5] = {"audio_test", "0", "44100", "1", "xxx"};
    // cli_audio_test_cmd(NULL, 0, 4, &stop);

    if (data[0] == 1) {
        TUYA_GPIO_BASE_CFG_T cfg;
        cfg.mode = TUYA_GPIO_FLOATING;
        cfg.direct = TUYA_GPIO_INPUT;
        tkl_gpio_init(TUYA_GPIO_NUM_28, &cfg);


        // char *argv[5] = {"audio_test", "1", "44100", "1", "xxx"};
        char *argv[5] = {"audio_test", "1", sample_rate_str, "1", "xxx"};
        cli_audio_test_cmd(NULL, 0, 4, &argv);

    } else if (data[0] == 2) {

        // char *argv[5] = {"audio_test", "2", "44100", "1", "xxx"};
        char *argv[5] = {"audio_test", "2", sample_rate_str, "1", "xxx"};
        cli_audio_test_cmd(NULL, 0, 4, &argv);
    }

    bk_printf("=============================\r\n");
    TUYA_GPIO_LEVEL_E level;
    tkl_gpio_read(TUYA_GPIO_NUM_28, &level);
    bk_printf("gpio: %x\r\n", level);
    bk_printf("=============================\r\n");

    CHAR_T *out = tkl_system_malloc(64);
    if (NULL == out) {
        bk_printf("malloc failed...\r\n");
        return OPRT_MALLOC_FAILED;
    }
    memset(out, 0, 64);

    if ((data[0] == 1 && level == 0) ||
            (data[0] == 2 && level == 1))
        strcpy(out, "{\"ret\":true}");
    else
        strcpy(out, "{\"ret\":false}");

    *ret_data = (uint8_t *)out;
    *ret_len = strlen(out);

    bk_printf("start audio success\r\n");
    return OPRT_OK;
}

/**
 * @brief 模组固件升级更新结束，0x11
 * @note 设备应用层只做状态的处理，不对数据做处理
 */
static OPERATE_RET __pt_stop_audio_test(uint16_t cmd, uint8_t *data, uint32_t len, uint8_t **ret_data, uint16_t *ret_len)
{
    bk_printf("__pt_stop_audio_test\r\n");
    // 采样率由 AEC_SAMPLE_RATE 决定，后续只需修改该宏即可
    char sample_rate_str[12];
    snprintf(sample_rate_str, sizeof(sample_rate_str), "%d", AEC_SAMPLE_RATE);
    // char *argv[5] = {"audio_test", "0", "44100", "1", "xxx"};
    char *argv[5] = {"audio_test", "0", sample_rate_str, "1", "xxx"};
    cli_audio_test_cmd(NULL, 0, 4, &argv);
    // __test_audio_deinit();

    __aec_test_flag = 0;

    CHAR_T *out = tkl_system_malloc(64);
    if (NULL == out) {
        bk_printf("malloc failed...\r\n");
        return OPRT_MALLOC_FAILED;
    }
    memset(out, 0, 64);

    strcpy(out, "{\"ret\":true}");
    *ret_data = (uint8_t *)out;
    *ret_len = strlen(out);

    bk_printf("stop audio success\r\n");
    return OPRT_OK;
}

static uint32_t mf_test_idVendor = 0xFFFFFFFF;
static uint32_t mf_test_idProduct = 0xFFFFFFFF;
void tkl_mftest_usb_info(uint32_t vid, uint32_t pid)
{
    mf_test_idVendor = vid;
    mf_test_idProduct = pid;
}

bool tkl_mftest_is_usb_ready(void)
{
    int cnt = 0;
    do {
        if ((mf_test_idVendor != 0xFFFFFFFF) &&
           ((mf_test_idProduct != 0xFFFFFFFF))) {
            // got, next
            return true;
        }
        tkl_system_sleep(50);
    } while (cnt++ < 20);

    return false;
}

static OPERATE_RET __pt_mcu_get_usb_info(uint16_t cmd, uint8_t *data, uint32_t len, uint8_t **ret_data, uint16_t *ret_len)
{
    CHAR_T *ret_buf = tkl_system_malloc(128);
    if (ret_buf == NULL) {
        bk_printf("get usb info, malloc failed\r\n");
        return OPRT_MALLOC_FAILED;
    }
    memset(ret_buf, 0, 128);

    mf_test_idVendor = 0xFFFFFFFF;
    mf_test_idProduct = 0xFFFFFFFF;

    // power on, T5 测架P28使能usb电源
    TUYA_GPIO_BASE_CFG_T cfg;
    cfg.mode = TUYA_GPIO_PULLUP;
    cfg.direct = TUYA_GPIO_OUTPUT;
    cfg.level = TUYA_GPIO_LEVEL_HIGH;
    tkl_gpio_init(TUYA_GPIO_NUM_28, &cfg);
    tkl_gpio_write(TUYA_GPIO_NUM_28, TUYA_GPIO_LEVEL_HIGH);

    // open usb
    bk_usb_open(0);

    // get value, wait max 500ms
    tkl_mftest_is_usb_ready();

    bk_printf("usb device: 0x%04x 0x%04x\r\n", mf_test_idVendor, mf_test_idProduct);

    if ((mf_test_idVendor != 0xFFFFFFFF) && (mf_test_idProduct != 0xFFFFFFFF)) {
        bk_printf("get usb info success\r\n");
        char *usb_succ_str = "{\"ret\":true}";
        memcpy(ret_buf, usb_succ_str, strlen(usb_succ_str));
    } else {
        bk_printf("get usb info failed\r\n");
        char *failed_str = "{\"ret\":false}";
        memcpy(ret_buf, failed_str, strlen(failed_str));
    }

    *ret_data = (uint8_t *)ret_buf;
    *ret_len = strlen(ret_buf);

    bk_usb_close();
    tkl_gpio_write(TUYA_GPIO_NUM_28, TUYA_GPIO_LEVEL_LOW);
    tkl_system_sleep(100);
    return OPRT_OK;
}

struct aec_test {
    uint8_t in_mic;
    uint8_t aec_mic;
};

int16_t *aec_temp = NULL;
TKL_SEM_HANDLE __mf_aec_sem = NULL;
TKL_THREAD_HANDLE __mf_aec_calc_thread = NULL;
TKL_THREAD_HANDLE __mf_aec_data_thread = NULL;
static volatile int aec_freq = 0;

/* AEC频率采样验证 */
#define AEC_FREQ_SAMPLE_COUNT  10      /* 采样次数 */
#define AEC_FREQ_TARGET         2000    /* 目标频率 */
#define AEC_FREQ_TOLERANCE      10      /* 容差百分比 */
static int aec_freq_samples[AEC_FREQ_SAMPLE_COUNT] = {0};
static uint8_t aec_freq_sample_index = 0;
static volatile uint8_t aec_sampling_active = 0;

extern int func_calc_freq(uint16_t *buffer, uint32_t len, uint32_t sample_rate);

void tkl_mf_aec_notify(void)
{
    if (__mf_aec_sem)
        tkl_semaphore_post(__mf_aec_sem);
}

static void __tkl_mf_aec_calc_thread(void *args)
{
    bk_printf("start aec thread\r\n");

    TUYA_GPIO_BASE_CFG_T cfg;
    cfg.direct = TUYA_GPIO_OUTPUT;
    cfg.level = 1;
    tkl_gpio_init(TUYA_GPIO_NUM_33, &cfg);

    // 一帧 1280 字节 = 640 个 int16 (L/R 交织)，抽单声道后为 ADC_FRAME_SIZE/4 = 320 点
    uint32_t count = ADC_FRAME_SIZE / 4;
    // aec_temp 按 320 个 uint16 使用(func_calc_freq 读 320 点、CLI 写 320 点)，
    // 分配 320*2=640 字节，否则越界覆盖相邻堆对象 -> BusFault
    aec_temp = (int16_t *)os_malloc(count * sizeof(int16_t));
    memset(aec_temp, 0, count * sizeof(uint16_t));
    int last_aec_freq = 0;

    // create semphore
    tkl_semaphore_create_init(&__mf_aec_sem, 0, 1);

    __aec_test_flag = AEC_TEST_START;
    uint8_t cnt = 0;

    uint32_t t1 = 0, t2 = 0;
    while(1) {
        // tkl_semaphore_wait(__mf_aec_sem, 50);
        tkl_semaphore_wait(__mf_aec_sem, TKL_SEM_WAIT_FOREVER);

        __aec_test_flag = AEC_TEST_RUNNING;
        t1 = tkl_system_get_millisecond();
        /* Dump peak to judge whether MIC R captured real tone (noise stays near 0) */
        {
            int16_t peak = 0;
            extern volatile int16_t g_aec_peak_l;
            extern volatile int16_t g_aec_peak_r;
            extern volatile uint32_t g_aec_dac_wr_ok;
            extern volatile uint32_t g_aec_dac_wr_fail;
            for (uint32_t i = 0; i < count; i++) {
                int16_t v = aec_temp[i];
                if (v < 0) {
                    v = (int16_t)(-v);
                }
                if (v > peak) {
                    peak = v;
                }
            }
            aec_freq = func_calc_freq((uint16_t *)aec_temp, count, AEC_SAMPLE_RATE);
            t2 = tkl_system_get_millisecond();

            /* 存储频率采样值用于验证 */
            if (aec_sampling_active && aec_freq_sample_index < AEC_FREQ_SAMPLE_COUNT) {
                aec_freq_samples[aec_freq_sample_index++] = aec_freq;
            }

            if (last_aec_freq != aec_freq || (cnt++ % 10) == 0) {
                bk_printf("freq: %d, peak: %d, L:%d R:%d, wr:%u/%u, t: %d\r\n",
                          aec_freq, peak, g_aec_peak_l, g_aec_peak_r,
                          g_aec_dac_wr_ok, g_aec_dac_wr_fail, t2 - t1);
                last_aec_freq = aec_freq;
            }
        }

        __aec_test_flag = AEC_TEST_START;
    }

    tkl_semaphore_release(__mf_aec_sem);
    __mf_aec_sem = NULL;
    os_free(aec_temp);
    aec_temp = NULL;
}

extern void tkl_aec_fill_data(int16_t *buffer, uint32_t len);
extern void tkl_aec_set_tone_buf(int16_t *buf, uint32_t bytes);
extern void tkl_aec_enable_dual_mic(void);
static void __tkl_mf_aec_data_thread(void *args)
{
    int16_t *aec_data_buffer = NULL;
    uint32_t freq = 2000;
    double pi = 3.14159265358979f;

#define AEC_SAMPLE_NUM  320
    aec_data_buffer = (int16_t *)os_malloc(AEC_SAMPLE_NUM * sizeof(int16_t));
    if (aec_data_buffer == NULL) {
        bk_printf("aec buffer init failed\r\n");
        return;
    }

    /* DAC/ADC use signed PCM centered at 0 */
    bk_printf("freq: %d, sample: 16k\r\n", freq);
    for (int i = 0; i < AEC_SAMPLE_NUM; i++) {
        aec_data_buffer[i] = (int16_t)(sin(2.0 * pi * (double)freq * (double)i / 16000.0) * 32767.0);
    }

    /* Register tone for ISR; avoid thread sleep(20) racing with mic-loopback */
    tkl_aec_set_tone_buf(aec_data_buffer, AEC_SAMPLE_NUM * sizeof(int16_t));
    tkl_aec_fill_data(aec_data_buffer, AEC_SAMPLE_NUM * sizeof(int16_t));
    tkl_aec_fill_data(aec_data_buffer, AEC_SAMPLE_NUM * sizeof(int16_t));
    bk_printf("aec tone registered, isr will feed dac\r\n");

    while (1) {
        tkl_system_sleep(1000);
    }
}

static int __tkl_mf_aec_mic_test(void *args)
{
    if (__aec_test_flag != 0) {
        bk_printf("aec test already start\r\n");
        return 0;
    }
    __aec_test_flag = AEC_TEST_INIT;

    // start mic1 to dac
    bk_printf("start aec hw\r\n");

    // 采样率由 AEC_SAMPLE_RATE 决定，后续只需修改该宏即可
    char sample_rate_str[12];
    snprintf(sample_rate_str, sizeof(sample_rate_str), "%d", AEC_SAMPLE_RATE);
    // char *argv[5] = {"audio_test", "1", "44100", "1", "xxx"};
    char *argv[5] = {"audio_test", "1", sample_rate_str, "1", "xxx"};
    cli_audio_test_cmd(NULL, 0, 4, &argv);

    /* MIC R must be on for SPK loopback; override legacy mute-mic2 behavior */
    // tkl_aec_enable_dual_mic();

    // create thread
    tkl_thread_create(&__mf_aec_data_thread, "aec_data", 8192, 7, __tkl_mf_aec_data_thread, NULL);
    tkl_thread_create(&__mf_aec_calc_thread, "aec_calc", 8192, 6, __tkl_mf_aec_calc_thread, NULL);
    return 0;
}

static OPERATE_RET __pt_mcu_aec_start(uint16_t cmd, uint8_t *data, uint32_t len, uint8_t **ret_data, uint16_t *ret_len)
{
    // if (data == NULL) {
    //     bk_printf("invalid data\r\n");
    //     return OPRT_INVALID_PARM;
    // }

    CHAR_T *ret_buf = tkl_system_malloc(128);
    if (ret_buf == NULL) {
        bk_printf("get usb info, malloc failed\r\n");
        return OPRT_MALLOC_FAILED;
    }
    memset(ret_buf, 0, 128);

    struct aec_test *aec_config = (struct aec_test *)data;

    // ACTION Begin
    __tkl_mf_aec_mic_test(NULL);
    // ACTION End

    if (1) {
        bk_printf("aec start success\r\n");
        char *aec_succ_str = "{\"ret\":true,\"data\":\"xxx\"}";
        memcpy(ret_buf, aec_succ_str, strlen(aec_succ_str));
    } else {
        bk_printf("aec start failed\r\n");
        char *aec_failed_str = "{\"ret\":false,\"errCode\":500000,\"errMsg\":\"start error\"}";
        memcpy(ret_buf, aec_failed_str, strlen(aec_failed_str));
    }

    *ret_data = (uint8_t *)ret_buf;
    *ret_len = strlen(ret_buf);

    return OPRT_OK;
}

static OPERATE_RET __pt_mcu_aec_result(uint16_t cmd, uint8_t *data, uint32_t len, uint8_t **ret_data, uint16_t *ret_len)
{
    CHAR_T *ret_buf = tkl_system_malloc(256);
    if (ret_buf == NULL) {
        bk_printf("get usb info, malloc failed\r\n");
        return OPRT_MALLOC_FAILED;
    }
    memset(ret_buf, 0, 256);

    /* 开始采样验证 */
    aec_freq_sample_index = 0;
    aec_sampling_active = 1;

    /* 等待采样完成 (约1-2秒) */
    bk_printf("Starting AEC frequency sampling...\r\n");
    tkl_system_sleep(500);
    aec_sampling_active = 0;

    /* 验证采样结果 */
    uint8_t valid_count = 0;
    int32_t freq_sum = 0;
    int32_t freq_min = 99999;
    int32_t freq_max = 0;

    /* 计算有效范围：2000 ± 10% = 1800-2200Hz */
    const int32_t freq_min_valid = AEC_FREQ_TARGET * (100 - AEC_FREQ_TOLERANCE) / 100;
    const int32_t freq_max_valid = AEC_FREQ_TARGET * (100 + AEC_FREQ_TOLERANCE) / 100;

    bk_printf("AEC frequency validation: target=%d, range=[%d, %d]\r\n",
              AEC_FREQ_TARGET, freq_min_valid, freq_max_valid);
    bk_printf("Samples collected: %d/%d\r\n", aec_freq_sample_index, AEC_FREQ_SAMPLE_COUNT);

    for (uint8_t i = 0; i < aec_freq_sample_index; i++) {
        int freq = aec_freq_samples[i];
        bk_printf("  Sample[%d]: %d Hz", i, freq);

        if (freq >= freq_min_valid && freq <= freq_max_valid) {
            bk_printf("valid\r\n");
            valid_count++;
            freq_sum += freq;
            if (freq < freq_min) freq_min = freq;
            if (freq > freq_max) freq_max = freq;
        } else {
            bk_printf("not valid\r\n");
        }
    }

    /* 计算通过率 */
    uint8_t pass_rate = 0;
    if (aec_freq_sample_index > 0) {
        pass_rate = (valid_count * 100) / aec_freq_sample_index;
    }

    int32_t avg_freq = 0;
    if (valid_count > 0) {
        avg_freq = freq_sum / valid_count;
    }

    bk_printf("Validation result: %d/%d valid (%d%%), avg=%d, range=[%d, %d]\r\n",
              valid_count, aec_freq_sample_index, pass_rate, avg_freq, freq_min, freq_max);

    /* 判断测试结果：至少70%的样本在有效范围内 */
    BOOL_T test_passed = (pass_rate >= 70 && valid_count >= (AEC_FREQ_SAMPLE_COUNT * 7 / 10));

    if (test_passed) {
        bk_printf("AEC test SUCCESS\r\n");
        sprintf(ret_buf,
                "{\"ret\":true,\"testItem\":\"aec_result\",\"type\":\"int\",\"value\":\"%d\"}", avg_freq);
    } else {
        bk_printf("AEC test FAILED\r\n");
        sprintf(ret_buf,
                "{\"ret\":false,\"errCode\":500001,\"errMsg\":\"AEC frequency validation failed\"}");
    }

    *ret_data = (uint8_t *)ret_buf;
    *ret_len = strlen(ret_buf);

    return OPRT_OK;
}

OPERATE_RET tkl_mftest_ctrl(uint16_t cmd, uint8_t *in, uint32_t inlen, uint8_t **out, uint16_t *outlen)
{
    switch (cmd) {
        case TKL_AUDIO_TEST_START_CMD:
            return __pt_start_audio_test(cmd, in, inlen, out, outlen);

        case TKL_AUDIO_TEST_STOP_CMD:
            return __pt_stop_audio_test(cmd, in, inlen, out, outlen);

        case TKL_USB_INFO_TEST_CMD:
            return __pt_mcu_get_usb_info(cmd, in, inlen, out, outlen);

        case TKL_AUDIO_AEC_TEST_START_CMD:
            return __pt_mcu_aec_start(cmd, in, inlen, out, outlen);

        case TKL_AUDIO_AEC_GET_RESULT:
            return __pt_mcu_aec_result(cmd, in, inlen, out, outlen);
    }

    return OPRT_NOT_FOUND;
}
