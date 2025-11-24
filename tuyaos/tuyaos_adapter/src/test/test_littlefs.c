#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include "bk_posix.h"
#include "sdkconfig.h"
#include "driver/qspi_flash_common.h"
#include "FreeRTOS.h"
#include "task.h"
#include "tuya_cloud_types.h"
#include "tkl_system.h"
#include "tkl_fs.h"
#include "tal_mtd_service.h"
#include <driver/qspi.h>
#include <driver/qspi_flash.h>
#include "qspi_hal.h"
#include <driver/qspi.h>
#include <driver/qspi_flash_common.h>


#if CONFIG_QSPI
#define QSPI_FLASH_ADDR    0
#define QSPI_FLASH_PREFIX  "/ext-flash"
#define INNER_QSPI_FLASH_PREFIX  "/inner-flash"

volatile static uint8_t huge_file_test_in_progress = 0;

static void __cli_lfs_mkfs(void)
{
    struct bk_little_fs_partition partition;
    char *fs_name = NULL;
    int ret;


    fs_name = "littlefs";
#ifdef CONFIG_TUYA_USE_MTD

extern MTD_DEVICE_T *tuya_mtd_device_query(const char *name);
    MTD_DEVICE_T *mtd_dev = tuya_mtd_device_query(CONFIG_TUYA_QSPI_FLASH_TYPE);
    bk_printf("mkfs mtd\r\n");
    partition.part_flash.start_addr = QSPI_FLASH_ADDR;
    partition.part_flash.size = mtd_dev->nand_dev.total_size;
    partition.part_flash.page_size = mtd_dev->nand_dev.page_size;
    partition.part_flash.block_size = mtd_dev->nand_dev.block_size;
#else
    qspi_driver_desc_t *qflash_dev = tuya_qspi_device_query("gd5f1g");
    if (qflash_dev == NULL) {
        bk_printf("Not found qspi flash %s\r\n", CONFIG_TUYA_QSPI_FLASH_TYPE);
        return;
    }

    static int __qspi_has_inited = 0;
    if (__qspi_has_inited == 0) {
        qflash_init();
        __qspi_has_inited = 1;
    }
    bk_printf("\r\nfull chip erase %s\r\n", CONFIG_TUYA_QSPI_FLASH_TYPE);
    qflash_erase(0, qflash_dev->total_size);

    bk_printf("mkfs on qspi flash, total size: %d, block size: %d\r\n",
            qflash_dev->total_size, qflash_dev->block_size);
    partition.part_flash.start_addr = QSPI_FLASH_ADDR;
    partition.part_flash.size = qflash_dev->total_size;
    partition.part_flash.page_size = qflash_dev->page_size;
    partition.part_flash.block_size = qflash_dev->block_size;
#endif
    partition.part_type = LFS_QSPI_FLASH;
    partition.mount_path = QSPI_FLASH_PREFIX;

    ret = mkfs("PART_NONE", fs_name, &partition);
    bk_printf("mkfs ret:%d\r\n", ret);
}

static void __cli_lfs_mount(void)
{
    int ret = test_fs_mount(QSPI_FLASH_PREFIX, DEV_EXT_FLASH);
    bk_printf("mount %s ret:%d\r\n", QSPI_FLASH_PREFIX, ret);
}

static void __cli_lfs_mount_inner(void)
{
    int ret = test_fs_mount(INNER_QSPI_FLASH_PREFIX, DEV_INNER_FLASH);
    bk_printf("mount %s ret:%d\r\n", INNER_QSPI_FLASH_PREFIX, ret);
}

static void __cli_lfs_umount_inner(void)
{
    int ret = test_fs_unmount(INNER_QSPI_FLASH_PREFIX);
    bk_printf("umount %s ret:%d\r\n", INNER_QSPI_FLASH_PREFIX, ret);
}

static void __cli_lfs_umount(void)
{
    int ret = test_fs_unmount(QSPI_FLASH_PREFIX);
    bk_printf("umount %s ret:%d\r\n", QSPI_FLASH_PREFIX, ret);
}

static void __cli_lfs_read(char *path, uint8_t *buf, uint32_t len)
{
    char fp[64] = {'\0'};

    if (path == NULL) {
        bk_printf("read failed, no file name spec\r\n");
        return;
    }

    sprintf(fp, "%s/%s", QSPI_FLASH_PREFIX, path);

    int fd = open(fp, O_RDONLY);
    if(fd < 0)
    {
        bk_printf("[%s][%d] open fail:%d\r\n", __FUNCTION__, __LINE__, fd);
        return ;
    }

    read(fd, buf, len);
    close(fd);
#if 0
    uint32_t debug_len = (len < 64)? len: 64;
    bk_printf("read data <display 64 bytes at most>:\r\n");
    for(int i = 0; i < debug_len; i++) {
        if (((i % 16) == 0) && (i != 0))
            bk_printf("\r\n");
        bk_printf("%02x ", buf[i]);
    }
    bk_printf("\r\n");
#endif
}

static void __cli_lfs_write(const char *path, const uint8_t *buf, uint32_t size, int mode)
{
    char fp[64] = {'\0'};

    if (path == NULL) {
        bk_printf("write failed, no file name spec\r\n");
        return;
    }

    sprintf(fp, "%s/%s", QSPI_FLASH_PREFIX, path);

    int fd = open(fp, O_RDWR | O_CREAT);
    if(fd < 0)
    {
        bk_printf("[%s][%d] open fail:%d\r\n", __FUNCTION__, __LINE__, fd);
        return ;
    }

    write(fd, buf, size);
    close(fd);

#if 0
    uint32_t debug_len = (size < 64)? size: 64;
    bk_printf("write data <display 64 bytes at most>:\r\n");
    for(int i = 0; i < debug_len; i++) {
        if (((i % 16) == 0) && (i != 0))
            bk_printf("\r\n");
        bk_printf("%02x ", buf[i]);
    }
    bk_printf("\r\n");
#endif
}

static void __cli_lfs_unlink(const char *path)
{
    char fp[64] = {'\0'};

    if (path == NULL) {
        bk_printf("unlink failed, no file name spec\r\n");
        return;
    }

    sprintf(fp, "%s/%s", QSPI_FLASH_PREFIX, path);

    unlink(fp);
}


DIR *lfs_test_dirp = NULL;
static void __cli_lfs_closedir(DIR *lfs_test_dirp)
{
    if (lfs_test_dirp == NULL)
        return;

    closedir(lfs_test_dirp);
}
static void __cli_lfs_mkdir(const char *dir)
{
    char buff[128] = {0};

    if (dir == NULL) {
        return;
    }

    if (lfs_test_dirp != NULL) {
        // close last opened dir
        __cli_lfs_closedir(lfs_test_dirp);
        lfs_test_dirp = NULL;
    }

    lfs_test_dirp = opendir(dir);
    if (lfs_test_dirp == NULL) {
        bk_printf("[%s][%d] no %s found, mkdir\r\n", __FUNCTION__, __LINE__, dir);
        // mkdir
        int fd = mkdir(dir, O_RDWR | O_CREAT);
        if(fd < 0)
        {
            bk_printf("[%s][%d] mkdir fail:%d\r\n", __FUNCTION__, __LINE__, fd);
            return ;
        }
        read(fd, buff, sizeof(buff));
        close(fd);
        bk_printf("[%s][%d] mkdir ok\r\n", __FUNCTION__, __LINE__);
    }
}

int last_file_fd = -1;
void cli_littlefs_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    if (argc < 2) {
        bk_printf("Usage: lfs mkfs\r\n");
        bk_printf("       lfs mount|umount\r\n");
        bk_printf("       mkdir [dir name] | rmdir [dir name]\r\n");
        bk_printf("       read [file] | write [file] [data] | unlink [file]\r\n");
        bk_printf("       wt [len], write file test\r\n");
        return;
    }
    bk_printf("argc: %d\r\n cmd: ", argc);
    for (int i = 0; i < argc; i++) {
        bk_printf("%s ", argv[i]);
    }
    bk_printf("\r\n");

    uint32_t tick = xTaskGetTickCount();
    if (tick < 2500) {
        bk_printf("Wait startup complete, ignore\r\n");
        return;
    }

    if (!os_strcmp(argv[1], "mkfs")) {
        __cli_lfs_mkfs();
    } else if (!os_strcmp(argv[1], "mount")) {
        __cli_lfs_mount();
    } else if (!os_strcmp(argv[1], "umount")) {
        __cli_lfs_umount();
    } else if (!os_strcmp(argv[1], "inner-mount")) {
        __cli_lfs_mount_inner();
    } else if (!os_strcmp(argv[1], "inner-umount")) {
        __cli_lfs_umount_inner();
    } else if (!os_strcmp(argv[1], "open")) {
        if (argv[2] == NULL) {
            bk_printf("no file name\r\n");
            return;
        }
        // record last opened file
        last_file_fd = open(argv[2], O_RDONLY | O_CREAT);
        if(last_file_fd < 0) {
            bk_printf("[%s][%d] open %s fail:%d\r\n", __FUNCTION__, __LINE__, argv[2], last_file_fd);
            return ;
        }
        bk_printf("[%s][%d] open file:%s, fd: %d\r\n", __FUNCTION__, __LINE__, argv[2], last_file_fd);
    } else if (!os_strcmp(argv[1], "close")) {
        int fd = -1;
        if (argv[2] == NULL) {
            int fd = last_file_fd;
            bk_printf("no fd\r\n");
            return;
        } else {
            fd = os_strtoul(argv[2], NULL, 10);
        }

        if(last_file_fd < 0) {
            // no opened file
            return;
        }
        bk_printf("[%s][%d] close the last opened file:%d\r\n", __FUNCTION__, __LINE__, last_file_fd);
        close(last_file_fd);
        last_file_fd = -1;

    } else if (!os_strcmp(argv[1], "unlink")) {
        if (argv[2] == NULL) {
            bk_printf("no file name\r\n");
            return;
        }
         //TODO if file exist
        __cli_lfs_unlink(argv[2]);
    } else if (!os_strcmp(argv[1], "read")) {
        if (argv[2] == NULL) {
            bk_printf("no file name\r\n");
            return;
        }
        if (argv[3] == NULL) {
            bk_printf("no spec data len\r\n");
            return;
        }
        char *file_path = argv[2];
        uint32_t data_len = os_strtoul(argv[3], NULL, 10);
        if ((data_len == 0) || (data_len > 500 * 1024)) {
            bk_printf("data len error: %d\r\n", data_len);
            return;
        }
        bk_printf("read %s, len: %d\r\n", file_path, data_len);

        uint8_t *buf = psram_malloc(data_len);
        if (buf == NULL) {
            bk_printf("[%s][%d] malloc fail:%d\r\n", __FUNCTION__, __LINE__, data_len);
            return;
        }

        SYS_TIME_T t0 = tkl_system_get_millisecond();
        __cli_lfs_read(file_path, buf, data_len);
        SYS_TIME_T t1 = tkl_system_get_millisecond();
        bk_printf("read %d, time: %lld = %lld - %lld\r\n", data_len, t1 - t0, t1, t0);

        uint32_t debug_len = (data_len < 64)? data_len: 64;
        bk_printf("read data <display 64 bytes at most>:\r\n");
        for(int i = 0; i < debug_len; i++) {
            if (((i % 16) == 0) && (i != 0))
                bk_printf("\r\n");
            bk_printf("%02x ", buf[i]);
        }
        bk_printf("\r\n");

        if (buf == NULL) {
            psram_free(buf);
            buf = NULL;
        }
    } else if (!os_strcmp(argv[1], "write")) {
        if (argv[2] == NULL) {
            bk_printf("no file name\r\n");
            return;
        }
        if (argv[3] == NULL) {
            bk_printf("no data\r\n");
            return;
        }
        uint32_t data_len = strlen(argv[3]);
        if (data_len == 0) {
            bk_printf("data len is zero\r\n");
            return;
        }
        __cli_lfs_write(argv[2], argv[3], data_len, 0);
    } else if (!os_strcmp(argv[1], "mkdir")) {
        if (argv[2] == NULL) {
            bk_printf("no dir\r\n");
            return;
        }
        __cli_lfs_mkdir(argv[2]);
    } else if (!os_strcmp(argv[1], "rmdir")) {
        if (argv[2] == NULL) {
            bk_printf("no dir\r\n");
            return;
        }
        rmdir(argv[2]);
    } else if (!os_strcmp(argv[1], "wt")) {
        if (huge_file_test_in_progress == 1) {
            bk_printf("last option in progress, wait and retry\r\n");
            return;
        }
#define WT_USE_PSRAM    1

        huge_file_test_in_progress = 1;
        // write file test
        uint32_t test_cnt = 0;
        uint32_t test_buf_len = 20;


        if (argv[2] != NULL) {
            test_buf_len = os_strtoul(argv[2], NULL, 10);
        }

        bk_printf("test file length: %dKB\r\n", test_buf_len);

        test_buf_len <<= 10;
        #if WT_USE_PSRAM
        uint8_t *wbuf = psram_malloc(test_buf_len);
        #else
        uint8_t *wbuf = tkl_system_malloc(test_buf_len);
        #endif
        if (wbuf == NULL) {
            bk_printf("malloc test buffer failed\r\n");
            return;
        }

        #if WT_USE_PSRAM
        uint8_t *check_buf = psram_malloc(test_buf_len);
        #else
        uint8_t *check_buf = tkl_system_malloc(test_buf_len);
        #endif
        if (check_buf == NULL) {
            bk_printf("malloc test buffer failed\r\n");
            #if WT_USE_PSRAM
            psram_free(wbuf);
            #else
            tkl_system_free(wbuf);
            #endif
            wbuf = NULL;
            return;
        }
        memset(check_buf, 0, test_buf_len);

        for (int i = 0; i < test_buf_len; i++) {
            wbuf[i] = i;
        }

        extern SYS_TIME_T tkl_system_get_millisecond(VOID_T);
        do {
            bk_printf("============> test start, %d\r\n", test_cnt++);

            bk_printf("====== write ====== \r\n");
            SYS_TIME_T t0 = tkl_system_get_millisecond();
            __cli_lfs_write("test_file", wbuf, test_buf_len, 0);
            SYS_TIME_T t1 = tkl_system_get_millisecond();
            bk_printf("====== write time: %lld = %lld - %lld\r\n", t1 - t0, t1, t0);

            memset(check_buf, 0x5a, test_buf_len);
            bk_printf("====== read ======\r\n");
            SYS_TIME_T t2 = tkl_system_get_millisecond();
            __cli_lfs_read("test_file", check_buf, test_buf_len);
            SYS_TIME_T t3 = tkl_system_get_millisecond();
            bk_printf("====== read time: %lld = %lld - %lld\r\n", t3 - t2, t3, t2);

            bk_printf("====== check ======\r\n");
            for (int i = 0; i < test_buf_len; i++) {
                if (wbuf[i] != check_buf[i]) {
                    bk_printf("!!!!!! file check error %d: %d %d !!!!!!\r\n", i, wbuf[i], check_buf[i]);
                }
            }

            bk_printf("====== delete ====== \r\n");
            __cli_lfs_unlink("test_file");

            bk_printf("<============ test end\r\n\r\n");
            tkl_system_sleep(50);
        } while (0);

        #if WT_USE_PSRAM
        psram_free(wbuf);
        wbuf = NULL;
        psram_free(check_buf);
        check_buf = NULL;
        #else
        tkl_system_free(wbuf);
        wbuf = NULL;
        tkl_system_free(check_buf);
        check_buf = NULL;
        #endif

        huge_file_test_in_progress = 0;
    }
}
#endif

