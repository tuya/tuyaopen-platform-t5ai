#include <stdio.h>
#include <string.h>
#include <os/os.h>
#include <common/bk_include.h>
#include "ff.h"
#include "diskio.h"
#include "../lv_conf.h"


#if (CONFIG_FATFS) && (LV_USE_FS_FATFS)
FATFS *fs = NULL;
static void lv_fatfs_mount(int number)
{
    FRESULT fr;
    char cFileName[FF_MAX_LFN];

    if (fs != NULL)
    {
        os_free(fs);
    }

    fs = os_malloc(sizeof(FATFS));
    if(NULL == fs)
    {
        BK_LOGD(NULL, "f_mount malloc failed!\r\n");
        goto failed_mount;
    }

    sprintf(cFileName, "%d:", number);
    fr = f_mount(fs, cFileName, 1);
    if (fr != FR_OK)
    {
        BK_LOGD(NULL, "f_mount failed:%d\r\n", fr);
    }
    else
    {
        BK_LOGD(NULL, "f_mount OK!\r\n");
    }

failed_mount:
    BK_LOGD(NULL, "----- test_mount %d over -----\r\n\r\n", number);
}


static void lv_fatfs_unmount(int number)
{
    FRESULT fr;
    char cFileName[FF_MAX_LFN];
    sprintf(cFileName, "%d:", number);
    fr = f_unmount(LV_FS_FATFS_DISK_NUM, cFileName, 1);
    if (fr != FR_OK)
    {
        BK_LOGD(NULL, "f_unmount failed:%d\r\n", fr);
    }
    else
    {
        os_free(fs);
        BK_LOGD(NULL, "f_unmount OK!\r\n");
    }

    BK_LOGD(NULL, "----- test_unmount %d over -----\r\n\r\n", number);
}

void lv_fatfs_init(void)
{
#if (CONFIG_SDCARD)
    lv_fatfs_mount(DISK_NUMBER_SDIO_SD);
#else
    char vol[3];
    lv_fatfs_mount(LV_FS_FATFS_DISK_NUM);
    sprintf(vol, "%d:", LV_FS_FATFS_DISK_NUM);
    f_chdrive(vol);
#endif
}

void lv_fatfs_deinit(void)
{
#if (CONFIG_SDCARD)
    lv_fatfs_unmount(DISK_NUMBER_SDIO_SD);
#else
    lv_fatfs_unmount(LV_FS_FATFS_DISK_NUM);
#endif
}
#endif
