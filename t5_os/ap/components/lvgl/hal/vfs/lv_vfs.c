#include "os/os.h"
#include "bk_posix.h"
#include "driver/flash_partition.h"


#if (CONFIG_FATFS)
static int _fs_mount(void)
{
    struct bk_fatfs_partition partition;
    char *fs_name = NULL;
    int ret;

    fs_name = "fatfs";
    partition.part_type = FATFS_DEVICE;
#if (CONFIG_SDCARD)
    partition.part_dev.device_name = FATFS_DEV_SDCARD;
    partition.mount_path = VFS_SD_0_PATITION_0;
#else
    partition.part_dev.device_name = FATFS_DEV_FLASH;
    partition.mount_path = VFS_INTERNAL_FLASH_PATITION_0;
#endif

    ret = mount("SOURCE_NONE", partition.mount_path, fs_name, 0, &partition);

    return ret;
}
#endif

#if (CONFIG_LITTLEFS)
static int _fs_mount_lfs(void)
{
    int ret;

    struct bk_little_fs_partition partition;
    char *fs_name = NULL;
#ifdef BK_PARTITION_LITTLEFS
    bk_logic_partition_t *pt = bk_flash_partition_get_info(BK_PARTITION_LITTLEFS);
#else
    bk_logic_partition_t *pt = bk_flash_partition_get_info(BK_PARTITION_USR_CONFIG);
#endif

    fs_name = "littlefs";
    partition.part_type = LFS_FLASH;
    partition.part_flash.start_addr = pt->partition_start_addr;
    partition.part_flash.size = pt->partition_length;
    partition.mount_path = VFS_INTERNAL_FLASH_PATITION_0;

    ret = mount("SOURCE_NONE", partition.mount_path, fs_name, 0, &partition);

    return ret;
}
#endif

bk_err_t lv_vfs_init(void)
{
    bk_err_t ret = BK_FAIL;

    do {
#if (CONFIG_FATFS)
        ret = _fs_mount();
        if (BK_OK != ret)
        {
            BK_LOGD(NULL, "[%s][%d] mount fail:%d\r\n", __FUNCTION__, __LINE__, ret);
            break;
        }
#endif
#if (CONFIG_LITTLEFS)
        ret = _fs_mount_lfs();
        if (BK_OK != ret)
        {
            BK_LOGD(NULL, "[%s][%d] mount fail:%d\r\n", __FUNCTION__, __LINE__, ret);
            break;
        }
#endif
        BK_LOGD(NULL, "[%s][%d] mount success\r\n", __FUNCTION__, __LINE__);
    } while(0);

    return ret;
}

bk_err_t lv_vfs_deinit(void)
{
    bk_err_t ret = BK_FAIL;

#if (CONFIG_FATFS)
#if (CONFIG_SDCARD)
    ret = umount(VFS_SD_0_PATITION_0);
#else
    ret = umount(VFS_INTERNAL_FLASH_PATITION_0);
#endif
#endif

#if (CONFIG_LITTLEFS)
    ret = umount(VFS_INTERNAL_FLASH_PATITION_0);
#endif

    if (BK_OK != ret) {
        BK_LOGD(NULL, "[%s][%d] unmount fail:%d\r\n", __FUNCTION__, __LINE__, ret);
    }

    BK_LOGD(NULL, "[%s][%d] unmount success\r\n", __FUNCTION__, __LINE__);

    return ret;
}

