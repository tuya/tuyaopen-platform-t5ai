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
#if CONFIG_BLUETOOTH_SUPPORT_IPC
#include "bt_ipc_core.h"
#endif

#define TAG       "bluetooth"
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)

#define BT_INIT_DEINIT_TIMEOUT_MS 5000

static uint8_t bluetooth_already_init = 0;
static beken_semaphore_t bt_sem = NULL;

static void bk_enable_bt(void)
{
    bt_err_t ret = 0;
    uint8_t cmd_data[2];
    cmd_data[0] = (BT_INIT_VENDOR_SUB_OPCODE>>8);
    cmd_data[1] = BT_INIT_VENDOR_SUB_OPCODE&0xff;
    bt_ipc_hci_send_vendor_cmd(cmd_data, sizeof(cmd_data));
    ret = rtos_get_semaphore(&bt_sem, BT_INIT_DEINIT_TIMEOUT_MS);
    if(ret != BK_OK)
    {
        LOGE("bk_enable_bt timeout!\r\n");
    }
}

static void bk_disable_bt(void)
{
    bt_err_t ret = 0;
    uint8_t cmd_data[2];
    cmd_data[0] = BT_DEINIT_VENDOR_SUB_OPCODE>>8;
    cmd_data[1] = BT_DEINIT_VENDOR_SUB_OPCODE&0xff;
    bt_ipc_hci_send_vendor_cmd(cmd_data, sizeof(cmd_data));
    ret = rtos_get_semaphore(&bt_sem, BT_INIT_DEINIT_TIMEOUT_MS);
    if(ret != BK_OK)
    {
        LOGE("bk_disable_bt timeout!\r\n");
    }
}

bt_err_t bk_bluetooth_init(void)
{
    bt_err_t ret = 0;

    LOGD("%s start, %d \r\n", __func__, bluetooth_already_init);
    if (bluetooth_already_init)
    {
        LOGE("%s bluetooth already initialised\r\n", __func__);
        return 0;
    }

    /* init semaphore */
    ret = rtos_init_semaphore(&bt_sem, 1);
    if (ret != BK_OK) {
        LOGE("init send_sema fail!\r\n");
    }

#if CONFIG_BLUETOOTH_SUPPORT_IPC
    bt_ipc_init();
#endif

    bk_enable_bt();

    bluetooth_already_init = 1;

    LOGD("%s ok\r\n", __func__);
    return ret;
}

bt_err_t bk_bluetooth_deinit(void)
{
    bt_err_t ret = 0;

    LOGD("%s start, %d \r\n", __func__, bluetooth_already_init);
    if (!bluetooth_already_init)
    {
        LOGE("%s bluetooth already de-initialised\r\n", __func__);
        return 0;
    }

    bk_disable_bt();

    bluetooth_already_init = 0;

    if(bt_sem != NULL)
    {
        rtos_deinit_semaphore(&bt_sem);
        bt_sem = NULL;
    }

    LOGD("%s ok\r\n", __func__);
    return ret;
}

void bk_bluetooth_init_deinit_compelete()
{
    if(bt_sem)
    {
        rtos_set_semaphore(&bt_sem);
    }
}

