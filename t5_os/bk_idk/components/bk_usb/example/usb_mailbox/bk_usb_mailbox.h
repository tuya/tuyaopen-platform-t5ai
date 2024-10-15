#include <common/bk_include.h>
#include <driver/mailbox_channel.h>
#include <components/usb.h>
#include <components/usb_types.h>
#include "os/os.h"
#include "os/mem.h"
#include "components/log.h"
#include <driver/pwr_clk.h>
#include <modules/pm.h>
#include <driver/gpio.h>
#include <driver/gpio_types.h>
#include "gpio_driver.h"

#define TAG "USB_MAILBOX"
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)

typedef struct {
	bk_usb_drv_op_t op;
	mb_chnl_cmd_t param;
} bk_usb_mailbox_msg_t;

