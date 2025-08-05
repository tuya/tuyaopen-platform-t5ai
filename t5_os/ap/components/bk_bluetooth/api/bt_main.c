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
#if CONFIG_BLUETOOTH_HOST_ONLY
#include "bt_os_adapter.h"
#include "bluetooth_internal.h"
#include "hal_hci_core.h"
#endif
#if CONFIG_BLUETOOTH_SUPPORT_IPC
#include "bt_ipc_core.h"
#endif
#if (CONFIG_BLE_AT_ENABLE)
#include "../include/private/bk_at_ble.h"
#endif
#include <os/mem.h>

#define TAG       "bluetooth"
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

#define BT_INIT_DEINIT_TIMEOUT_MS 5000

static uint8_t bluetooth_already_init = 0;
extern int bk_bt_os_adapter_init(void);
extern int bk_bt_feature_init(void);
static beken_semaphore_t bt_sem = NULL;
static beken_mutex_t bluetooth_mutex = NULL;
static void bk_enable_bt(void)
{
    bt_err_t ret = 0;
    uint8_t cmd_data[2];
    cmd_data[0] = (BT_VENDOR_SUB_OPCODE_INIT>>8);
    cmd_data[1] = BT_VENDOR_SUB_OPCODE_INIT&0xff;
    bt_ipc_hci_send_vendor_cmd(cmd_data, sizeof(cmd_data));
    ret = rtos_get_semaphore(&bt_sem, BT_INIT_DEINIT_TIMEOUT_MS);
    if(ret != BK_OK)
    {
        LOGW("bk_enable_bt timeout!\r\n");
    }
}

static void bk_disable_bt(void)
{
    bt_err_t ret = 0;
    uint8_t cmd_data[2];
    cmd_data[0] = BT_VENDOR_SUB_OPCODE_DEINIT>>8;
    cmd_data[1] = BT_VENDOR_SUB_OPCODE_DEINIT&0xff;
    bt_ipc_hci_send_vendor_cmd(cmd_data, sizeof(cmd_data));
    ret = rtos_get_semaphore(&bt_sem, BT_INIT_DEINIT_TIMEOUT_MS);
    if(ret != BK_OK)
    {
        LOGW("bk_disable_bt timeout!\r\n");
    }
}

bt_err_t bk_bluetooth_init(void)
{
    bt_err_t ret = 0;

    LOGD("%s start, %d \r\n", __func__, bluetooth_already_init);
    if (bluetooth_already_init)
    {
        LOGW("%s bluetooth already initialised\r\n", __func__);
        return 0;
    }

    /* init semaphore */
    ret = rtos_init_semaphore(&bt_sem, 1);
    if (ret != BK_OK) {
        LOGW("init send_sema fail!\r\n");
    }

#if CONFIG_BLUETOOTH_SUPPORT_IPC
    bt_ipc_init();
#endif

    bk_enable_bt();

#if CONFIG_BLUETOOTH_HOST_ONLY
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

	hal_hci_driver_open();

    ret = bluetooth_host_init();
    if (ret)
    {
        LOGW("%s init host failed\r\n", __func__);
        return ret;
    }

#if defined (CONFIG_BLE_AT_ENABLE) && defined(CONFIG_BLE)
    extern void ble_at_cmd_init(void);
    ble_at_cmd_init();
#endif
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
    bt_err_t ret = 0;
    rtos_lock_mutex(&bluetooth_mutex);
    LOGD("%s start, %d \r\n", __func__, bluetooth_already_init);
    if (!bluetooth_already_init)
    {
        LOGW("%s bluetooth already de-initialised\r\n", __func__);
        rtos_unlock_mutex(&bluetooth_mutex);
        return 0;
    }

#if CONFIG_BLUETOOTH_HOST_ONLY
    ret = bluetooth_host_deinit();
    if (ret)
    {
        LOGW("%s deinit host failed\r\n", __func__);
        rtos_unlock_mutex(&bluetooth_mutex);
        return ret;
    }

    hal_hci_driver_close();
#endif

    bk_disable_bt();

    bluetooth_already_init = 0;

    if(bt_sem != NULL)
    {
        rtos_deinit_semaphore(&bt_sem);
        bt_sem = NULL;
    }

    rtos_unlock_mutex(&bluetooth_mutex);
    LOGD("%s ok\r\n", __func__);
    return ret;
}

#if CONFIG_BLUETOOTH_HOST_ONLY
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

ble_err_t bk_ble_tx_power_set(float pwr_gain)
{
    uint8_t cmd_data[6];
    cmd_data[0] = BT_VENDOR_SUB_OPCODE_SETPWR>>8;
    cmd_data[1] = BT_VENDOR_SUB_OPCODE_SETPWR&0xff;
    os_memcpy(&cmd_data[2], &pwr_gain , 4);
    bt_ipc_hci_send_vendor_cmd(cmd_data, sizeof(cmd_data));
    return 0;
}
#endif

void bk_bluetooth_init_deinit_compelete()
{
    if(bt_sem)
    {
        rtos_set_semaphore(&bt_sem);
    }
}

