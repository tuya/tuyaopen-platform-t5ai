/**
 * @file test_fs.c
 * @brief Generic filesystem test (sdcard / udisk / littlefs) cli implementation
 *
 * @copyright Copyright (c) 2025 Tuya Inc. All Rights Reserved.
 *
 * Permission is hereby granted, to any person obtaining a copy of this software and
 * associated documentation files (the "Software"), Under the premise of complying
 * with the license of the third-party open source software contained in the software,
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software.
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 */

/* ---------------------------------------------------------------------------
 * Includes
 * --------------------------------------------------------------------------- */
#include "cli_tuya_test.h"

#if CONFIG_QSPI
#include "driver/qspi_flash_common.h"
#include <driver/qspi.h>
#include <driver/qspi_flash.h>
#ifdef CONFIG_TUYA_USE_MTD
#include "tal_mtd_service.h"
#endif
#endif /* CONFIG_QSPI */

/* ---------------------------------------------------------------------------
 * Macros
 * --------------------------------------------------------------------------- */
#define TEST_FS_PATH_MAX        128
#define TEST_FS_READ_DUMP_MAX   64
#define TEST_FS_READ_LEN_MAX    (500 * 1024)
#define TEST_FS_READ_LEN_DEF    64
#define TEST_FS_UDISK_RETRY_MS  200

/* write_large_file_test defaults */
#define TEST_FS_WT_DEFAULT_KB   500
#define TEST_FS_WT_MAX_KB       (4 * 1024)        /* 4 MB upper bound */
#define TEST_FS_WT_FILE_NAME    "wt_test.bin"
#define TEST_FS_WT_CHUNK_SZ     (32 * 1024)       /* per-call write chunk */

/* mkfs (littlefs only) */
#define TEST_FS_LFS_FS_NAME     "littlefs"
#define TEST_FS_LFS_QSPI_ADDR   0

/* ---------------------------------------------------------------------------
 * Type definitions
 * --------------------------------------------------------------------------- */
typedef void (*FS_CMD_FN_T)(int argc, char **argv);

typedef struct {
    const char     *name;       /* sub-command name (argv[1]) */
    FS_CMD_FN_T     handler;    /* sub-command handler */
    int             min_argc;   /* minimum argc, includes argv[0] and argv[1] */
    const char     *usage;      /* one-line usage hint, no leading 'xfs ' */
} FS_SUB_CMD_T;

typedef struct {
    const char     *name;       /* device tag, e.g. "sdcard" / "udisk" */
    const char     *path;       /* default mount point */
    FS_DEV_TYPE_T   dev_type;   /* tkl fs device type */
} FS_DEV_ENTRY_T;

/* ---------------------------------------------------------------------------
 * File scope variables
 * --------------------------------------------------------------------------- */
static const FS_DEV_ENTRY_T s_fs_devs[] = {
    {"sdcard",   TEST_FS_SDCARD_MOUNT_POINT,   DEV_SDCARD   },
    {"udisk",    TEST_FS_UDISK_MOUNT_POINT,    DEV_USB_DISK },
    {"littlefs", TEST_FS_LITTLEFS_MOUNT_POINT, DEV_EXT_FLASH},
};
#define TEST_FS_DEV_NUM (sizeof(s_fs_devs) / sizeof(s_fs_devs[0]))

/* per-device mount state, indexed parallel to s_fs_devs */
static BOOL_T s_fs_mounted[TEST_FS_DEV_NUM] = {0};

/* serialise long-running write_large_file_test runs */
static volatile uint8_t s_fs_wt_in_progress = 0;

/* ---------------------------------------------------------------------------
 * Function implementations
 * --------------------------------------------------------------------------- */
/**
 * @brief Lookup device entry index by name
 * @param[in] name device tag, may be NULL
 * @return index in s_fs_devs on success, -1 on miss
 */
static int __fs_dev_index(const char *name)
{
    if (name == NULL) {
        return -1;
    }
    for (uint32_t i = 0; i < TEST_FS_DEV_NUM; i++) {
        if (os_strcmp(s_fs_devs[i].name, name) == 0) {
            return (int)i;
        }
    }
    return -1;
}

/**
 * @brief Resolve device index from argv[2] with unified diagnostic output
 * @param[in] argc argument count
 * @param[in] argv argument vector
 * @param[in] need_mounted require the device to be mounted
 * @return device index on success, -1 on missing/unknown/not-mounted
 */
static int __fs_resolve_dev(int argc, char **argv, BOOL_T need_mounted)
{
    if ((argc < 3) || (argv[2] == NULL)) {
        bk_printf("missing <dev>, expect: sdcard | udisk | littlefs\r\n");
        return -1;
    }
    int idx = __fs_dev_index(argv[2]);
    if (idx < 0) {
        bk_printf("unknown device: %s, expect: sdcard | udisk | littlefs\r\n", argv[2]);
        return -1;
    }
    if ((need_mounted == TRUE) && (s_fs_mounted[idx] == FALSE)) {
        bk_printf("%s not mounted, please mount first\r\n", s_fs_devs[idx].name);
        return -1;
    }
    return idx;
}

/**
 * @brief Build a full path under the given device's mount point
 * @param[out] out output buffer
 * @param[in] out_size size of out buffer (bytes)
 * @param[in] idx device index in s_fs_devs (must be valid)
 * @param[in] sub sub path under mount point; may be NULL or empty
 * @return 0 on success, negative on error
 */
static int __fs_build_path(char *out, uint32_t out_size, int idx, const char *sub)
{
    if ((out == NULL) || (out_size == 0) || (idx < 0) || (idx >= (int)TEST_FS_DEV_NUM)) {
        return -1;
    }

    const char *mp = s_fs_devs[idx].path;
    int n;
    if ((sub == NULL) || (sub[0] == '\0')) {
        n = snprintf(out, out_size, "%s", mp);
    } else if (sub[0] == '/') {
        n = snprintf(out, out_size, "%s%s", mp, sub);
    } else {
        n = snprintf(out, out_size, "%s/%s", mp, sub);
    }

    if ((n < 0) || ((uint32_t)n >= out_size)) {
        bk_printf("path too long\r\n");
        return -1;
    }
    return 0;
}

/**
 * @brief Recursively list the directory tree under @p path
 * @param[in] path absolute directory path inside a mounted filesystem
 * @return none
 * @note Recursion depth is bounded by the underlying filesystem path depth.
 *       All operations go through the tkl_fs abstraction; no POSIX is used.
 */
static void __fs_op_list(const char *path)
{
    TUYA_DIR dir = NULL;
    TUYA_FILEINFO info = NULL;
    const char *name = NULL;
    BOOL_T is_dir = FALSE;
    char child[TEST_FS_PATH_MAX] = {0};

    if (path == NULL) {
        return;
    }

    int ret = tkl_dir_open(path, &dir);
    if ((ret != 0) || (dir == NULL)) {
        bk_printf("tkl_dir_open %s failed: %d\r\n", path, ret);
        return;
    }

    bk_printf("%s\r\n", path);
    while (tkl_dir_read(dir, &info) == 0) {
        if (tkl_dir_name(info, &name) != 0) {
            break;
        }
        if ((name == NULL) || (name[0] == '\0')) {
            break;
        }
        if ((os_strcmp(name, ".") == 0) || (os_strcmp(name, "..") == 0)) {
            continue;
        }

        int n = snprintf(child, sizeof(child), "%s/%s", path, name);
        if ((n < 0) || ((uint32_t)n >= sizeof(child))) {
            bk_printf("path too long, skip: %s/%s\r\n", path, name);
            continue;
        }

        is_dir = FALSE;
        (void)tkl_dir_is_directory(info, &is_dir);
        if (is_dir == TRUE) {
            __fs_op_list(child);
        } else {
            bk_printf("%s\r\n", child);
        }
    }
    tkl_dir_close(dir);
}

/**
 * @brief Mount sub-cmd handler: xfs mount <dev>
 * @param[in] argc argument count
 * @param[in] argv argument vector
 * @return none
 * @note Uses test_fs_mount() so that mount/unmount operations are
 *       reference-counted across CLI and other test modules. The ref count
 *       is only bumped on the first successful hardware mount, which means
 *       repeated CLI 'mount <dev>' calls will hit the "already mounted"
 *       guard below without a double mount on hardware.
 *       For DEV_USB_DISK, the very first mount may fail because USB
 *       enumeration is still in progress; a single short-delay retry is
 *       used to absorb that race.
 */
static void __fs_cmd_mount(int argc, char **argv)
{
    int idx = __fs_dev_index((argc >= 3) ? argv[2] : NULL);
    if (idx < 0) {
        bk_printf("missing or unknown <dev>, expect: sdcard | udisk | littlefs\r\n");
        return;
    }
    if (s_fs_mounted[idx] == TRUE) {
        bk_printf("%s already mounted on %s\r\n", s_fs_devs[idx].name, s_fs_devs[idx].path);
        return;
    }

    int ret = test_fs_mount(s_fs_devs[idx].path, s_fs_devs[idx].dev_type);
    if ((ret != 0) && (s_fs_devs[idx].dev_type == DEV_USB_DISK)) {
        bk_printf("udisk first mount failed (%d), retry after %d ms\r\n",
                  ret, TEST_FS_UDISK_RETRY_MS);
        tkl_system_sleep(TEST_FS_UDISK_RETRY_MS);
        ret = test_fs_mount(s_fs_devs[idx].path, s_fs_devs[idx].dev_type);
    }
    if (ret != 0) {
        bk_printf("mount %s on %s failed: %d\r\n",
                  s_fs_devs[idx].name, s_fs_devs[idx].path, ret);
        return;
    }

    s_fs_mounted[idx] = TRUE;
    bk_printf("mount %s on %s ok\r\n", s_fs_devs[idx].name, s_fs_devs[idx].path);
}

/**
 * @brief Umount sub-cmd handler: xfs umount <dev>
 * @param[in] argc argument count
 * @param[in] argv argument vector
 * @return none
 * @note Uses test_fs_unmount() which is ref-counted, so the underlying
 *       hardware unmount only runs when the last user releases the FS.
 *       Mount state for the given device is always cleared so the user can
 *       re-mount via CLI even when the underlying unmount returns a non-zero
 *       status (e.g. resource already released by lower layer).
 */
static void __fs_cmd_umount(int argc, char **argv)
{
    int idx = __fs_resolve_dev(argc, argv, TRUE);
    if (idx < 0) {
        return;
    }

    int ret = test_fs_unmount(s_fs_devs[idx].path);
    bk_printf("umount %s ret: %d\r\n", s_fs_devs[idx].path, ret);
    s_fs_mounted[idx] = FALSE;
}

/**
 * @brief Ls sub-cmd handler: xfs ls [dev] [sub-path]
 * @param[in] argc argument count
 * @param[in] argv argument vector
 * @return none
 * @note When <dev> is omitted, every currently mounted device is listed;
 *       when present, the listing is restricted to that device.
 */
static void __fs_cmd_ls(int argc, char **argv)
{
    if ((argc >= 3) && (argv[2] != NULL)) {
        int idx = __fs_resolve_dev(argc, argv, TRUE);
        if (idx < 0) {
            return;
        }
        char fp[TEST_FS_PATH_MAX] = {0};
        const char *sub = ((argc >= 4) && (argv[3] != NULL)) ? argv[3] : NULL;
        if (__fs_build_path(fp, sizeof(fp), idx, sub) != 0) {
            return;
        }
        __fs_op_list(fp);
        return;
    }

    BOOL_T any = FALSE;
    for (uint32_t i = 0; i < TEST_FS_DEV_NUM; i++) {
        if (s_fs_mounted[i] == TRUE) {
            any = TRUE;
            __fs_op_list(s_fs_devs[i].path);
        }
    }
    if (any == FALSE) {
        bk_printf("no fs mounted\r\n");
    }
}

/**
 * @brief Read sub-cmd handler: xfs read <dev> <file> [len]
 * @param[in] argc argument count
 * @param[in] argv argument vector
 * @return none
 */
static void __fs_cmd_read(int argc, char **argv)
{
    int idx = __fs_resolve_dev(argc, argv, TRUE);
    if (idx < 0) {
        return;
    }
    if ((argc < 4) || (argv[3] == NULL)) {
        bk_printf("missing <file>, usage: xfs read <dev> <file> [len]\r\n");
        return;
    }

    uint32_t len = TEST_FS_READ_LEN_DEF;
    if ((argc >= 5) && (argv[4] != NULL)) {
        len = os_strtoul(argv[4], NULL, 10);
    }
    if ((len == 0) || (len > TEST_FS_READ_LEN_MAX)) {
        bk_printf("read len invalid: %u\r\n", (unsigned)len);
        return;
    }

    char fp[TEST_FS_PATH_MAX] = {0};
    if (__fs_build_path(fp, sizeof(fp), idx, argv[3]) != 0) {
        return;
    }

    TUYA_FILE file = tkl_fopen(fp, "rb");
    if (file == NULL) {
        bk_printf("[%s][%d] tkl_fopen %s fail\r\n", __FUNCTION__, __LINE__, fp);
        return;
    }

    uint8_t *buf = (uint8_t *)tkl_system_malloc(len);
    if (buf == NULL) {
        bk_printf("malloc %u fail\r\n", (unsigned)len);
        tkl_fclose(file);
        return;
    }
    memset(buf, 0, len);

    int rd = tkl_fread((VOID_T *)buf, (INT_T)len, file);
    tkl_fclose(file);
    bk_printf("read %s, request: %u, actual: %d\r\n", fp, (unsigned)len, rd);

    if (rd > 0) {
        uint32_t debug_len = ((uint32_t)rd < TEST_FS_READ_DUMP_MAX) ? (uint32_t)rd : TEST_FS_READ_DUMP_MAX;
        bk_printf("read data <display %u bytes at most>:\r\n", (unsigned)TEST_FS_READ_DUMP_MAX);
        for (uint32_t i = 0; i < debug_len; i++) {
            if ((i % 16) == 0 && i != 0) {
                bk_printf("\r\n");
            }
            bk_printf("%02x ", buf[i]);
        }
        bk_printf("\r\n");
    }

    tkl_system_free(buf);
}

/**
 * @brief Write sub-cmd handler: xfs write <dev> <file> <data>
 * @param[in] argc argument count
 * @param[in] argv argument vector
 * @return none
 */
static void __fs_cmd_write(int argc, char **argv)
{
    int idx = __fs_resolve_dev(argc, argv, TRUE);
    if (idx < 0) {
        return;
    }
    if ((argc < 5) || (argv[3] == NULL) || (argv[4] == NULL)) {
        bk_printf("missing <file> or <data>, usage: xfs write <dev> <file> <data>\r\n");
        return;
    }

    uint32_t size = strlen(argv[4]);
    if (size == 0) {
        bk_printf("data len is zero\r\n");
        return;
    }

    char fp[TEST_FS_PATH_MAX] = {0};
    if (__fs_build_path(fp, sizeof(fp), idx, argv[3]) != 0) {
        return;
    }

    TUYA_FILE file = tkl_fopen(fp, "wb");
    if (file == NULL) {
        bk_printf("[%s][%d] tkl_fopen %s fail\r\n", __FUNCTION__, __LINE__, fp);
        return;
    }

    int wr = tkl_fwrite((VOID_T *)argv[4], (INT_T)size, file);
    tkl_fclose(file);
    bk_printf("write %s, request: %u, actual: %d\r\n", fp, (unsigned)size, wr);
}

/**
 * @brief Mkdir sub-cmd handler: xfs mkdir <dev> <dir-path>
 * @param[in] argc argument count
 * @param[in] argv argument vector
 * @return none
 */
static void __fs_cmd_mkdir(int argc, char **argv)
{
    int idx = __fs_resolve_dev(argc, argv, TRUE);
    if (idx < 0) {
        return;
    }
    if ((argc < 4) || (argv[3] == NULL)) {
        bk_printf("missing <dir-path>, usage: xfs mkdir <dev> <dir-path>\r\n");
        return;
    }

    char fp[TEST_FS_PATH_MAX] = {0};
    if (__fs_build_path(fp, sizeof(fp), idx, argv[3]) != 0) {
        return;
    }

    int ret = tkl_fs_mkdir(fp);
    if (ret != 0) {
        bk_printf("tkl_fs_mkdir %s fail: %d\r\n", fp, ret);
        return;
    }
    bk_printf("mkdir %s ok\r\n", fp);
}

/**
 * @brief Unlink sub-cmd handler: xfs rm|unlink <dev> <file>
 * @param[in] argc argument count
 * @param[in] argv argument vector
 * @return none
 */
static void __fs_cmd_unlink(int argc, char **argv)
{
    int idx = __fs_resolve_dev(argc, argv, TRUE);
    if (idx < 0) {
        return;
    }
    if ((argc < 4) || (argv[3] == NULL)) {
        bk_printf("missing <file>, usage: xfs rm|unlink <dev> <file>\r\n");
        return;
    }

    char fp[TEST_FS_PATH_MAX] = {0};
    if (__fs_build_path(fp, sizeof(fp), idx, argv[3]) != 0) {
        return;
    }

    int ret = tkl_fs_remove(fp);
    bk_printf("tkl_fs_remove %s ret: %d\r\n", fp, ret);
}

/**
 * @brief Mkfs sub-cmd handler: xfs mkfs littlefs
 * @param[in] argc argument count
 * @param[in] argv argument vector
 * @return none
 * @note Only DEV_EXT_FLASH (littlefs on QSPI flash) is supported.
 *       The target device must be unmounted before formatting; the chip
 *       contents will be erased before the new littlefs image is laid down.
 */
static void __fs_cmd_mkfs(int argc, char **argv)
{
    int idx = __fs_resolve_dev(argc, argv, FALSE);
    if (idx < 0) {
        return;
    }
    if (s_fs_devs[idx].dev_type != DEV_EXT_FLASH) {
        bk_printf("mkfs only supports 'littlefs', got: %s\r\n", s_fs_devs[idx].name);
        return;
    }
    if (s_fs_mounted[idx] == TRUE) {
        bk_printf("%s is mounted, please umount first\r\n", s_fs_devs[idx].name);
        return;
    }

#if CONFIG_QSPI
    struct bk_little_fs_partition partition;

#ifdef CONFIG_TUYA_USE_MTD
    extern MTD_DEVICE_T *tuya_mtd_device_query(const char *name);
    MTD_DEVICE_T *mtd_dev = tuya_mtd_device_query(CONFIG_TUYA_QSPI_FLASH_TYPE);
    if (mtd_dev == NULL) {
        bk_printf("mtd device %s not found\r\n", CONFIG_TUYA_QSPI_FLASH_TYPE);
        return;
    }
    bk_printf("mkfs on mtd %s\r\n", CONFIG_TUYA_QSPI_FLASH_TYPE);
    partition.part_flash.start_addr = TEST_FS_LFS_QSPI_ADDR;
    partition.part_flash.size       = mtd_dev->nand_dev.total_size;
    partition.part_flash.page_size  = mtd_dev->nand_dev.page_size;
    partition.part_flash.block_size = mtd_dev->nand_dev.block_size;
#else
    qspi_driver_desc_t *qflash_dev = tuya_qspi_device_query(CONFIG_TUYA_QSPI_FLASH_TYPE);
    if (qflash_dev == NULL) {
        bk_printf("qspi flash %s not found\r\n", CONFIG_TUYA_QSPI_FLASH_TYPE);
        return;
    }

    static int s_qspi_inited = 0;
    if (s_qspi_inited == 0) {
        qflash_init();
        s_qspi_inited = 1;
    }
    bk_printf("full chip erase %s\r\n", CONFIG_TUYA_QSPI_FLASH_TYPE);
    qflash_erase(0, qflash_dev->total_size);

    bk_printf("mkfs on qspi flash, total: %d, block: %d\r\n",
              qflash_dev->total_size, qflash_dev->block_size);
    partition.part_flash.start_addr = TEST_FS_LFS_QSPI_ADDR;
    partition.part_flash.size       = qflash_dev->total_size;
    partition.part_flash.page_size  = qflash_dev->page_size;
    partition.part_flash.block_size = qflash_dev->block_size;
#endif /* CONFIG_TUYA_USE_MTD */

    partition.part_type  = LFS_QSPI_FLASH;
    partition.mount_path = s_fs_devs[idx].path;

    int ret = mkfs("PART_NONE", TEST_FS_LFS_FS_NAME, &partition);
    bk_printf("mkfs %s ret: %d\r\n", s_fs_devs[idx].path, ret);
#else
    bk_printf("mkfs disabled: CONFIG_QSPI is not enabled\r\n");
#endif /* CONFIG_QSPI */
}

/**
 * @brief Write_large_file_test sub-cmd handler: xfs wt <dev> [len_kb]
 * @param[in] argc argument count
 * @param[in] argv argument vector
 * @return none
 * @note Allocates a PSRAM buffer of <len_kb> KB (default 500 KB), fills it
 *       with a deterministic byte pattern and writes it as a single file
 *       to the specified mounted filesystem. The write throughput is
 *       reported, then the data is read back and verified, and finally
 *       the test file is removed. Designed to mirror the 'wt' command
 *       in test_littlefs.c, but works against any mounted FS.
 */
static void __fs_cmd_wt(int argc, char **argv)
{
    int idx = __fs_resolve_dev(argc, argv, TRUE);
    if (idx < 0) {
        return;
    }

    if (s_fs_wt_in_progress != 0) {
        bk_printf("last wt in progress, wait and retry\r\n");
        return;
    }

    uint32_t kb = TEST_FS_WT_DEFAULT_KB;
    if ((argc >= 4) && (argv[3] != NULL)) {
        kb = (uint32_t)os_strtoul(argv[3], NULL, 10);
    }
    if ((kb == 0) || (kb > TEST_FS_WT_MAX_KB)) {
        bk_printf("len_kb out of range: %u (1..%u)\r\n", kb, TEST_FS_WT_MAX_KB);
        return;
    }

    uint32_t total = kb << 10; /* KB -> bytes */
    bk_printf("wt: dev=%s, len=%u KB (%u bytes)\r\n", s_fs_devs[idx].name, kb, total);

    s_fs_wt_in_progress = 1;

    uint8_t *wbuf = (uint8_t *)tkl_system_psram_malloc((SIZE_T)total);
    uint8_t *rbuf = (uint8_t *)tkl_system_psram_malloc((SIZE_T)total);
    if ((wbuf == NULL) || (rbuf == NULL)) {
        bk_printf("psram alloc failed (wbuf=%p, rbuf=%p)\r\n", wbuf, rbuf);
        if (wbuf != NULL) {
            tkl_system_psram_free(wbuf);
        }
        if (rbuf != NULL) {
            tkl_system_psram_free(rbuf);
        }
        s_fs_wt_in_progress = 0;
        return;
    }

    for (uint32_t i = 0; i < total; i++) {
        wbuf[i] = (uint8_t)i;
    }
    memset(rbuf, 0x5A, total);

    char fp[TEST_FS_PATH_MAX] = {0};
    if (__fs_build_path(fp, sizeof(fp), idx, TEST_FS_WT_FILE_NAME) != 0) {
        tkl_system_psram_free(wbuf);
        tkl_system_psram_free(rbuf);
        s_fs_wt_in_progress = 0;
        return;
    }

    /* ---- write phase ---- */
    bk_printf("====== write %s ======\r\n", fp);
    TUYA_FILE wf = tkl_fopen(fp, "wb");
    if (wf == NULL) {
        bk_printf("tkl_fopen %s for write failed\r\n", fp);
        tkl_system_psram_free(wbuf);
        tkl_system_psram_free(rbuf);
        s_fs_wt_in_progress = 0;
        return;
    }

    SYS_TIME_T t0 = tkl_system_get_millisecond();
    uint32_t off = 0;
    int werr = 0;
    while (off < total) {
        uint32_t chunk = total - off;
        if (chunk > TEST_FS_WT_CHUNK_SZ) {
            chunk = TEST_FS_WT_CHUNK_SZ;
        }
        int wr = tkl_fwrite((VOID_T *)(wbuf + off), (INT_T)chunk, wf);
        if (wr <= 0) {
            bk_printf("tkl_fwrite at off=%u, ret=%d\r\n", off, wr);
            werr = -1;
            break;
        }
        off += (uint32_t)wr;
    }
    tkl_fclose(wf);
    SYS_TIME_T t1 = tkl_system_get_millisecond();

    if (werr != 0) {
        bk_printf("write aborted, written=%u/%u\r\n", off, total);
        (void)tkl_fs_remove(fp);
        tkl_system_psram_free(wbuf);
        tkl_system_psram_free(rbuf);
        s_fs_wt_in_progress = 0;
        return;
    }

    uint64_t dt_ms = (uint64_t)(t1 - t0);
    if (dt_ms == 0) {
        dt_ms = 1; /* avoid divide-by-zero, sub-ms write is plausible only for tiny size */
    }
    /* throughput in KB/s = (total/1024) * 1000 / dt_ms */
    uint32_t kbps = (uint32_t)(((uint64_t)total * 1000ULL) / (1024ULL * dt_ms));
    bk_printf("write %u bytes ok, time=%llu ms, throughput=%u KB/s\r\n", total, dt_ms, kbps);

    /* ---- read phase ---- */
    bk_printf("====== read %s ======\r\n", fp);
    TUYA_FILE rf = tkl_fopen(fp, "rb");
    if (rf == NULL) {
        bk_printf("tkl_fopen %s for read failed\r\n", fp);
        (void)tkl_fs_remove(fp);
        tkl_system_psram_free(wbuf);
        tkl_system_psram_free(rbuf);
        s_fs_wt_in_progress = 0;
        return;
    }

    SYS_TIME_T t2 = tkl_system_get_millisecond();
    off = 0;
    int rerr = 0;
    while (off < total) {
        uint32_t chunk = total - off;
        if (chunk > TEST_FS_WT_CHUNK_SZ) {
            chunk = TEST_FS_WT_CHUNK_SZ;
        }
        int rd = tkl_fread((VOID_T *)(rbuf + off), (INT_T)chunk, rf);
        if (rd <= 0) {
            bk_printf("tkl_fread at off=%u, ret=%d\r\n", off, rd);
            rerr = -1;
            break;
        }
        off += (uint32_t)rd;
    }
    tkl_fclose(rf);
    SYS_TIME_T t3 = tkl_system_get_millisecond();

    if (rerr == 0) {
        uint64_t dr_ms = (uint64_t)(t3 - t2);
        if (dr_ms == 0) {
            dr_ms = 1;
        }
        uint32_t rkbps = (uint32_t)(((uint64_t)total * 1000ULL) / (1024ULL * dr_ms));
        bk_printf("read  %u bytes ok, time=%llu ms, throughput=%u KB/s\r\n",
                  off, dr_ms, rkbps);

        /* ---- verify ---- */
        uint32_t mism = 0;
        for (uint32_t i = 0; i < total; i++) {
            if (wbuf[i] != rbuf[i]) {
                if (mism < 4) {
                    bk_printf("mismatch @%u: w=%02x r=%02x\r\n", i, wbuf[i], rbuf[i]);
                }
                mism++;
            }
        }
        if (mism == 0) {
            bk_printf("verify: OK (%u bytes)\r\n", total);
        } else {
            bk_printf("verify: FAILED, %u byte(s) differ\r\n", mism);
        }
    } else {
        bk_printf("read aborted, read=%u/%u\r\n", off, total);
    }

    /* ---- cleanup ---- */
    int rm = tkl_fs_remove(fp);
    bk_printf("cleanup remove %s ret: %d\r\n", fp, rm);

    tkl_system_psram_free(wbuf);
    tkl_system_psram_free(rbuf);
    s_fs_wt_in_progress = 0;
}

/* ---------------------------------------------------------------------------
 * Sub-command table (table-driven dispatch)
 * --------------------------------------------------------------------------- */
static const FS_SUB_CMD_T s_fs_sub_cmds[] = {
    {"mount",  __fs_cmd_mount,  3, "mount <dev>"                  },
    {"umount", __fs_cmd_umount, 3, "umount <dev>"                 },
    {"mkfs",   __fs_cmd_mkfs,   3, "mkfs littlefs"                },
    {"ls",     __fs_cmd_ls,     2, "ls [dev] [sub-path]"          },
    {"read",   __fs_cmd_read,   4, "read <dev> <file> [len]"      },
    {"write",  __fs_cmd_write,  5, "write <dev> <file> <data>"    },
    {"mkdir",  __fs_cmd_mkdir,  4, "mkdir <dev> <dir-path>"       },
    {"rm",     __fs_cmd_unlink, 4, "rm <dev> <file>"              },
    {"unlink", __fs_cmd_unlink, 4, "unlink <dev> <file>"          },
    {"wt",     __fs_cmd_wt,     3, "wt <dev> [len_kb] (default 500)"},
};
#define TEST_FS_SUB_CMD_NUM (sizeof(s_fs_sub_cmds) / sizeof(s_fs_sub_cmds[0]))

/**
 * @brief Print the usage of xfs cli command, driven by the sub-cmd table
 * @return none
 */
static void __fs_cmd_usage(void)
{
    bk_printf("usage:\r\n");
    for (uint32_t i = 0; i < TEST_FS_SUB_CMD_NUM; i++) {
        bk_printf("  xfs %s\r\n", s_fs_sub_cmds[i].usage);
    }
    bk_printf("dev: sdcard | udisk | littlefs\r\n");
}

/**
 * @brief Generic filesystem cli command, dispatch to handlers by table lookup
 * @param[in] pcWriteBuffer cli output buffer (unused)
 * @param[in] xWriteBufferLen cli output buffer length (unused)
 * @param[in] argc argument count
 * @param[in] argv argument vector
 * @return none
 */
void cli_fs_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    (void)pcWriteBuffer;
    (void)xWriteBufferLen;

    if (argc < 2) {
        bk_printf("no parameter\r\n");
        __fs_cmd_usage();
        return;
    }

    bk_printf("argc: %d\r\n cmd:", argc);
    for (int i = 0; i < argc; i++) {
        bk_printf(" %s", argv[i]);
    }
    bk_printf("\r\n");

    for (uint32_t i = 0; i < TEST_FS_SUB_CMD_NUM; i++) {
        const FS_SUB_CMD_T *e = &s_fs_sub_cmds[i];
        if (os_strcmp(argv[1], e->name) != 0) {
            continue;
        }
        if (argc < e->min_argc) {
            bk_printf("'%s' need more args, usage: xfs %s\r\n", e->name, e->usage);
            return;
        }
        e->handler(argc, argv);
        return;
    }

    bk_printf("unknown sub cmd: %s\r\n", argv[1]);
    __fs_cmd_usage();
}

