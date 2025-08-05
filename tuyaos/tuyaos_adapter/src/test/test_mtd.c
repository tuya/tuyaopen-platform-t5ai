/*
 * test_qspi.c
 * Copyright (C) 2024 cc <cc@tuya>
 *
 * Distributed under terms of the MIT license.
 */

#include "stdint.h"
#include "tuya_cloud_types.h"

#include "cli.h"
#include <driver/int_types.h>
#include "cli_tuya_test.h"
#include "tal_mtd_service.h"

static volatile int __qspi_has_inited = 0;
extern MTD_DEVICE_T w25q32flash_cfg;
extern MTD_DEVICE_T gd25q127cflash_cfg;
extern MTD_DEVICE_T gd25q32flash_cfg;
extern MTD_DEVICE_T gd5f1g_flash_cfg;

void cli_xmtd_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    int i;
    uint32_t c = 0x1;
    uint32_t test_len = 200;
    uint32_t addr = 0x21f000;

    MTD_CFG_T cfg = {
        .nor_cfg = {
            .cfg_qspi = {
                .port = TUYA_QSPI_NUM_1,
            }
        }
    };
    MTD_HANDLE handle = NULL;

    if (__qspi_has_inited == 0) {
        handle = tal_mtd_init(&gd5f1g_flash_cfg, &cfg);
        __qspi_has_inited = 1;
    }

    test_len <<= 10;
    // test_len += tal_mtd_dev->block_size;
    // test_len /= tal_mtd_dev->block_size;
    // test_len *= tal_mtd_dev->block_size;

    bk_printf("\r\n------- qspi test init ------\r\n");

    bk_printf("\r\n--- %d\r\n", test_len);
    uint8_t *write_buffer = (uint8_t *)psram_malloc(test_len);
    if (write_buffer == NULL) {
        bk_printf("------- qspi test 3 failed, malloc write buffer error ------\r\n");
        return;
    }

    uint8_t *read_buffer = (uint8_t *)psram_malloc(test_len);
    if (read_buffer == NULL) {
        bk_printf("------- qspi test 3 failed, malloc read buffer error ------\r\n");
        tkl_system_free(write_buffer);
        write_buffer = NULL;
        return;
    }

    bk_printf("\r\n------- qspi test read id ------\r\n");

    uint32_t v = 0;
    tal_mtd_get_id(handle, &v);
    bk_printf("flash id: 0x%03x\r\n", v & 0xFFFFFF);

#if 1

    bk_printf("\r\n------- qspi test erase ------\r\n");
    tal_mtd_erase(handle, 0, gd5f1g_flash_cfg.nor_dev.total_size);
    bk_printf("------- qspi large test ------\r\n");

    memset(read_buffer, 0x5a, test_len);
    tal_mtd_read(handle, addr, read_buffer, test_len);
    bk_printf("!!!!!!! val is  %x !!!!!!\r\n", 0,  read_buffer[0]);

    bk_printf("test write length: %d / %dKB\r\n", test_len, test_len >> 10);
    for (int i = 0; i < test_len; i++) {
        write_buffer[i] = c + (i / 0xff) & 0xff;
    }

    tal_mtd_write(handle, addr, write_buffer, test_len);

    // tkl_system_sleep(10);

    bk_printf("------- read ------\r\n");

    // bk_qspi_read(id, data, size);
    memset(read_buffer, 0x5a, test_len);
    tal_mtd_read(handle, addr, read_buffer, test_len);

    bk_printf("------- data check ------\r\n");
    for (i = 0; i < test_len; i++) {
        if (write_buffer[i] != read_buffer[i]) {
            // bk_printf("!!!!!!! error  %d %02x %02x !!!!!!\r\n", i, write_buffer[i], read_buffer[i]);
            bk_printf("!!!!!!! error  %d %x %x !!!!!!\r\n", i, write_buffer[i], read_buffer[i]);
            // break;
        }
    }

#endif

    bk_printf("------- qspi test end ------\r\n");

    psram_free(write_buffer);
    write_buffer = NULL;

    psram_free(read_buffer);
    read_buffer = NULL;

    return;
}

