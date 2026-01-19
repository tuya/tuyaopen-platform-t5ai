// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "os/os.h"
#include "components/bluetooth/bk_dm_bluetooth.h"
#include <components/log.h>
#include "bt_os_adapter.h"
#include "bluetooth_internal.h"
#include <modules/pm.h>
#if (CONFIG_BLE_AT_ENABLE)
#include "../include/private/bk_at_ble.h"
#endif
#if CONFIG_BLUETOOTH_SUPPORT_IPC
#include "bt_ipc_core.h"
#endif
#if CONFIG_BTDM_CONTROLLER_ONLY
#include "hal_hci_core.h"
#endif
#define TAG       "bluetooth"
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

static uint8_t bluetooth_already_init = 0;

extern int bk_bt_os_adapter_init(void);
extern int bk_bt_feature_init(void);
static beken_mutex_t bluetooth_mutex = NULL;

bk_bluetooth_status_t bk_bluetooth_get_status(void)
{
    if (bluetooth_already_init)
    {
        return BK_BLUETOOTH_STATUS_ENABLED;
    }
    else
    {
        return BK_BLUETOOTH_STATUS_UNINITIALIZED;
    }
}

static int bluetooth_deepsleep_enter_cb(uint64_t expected_time_ms, void *args)
{
    if (bluetooth_already_init)
    {
#if !CONFIG_BTDM_CONTROLLER_ONLY
        bluetooth_host_deinit();
#endif

        bluetooth_controller_deinit();

        bluetooth_already_init = 0;
    }
    return 0;
}

bt_err_t bk_bluetooth_init(void)
{
    bt_err_t ret;

    if (bluetooth_already_init)
    {
        LOGW("%s bluetooth already initialised\r\n", __func__);
        return 0;
    }

    ret = bk_bt_os_adapter_init();
    if (ret)
    {
        LOGW("%s initialize bt os adapter failed\r\n", __func__);
        return ret;
    }

    if ((ret = bk_bt_feature_init()) != 0)
    {
        LOGW("%s initialize bt feature failed\r\n", __func__);
        return ret;
    }

#if CONFIG_BLUETOOTH_SUPPORT_IPC
    bt_ipc_init();
#endif

    ret = bluetooth_controller_init();
    if (ret)
    {
        LOGW("%s initialize controller failed\r\n", __func__);
        return ret;
    }

#if !CONFIG_BTDM_CONTROLLER_ONLY
    ret = bluetooth_host_init();
    if (ret)
    {
        LOGW("%s init host failed\r\n", __func__);
        return ret;
    }
#else
    hal_hci_driver_open();
#endif

    pm_cb_conf_t enter_conf_bt = {NULL, NULL};
    enter_conf_bt.cb = bluetooth_deepsleep_enter_cb;
    bk_pm_sleep_register_cb(PM_MODE_DEEP_SLEEP, PM_DEV_ID_BTDM, &enter_conf_bt, NULL);
#if CONFIG_PM_SUPER_DEEP_SLEEP
    bk_pm_sleep_register_cb(PM_MODE_SUPER_DEEP_SLEEP, PM_DEV_ID_BTDM, &enter_conf_bt, NULL);
#endif
#if defined (CONFIG_BLE_AT_ENABLE) && !defined(CONFIG_BTDM_CONTROLLER_ONLY) && defined(CONFIG_BLE)
    extern void ble_at_cmd_init(void);
    ble_at_cmd_init();
#endif
    if (bluetooth_mutex == NULL)
    {
        rtos_init_mutex(&bluetooth_mutex);
    }
	
    bluetooth_already_init = 1;
    LOGD("%s ok\r\n", __func__);
    return ret;
}

bt_err_t bk_bluetooth_deinit(void)
{
    bt_err_t ret;
    rtos_lock_mutex(&bluetooth_mutex);
    if (!bluetooth_already_init)
    {
        LOGW("%s bluetooth already de-initialised\r\n", __func__);
        rtos_unlock_mutex(&bluetooth_mutex);
        return 0;
    }
    LOGD("%s start, %d \r\n", __func__, bluetooth_already_init);
#if !CONFIG_BTDM_CONTROLLER_ONLY
    ret = bluetooth_host_deinit();
    if (ret)
    {
        LOGW("%s deinit host failed\r\n", __func__);
        rtos_unlock_mutex(&bluetooth_mutex);
        return ret;
    }
#else
    hal_hci_driver_close();
#endif

    ret = bluetooth_controller_deinit();
    if (ret)
    {
        LOGW("%s deinit controller failed\r\n", __func__);
        rtos_unlock_mutex(&bluetooth_mutex);
        return ret;
    }

    bk_pm_sleep_unregister_cb(PM_MODE_DEEP_SLEEP, PM_DEV_ID_BTDM, true, false);
#if CONFIG_PM_SUPER_DEEP_SLEEP
    bk_pm_sleep_unregister_cb(PM_MODE_SUPER_DEEP_SLEEP, PM_DEV_ID_BTDM, true, false);
#endif

    bluetooth_already_init = 0;

    LOGD("%s ok, %d \r\n", __func__, bluetooth_already_init);
    rtos_unlock_mutex(&bluetooth_mutex);
    return ret;
}

bt_err_t bk_bluetooth_get_address(uint8_t *addr)
{
    bt_err_t ret;

    if (!bluetooth_already_init)
    {
        return -1;
    }

    ret = bluetooth_get_mac(addr);

    return ret;
}


