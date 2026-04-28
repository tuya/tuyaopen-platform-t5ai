/*
 * test_common.c
 * Copyright (C) 2025 cc <cc@tuya>
 *
 * Distributed under terms of the TUYA license.
 */

#include "cli_tuya_test.h"
#include "tkl_fs.h"
#include <os/os.h>
#include "bk_misc.h"

static int qflash_ref = 0;
static int sdcard_ref = 0;
static int inner_flash_ref = 0;

int test_fs_mount(const char *path, FS_DEV_TYPE_T dev_type)
{
    int ret = 0;
    int is_need_mount = 0;
    (void)path;

    uint32_t level = rtos_enter_critical();
    if ((dev_type == DEV_EXT_FLASH && qflash_ref == 0) ||
        (dev_type == DEV_INNER_FLASH && inner_flash_ref == 0) ||
        (dev_type == DEV_SDCARD && sdcard_ref == 0)) {
        is_need_mount = 1;
    }
    rtos_exit_critical(level);

    if (is_need_mount) {
        ret = tkl_fs_mount(path, dev_type);
        if (ret == 0) {
            level = rtos_enter_critical();
            if (dev_type == DEV_EXT_FLASH) {
                qflash_ref++;
            } else if (dev_type == DEV_SDCARD) {
                sdcard_ref++;
            } else if (dev_type == DEV_INNER_FLASH) {
                inner_flash_ref++;
            }
            rtos_exit_critical(level);
        } else {
            bk_printf("mount %s failed\r\n", path);
        }
    }

    return ret;
}

int test_fs_unmount(const char *path)
{
    int ret = 0;
    FS_DEV_TYPE_T dev_type;
    int need_umount = 0;
    int original_ref = 0;

    if (os_strcmp(path, "/sdcard") == 0) {
        dev_type = DEV_SDCARD;
    } else if (os_strcmp(path, "/ext-flash") == 0) {
        dev_type = DEV_EXT_FLASH;
    } else if (os_strcmp(path, "/inner-flash") == 0) {
        dev_type = DEV_INNER_FLASH;
    } else {
        return -1;
    }

    uint32_t level = rtos_enter_critical();
    if (dev_type == DEV_EXT_FLASH) {
        if (qflash_ref > 0) {
            original_ref = qflash_ref;
            qflash_ref--;
            if (qflash_ref == 0) {
                need_umount = 1;
            }
        } else {
            rtos_exit_critical(level);
            return 0;
        }
    } else if (dev_type == DEV_SDCARD) {
        if (sdcard_ref > 0) {
            original_ref = sdcard_ref;
            sdcard_ref--;
            if (sdcard_ref == 0) {
                need_umount = 1;
            }
        } else {
            rtos_exit_critical(level);
            return 0;
        }
    } else if (dev_type == DEV_INNER_FLASH) {
        if (inner_flash_ref > 0) {
            original_ref = inner_flash_ref;
            inner_flash_ref--;
            if (inner_flash_ref == 0) {
                need_umount = 1;
            }
        } else {
            rtos_exit_critical(level);
            return 0;
        }
    }
    rtos_exit_critical(level);

    if (need_umount) {
        ret = tkl_fs_unmount(path);
        if (ret != 0) {
            level = rtos_enter_critical();
            if (dev_type == DEV_EXT_FLASH) {
                qflash_ref = original_ref;
            } else if (dev_type == DEV_SDCARD) {
                sdcard_ref = original_ref;
            } else if (dev_type == DEV_INNER_FLASH) {
                inner_flash_ref = original_ref;
            }
            rtos_exit_critical(level);
            bk_printf("unmount %s failed\r\n", path);
        }
    }

    return ret;
}

