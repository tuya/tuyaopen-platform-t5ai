#include "host/ethermind_impl.h"
#include <string.h>
#include "bluetooth/bluetooth.h"
#include "bluetooth/mesh.h"
#include "bluetooth/hci.h"
#include "bluetooth/buf.h"
#include "host/hci_core.h"
//#include <os/mem.h>
#include "host/hci_core.h"


bool zephyr_is_stack_inited(void)
{
    return atomic_test_bit(bt_dev.flags, BT_DEV_READY);
}

uint8_t zephyr_is_scan_enable(void)
{
    return atomic_test_bit(bt_dev.flags, BT_DEV_SCANNING);
}

uint8_t zephyr_get_adv_handle(void)
{
    return CONFIG_BT_EXT_ADV_MAX_ADV_SET - 1;
}

