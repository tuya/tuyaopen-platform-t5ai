#pragma once

#include <components/log.h>


#define HCI_LOGV(...) BK_LOGV("hcidrv", ##__VA_ARGS__)
#define HCI_LOGD(...) BK_LOGD("hcidrv", ##__VA_ARGS__)
#define HCI_LOGI(...) BK_LOGI("hcidrv", ##__VA_ARGS__)
#define HCI_LOGW(...) BK_LOGW("hcidrv", ##__VA_ARGS__)
#define HCI_LOGE(...) BK_LOGE("hcidrv", ##__VA_ARGS__)


enum
{
    HCI_CMD_TYPE                           = 0x01,
    HCI_ACL_TYPE                           = 0x02,
    HCI_SYNC_TYPE                          = 0x03,
    HCI_EVT_TYPE                           = 0x04,
};

int hal_hci_driver_open(void);
int hal_hci_driver_close(void);
