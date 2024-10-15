#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <string.h>

#include <common/bk_include.h>
#include <components/log.h>

#include "lcd_panel_devices.h"
#include "dvp_sensor_devices.h"
#include "tp_sensor_devices.h"


#define TAG "bk_peripheral"

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)




void bk_peripheral_init(void)
{
	lcd_panel_devices_init();

#ifdef CONFIG_DVP_CAMERA
	dvp_sensor_devices_init();
#endif

#ifdef CONFIG_TP
	tp_sensor_devices_init();
#endif
}
