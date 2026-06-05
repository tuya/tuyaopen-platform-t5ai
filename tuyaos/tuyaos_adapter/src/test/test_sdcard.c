/*
 * test_sdcard.c
 * Copyright (C) 2025 cc <cc@tuya>
 *
 * Distributed under terms of the MIT license.
 */

#include "cli.h"
#include "cli_tuya_test.h"
#include "tuya_cloud_types.h"

// #include "ff.h"         /* FatFS头文件 */
// #include "ffconf.h"     /* FatFS配置 */

#include <common/bk_include.h>
// #include "diskio.h"
#include "bk_posix.h"
#include "driver/sd_card_types.h"
#include "tkl_fs.h"
#include "tkl_memory.h"


static int __cli_sdcard_mount(void)
{
    return test_fs_mount(TEST_SDCARD_MOUNT_POINT, DEV_SDCARD);
}

static void __cli_sdcard_umount(void)
{
    test_fs_unmount(TEST_SDCARD_MOUNT_POINT);
}

static void __cli_sdcard_unlink(const char *path)
{
    char buf[64] = {'\0'};
    char fp[64] = {'\0'};

    if (path == NULL) {
        bk_printf("rm failed, no file name spec\r\n");
        return;
    }

    sprintf(fp, "%s/%s", TEST_SDCARD_MOUNT_POINT, path);

    int ret = tkl_fs_remove(fp);
    if (ret != 0) {
        bk_printf("rm %s failed\r\n", path);
        return;
    }

    bk_printf("rm file: %s\r\n", path);
}

static void __cli_sdcard_read(const char *path)
{
    char buf[64] = {'\0'};
    char fp[64] = {'\0'};

    if (path == NULL) {
        bk_printf("read failed, no file name spec\r\n");
        return;
    }

    sprintf(fp, "%s/%s", TEST_SDCARD_MOUNT_POINT, path);

    TUYA_FILE f = tkl_fopen(fp, "r");
    if (f == NULL) {
        bk_printf("open %s failed\r\n", path);
        return;
    }

    tkl_fread(buf, 64, f);

    tkl_fclose(f);

    bk_printf("read: %s\r\n", buf);
}

static void __cli_sdcard_write(const char *path, const uint8_t *buf, uint32_t size)
{
    if (path == NULL) {
        bk_printf("write failed, no file name spec\r\n");
        return;
    }
    char fp[64] = {'\0'};

    sprintf(fp, "%s/%s", TEST_SDCARD_MOUNT_POINT, path);

    TUYA_FILE f = tkl_fopen(fp, "ab");
    if (f == NULL) {
        bk_printf("open %s failed\r\n", path);
        return;
    }

    tkl_fwrite(buf, size, f);

    tkl_fclose(f);
}

static void __cli_sdcard_list_file(const char* path)
{
    INT_T res = 0, is_dir = 0;
    TUYA_DIR dir;
    TUYA_FILEINFO info;
    char *f_name = NULL;

    res = tkl_dir_open(path, &dir);                 /* Open the directory */
    if (res == 0) {
        bk_printf("%s\r\n", path);
        while (1) {
            res = tkl_dir_read(dir, &info);         /* Read a directory item */
            if (res != 0) {
                break;  /* Break on error */
            }

            tkl_dir_name(info, &f_name);

            if (f_name == NULL) {
                break;  /* Break on end of dir */
            }

            tkl_dir_is_directory(info, &is_dir);
            if (is_dir) {
                /* It is a directory */
                char *pathTemp = tkl_system_malloc(strlen(path)+strlen(f_name)+2);
                if(pathTemp == NULL) {
                    bk_printf("%s:os_malloc dir failed \r\n", __func__);
                    break;
                }
                sprintf(pathTemp, "%s/%s", path, f_name);
                __cli_sdcard_list_file(pathTemp);      /* Enter the directory */
                tkl_system_free(pathTemp);
            } else {
                /* It is a file. */
                bk_printf("%s/%s\r\n", path, f_name);
            }
        }
        tkl_dir_close(dir);
    } else {
        bk_printf("f_opendir failed\r\n");
    }

    return;
}

static TUYA_DIR sdcard_test_dirp = NULL;
static void __cli_sdcard_closedir(TUYA_DIR sdcard_test_dirp)
{
    if (sdcard_test_dirp == NULL)
        return;

    tkl_dir_close(sdcard_test_dirp);
}

static void __cli_sdcard_mkdir(const char *dir_path)
{
    char buff[128] = {0};

    if (dir_path == NULL) {
        return;
    }

    if (sdcard_test_dirp != NULL) {
        // close last opened dir
        __cli_sdcard_closedir(sdcard_test_dirp);
        sdcard_test_dirp = NULL;
    }

    tkl_dir_open(dir_path, &sdcard_test_dirp);
    if (sdcard_test_dirp == NULL) {
        bk_printf("[%s][%d] no %s found, mkdir\r\n", __FUNCTION__, __LINE__, dir_path);
        // mkdir
        int fd = tkl_fs_mkdir(dir_path);
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



static void __cli_sdcard_speed_test_opt(uint32_t size)
{
    uint32_t cnt = 0;
    char *path = "speed_file";
    char fp[64] = {'\0'};

    sprintf(fp, "%s/%s", TEST_SDCARD_MOUNT_POINT, path);

    TUYA_FILE f = tkl_fopen(fp, "wb");
    if (f == NULL) {
        bk_printf("open %s failed\r\n", path);
        return;
    }

    uint32_t test_file_size = (12 * 1024 * 1024);
    uint32_t test_buf_size = size;

    uint8_t *buf = tkl_system_psram_malloc(test_buf_size);
    uint32_t test_times = (test_file_size / test_buf_size) - 1;

    SYS_TIME_T t1 = tkl_system_get_millisecond();
    do {
        tkl_fwrite(buf, test_buf_size, f);
    } while (cnt++ < test_times);

    tkl_fsync((int)f);

    SYS_TIME_T t2 = tkl_system_get_millisecond();
    uint32_t speed = ((test_file_size >> 20) * 1000 * 100) / (t2 - t1);
    uint32_t integer_num = speed / 100;
    uint32_t dec_num = speed % 100;
    bk_printf("time: %lld - %lld = %lld, speed: %d.%d MB/s\r\n", t2, t1, t2 - t1, integer_num, dec_num);

    tkl_fclose(f);
    tkl_system_psram_free(buf);
    buf = NULL;
}
static void __cli_sdcard_speed_test(void)
{
    __cli_sdcard_speed_test_opt(256 * 1024);
    tkl_system_sleep(100);
}

extern OPERATE_RET tkl_io_pinmux_config(TUYA_PIN_NAME_E pin, TUYA_PIN_FUNC_E pin_func);
static void __cli_sdcard_port_test(void)
{
    tkl_io_pinmux_config(TUYA_IO_PIN_14, TUYA_SDIO_CLK);
    tkl_io_pinmux_config(TUYA_IO_PIN_15, TUYA_SDIO_CMD);
    tkl_io_pinmux_config(TUYA_IO_PIN_16, TUYA_SDIO_DATA0);
}

void cli_sdcard_test_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    if (argc < 2) {
        bk_printf("invalid argc num\r\n");
        return;
    }

    if (!os_strcmp(argv[1], "mount")) {
        __cli_sdcard_mount();
    } else if (!os_strcmp(argv[1], "umount")) {
        __cli_sdcard_umount();
    } else if (!os_strcmp(argv[1], "port")) {
        __cli_sdcard_port_test();
    } else if (!os_strcmp(argv[1], "ls")) {
        __cli_sdcard_list_file(TEST_SDCARD_MOUNT_POINT);
    } else if (!os_strcmp(argv[1], "read")) {
        if (argv[2] == NULL) {
            bk_printf("no file name\r\n");
            return;
        }
        __cli_sdcard_read(argv[2]);
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

        __cli_sdcard_write(argv[2], argv[3], data_len);
    } else if (!os_strcmp(argv[1], "rmdir")) {
        if (argv[2] == NULL) {
            bk_printf("no spec dir\r\n");
            return;
        }
        tkl_fs_remove(argv[2]);
    } else if (!os_strcmp(argv[1], "mkdir")) {
        if (argv[2] == NULL) {
            bk_printf("no spec dir\r\n");
            return;
        }
        __cli_sdcard_mkdir(argv[2]);
    } else if (!os_strcmp(argv[1], "delete")) {
        if (argv[2] == NULL) {
            bk_printf("no file name\r\n");
            return;
        }
        __cli_sdcard_unlink(argv[2]);
    } else if (!os_strcmp(argv[1], "format")) {
        bk_printf("TODO...\r\n");
    } else if (!os_strcmp(argv[1], "auto")) {
        int ret = __cli_sdcard_mount();
        if (ret != 0)
            return;
        __cli_sdcard_list_file(TEST_SDCARD_MOUNT_POINT);
        __cli_sdcard_umount();
        bk_sd_card_deinit();
    } else if (!os_strcmp(argv[1], "speed_test")) {
        __cli_sdcard_mount();
        __cli_sdcard_speed_test();
        __cli_sdcard_umount();
        bk_sd_card_deinit();
    } else {

    }
}

