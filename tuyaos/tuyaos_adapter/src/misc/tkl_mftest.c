#include "tkl_mftest.h"
#include "tkl_memory.h"

extern void cli_audio_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv);

/**
 * @brief 模组固件升级更新结束，0x11
 * @note 设备应用层只做状态的处理，不对数据做处理
 */
static OPERATE_RET __pt_start_audio_test(uint16_t cmd, uint8_t *data, uint32_t len, uint8_t **ret_data, uint16_t *ret_len)
{
    uint8_t flag = 0;
    bk_printf("__pt_start_audio_record_cb\r\n");
    bk_printf("=============================\r\n");
    bk_printf("mic channel %x\r\n", data[0]);
    bk_printf("=============================\r\n");

    // char *stop[5] = {"audio_test", "0", "44100", "1", "xxx"};
    // cli_audio_test_cmd(NULL, 0, 4, &stop);

    if (data[0] == 1) {
        TUYA_GPIO_BASE_CFG_T cfg;
        cfg.mode = TUYA_GPIO_FLOATING;
        cfg.direct = TUYA_GPIO_INPUT;
        tkl_gpio_init(TUYA_GPIO_NUM_28, &cfg);


        char *argv[5] = {"audio_test", "1", "44100", "1", "xxx"};
        cli_audio_test_cmd(NULL, 0, 4, &argv);
    } else if (data[0] == 2) {

        char *argv[5] = {"audio_test", "2", "44100", "1", "xxx"};
        cli_audio_test_cmd(NULL, 0, 4, &argv);
    }

    bk_printf("=============================\r\n");
    TUYA_GPIO_LEVEL_E level;
    tkl_gpio_read(TUYA_GPIO_NUM_28, &level);
    bk_printf("gpio: %x\r\n", level);
    bk_printf("=============================\r\n");

    char *out = tkl_system_malloc(64);
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
    bk_printf("__pt_stop_audio_record_cb\r\n");
    char *argv[5] = {"audio_test", "0", "44100", "1", "xxx"};
    cli_audio_test_cmd(NULL, 0, 4, &argv);
    // __test_audio_deinit();

    char *out = tkl_system_malloc(64);
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
    char *ret_buf = tkl_system_malloc(128);
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

OPERATE_RET tkl_mftest_ctrl(uint16_t cmd, uint8_t *in, uint32_t inlen, uint8_t **out, uint16_t *outlen)
{
    switch (cmd) {

    case TKL_AUDIO_TEST_START_CMD:
        return __pt_start_audio_test(cmd, in, inlen, out, outlen);

    case TKL_AUDIO_TEST_STOP_CMD:
        return __pt_stop_audio_test(cmd, in, inlen, out, outlen);

    case TKL_USB_INFO_TEST_CMD:
        return __pt_mcu_get_usb_info(cmd, in, inlen, out, outlen);
    }

    return OPRT_NOT_FOUND;
}
