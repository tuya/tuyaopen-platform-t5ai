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
#include <driver/qspi.h>
#include <driver/qspi_flash.h>
#include "qspi_hal.h"
#include <driver/qspi.h>
#include <driver/qspi_flash_common.h>

static volatile int __qspi_has_inited = 0;

void cli_xqspi_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    int i;
    uint32_t c = 0x1;
    uint32_t test_len = 200;
    uint32_t addr = 0x21f000;

    bk_printf("argc: %d\r\n", argc);
    for (int i = 0; i < argc; i++) {
        bk_printf("argv[%d]: %s\r\n", i, argv[i]);
    }

    qspi_driver_desc_t *qflash_dev = NULL;
    qflash_dev = tuya_qspi_device_query(CONFIG_TUYA_QSPI_FLASH_TYPE);
    if (qflash_dev == NULL) {
        bk_printf("Not found qspi flash %s\r\n", CONFIG_TUYA_QSPI_FLASH_TYPE);
        return BK_FAIL;
    }

    if (__qspi_has_inited == 0) {
        qflash_init();
        __qspi_has_inited = 1;
    }

    if (argc >= 2) {
        if (!strcmp("aa", argv[1])) {
            c = os_strtoul(argv[2], NULL, 10);
        } else if (!strcmp("fce", argv[1])) {
            // full chip erase
            bk_printf("\r\nfull chip erase %s\r\n", CONFIG_TUYA_QSPI_FLASH_TYPE);
            qflash_erase(0, qflash_dev->total_size);
            return;
        } else {
            test_len = os_strtoul(argv[1], NULL, 10);
        }
    }

    test_len <<= 10;
    // test_len += qflash_dev->block_size;
    // test_len /= qflash_dev->block_size;
    // test_len *= qflash_dev->block_size;

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

    uint32_t v = qflash_dev->read_id();
    bk_printf("flash id: 0x%03x\r\n", v & 0xFFFFFF);


    bk_printf("------- qspi test erase ------\r\n");

    qflash_dev->unblock();
    qflash_erase(addr, test_len);

    bk_printf("------- qspi large test ------\r\n");

    bk_printf("test write length: %d / %dKB\r\n", test_len, test_len >> 10);
    for (int i = 0; i < test_len; i++) {
        write_buffer[i] = c + (i / 0xff) & 0xff;
    }

    qflash_write(addr, write_buffer, test_len);

    // tkl_system_sleep(10);

    bk_printf("------- read ------\r\n");

    // bk_qspi_read(id, data, size);
    memset(read_buffer, 0x5a, test_len);
    qflash_read(addr, read_buffer, test_len);

    bk_printf("------- data check ------\r\n");
    for (i = 0; i < test_len; i++) {
        if (write_buffer[i] != read_buffer[i]) {
            // bk_printf("!!!!!!! error  %d %02x %02x !!!!!!\r\n", i, write_buffer[i], read_buffer[i]);
            bk_printf("!!!!!!! error  %d %x %x !!!!!!\r\n", i, write_buffer[i], read_buffer[i]);
            break;
        }
    }


    bk_printf("------- qspi test end ------\r\n");

    psram_free(write_buffer);
    write_buffer = NULL;

    psram_free(read_buffer);
    read_buffer = NULL;

    return;
}



#define LCD_SPD2010_WRITE_COMMAND     0x02

UCHAR_T init_data1[] =
{
	0xa5,
	0x10,
	0x00,
	0x01,
	0x01,
	0x01,
	0x01,
	0x15,
	0x15,
	0x03,
	0xbb,
	0x14,
	0x13,
	0x3e,
	0x25,
	0x11,
	0x7c,
	0x56,
	0x2a,
	0x08,
	0x12,
	0x00,
	0x11,
	0x4b,
	0x7c,
	0x45,
	0x77,
	0x0a,
	0x2a,
	0x0a,
	0x1a,
	0x43,
	0x42,
	0x3c,
	0x64,
	0x41,
	0x3c,
	0x02,
	0x3c,
	0x1f,
	0x80,
	0x3f,
	0x21,
	0x07,
	0x0e,
	0x01,
	0x20,
	0x52,
	0x10,
	0x42,
	0x20,
	0x52,
	0x10,
	0x42,
	0x0a,
	0x32,
	0x14,
	0x06,
	0x00,
	0x06,
	0x00,
	0x08,
	0x08,
	0x0a,
	0x0a,
	0x09,
	0x09,
	0x38,
	0x2a,
	0x4a,
	0x40,
	0x39,
	0x39,
	0x37,
	0x37,
	0x28,
	0x28,
	0x0b,
	0x04,
	0x13,
	0x09,
	0x1b,
	0x11,
	0x11,
	0x0d,
	0x14,
	0x13,
	0x15,
	0x0e,
	0x10,
	0x0f,
	0x18,
	0x0e,
	0x07,
	0x05,
	0x11,
	0x0e,
	0x19,
	0x14,
	0x00,
	0x00,
};

STATIC TUYA_QSPI_CMD_T  init_cmd1[] =
{
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0xFF,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0xe7,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0x35,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0x3a,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0x40,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0x41,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0x55,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0x44,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0x45,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0x7d,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0xc1,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0xc2,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0xc3,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0xc6,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0xc7,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0xc8,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0x7a,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0x6f,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0x78,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0x73,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0x74,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0xc9,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0x67,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0x51,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0x52,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0x53,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0x54,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0x46,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0x47,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0x48,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0x49,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0x56,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0x57,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0x58,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0x59,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0x5A,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0x5B,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0x5c,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0x5d,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0x5e,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0x60,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0x61,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0x62,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0x63,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0x64,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0x65,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0xca,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0xcb,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0xcc,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0xcd,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0xd0,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0xd1,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0xd2,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0xd3,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0xd4,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0xd5,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0x6e,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0xe5,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0xe6,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0xf8,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0xf9,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0x80,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0xa0,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0x81,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0xa1,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0x82,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0xa2,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0x86,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0xa6,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0x87,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0xa7,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0x83,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0xa3,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0x84,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0xa4,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0x85,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0xa5,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0x88,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0xa8,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0x89,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0xa9,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0x8a,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0xaa,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0x8b,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0xab,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0x8c,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0xac,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0x8d,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0xad,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0x8e,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0xae,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0x8f,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0xaf,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0x90,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0xb0,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0x91,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0xb1,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0x92,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0xb2,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0xff,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 1, TUYA_QSPI_1WIRE,0x0},
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0x11,0x0},0x3,    TUYA_QSPI_1WIRE,         {NULL}, 0, TUYA_QSPI_1WIRE,0x0},
};


STATIC TUYA_QSPI_CMD_T  init_cmd2[] =
{
	{TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0x29,0x0},0x3, TUYA_QSPI_1WIRE,   {0x00}, 0, TUYA_QSPI_1WIRE,0x0},
};

STATIC TUYA_QSPI_CMD_T  init_cmd3[] =
{
	{TUYA_QSPI_WRITE, {0x32}, 1, TUYA_QSPI_1WIRE,  {0x00, 0x2c, 0x00},0x3, TUYA_QSPI_1WIRE,  {0x00}, 0, TUYA_QSPI_1WIRE,0x0},
};

#if 0
STATIC TUYA_QSPI_CMD_T  init_cmd30[] =
{
	{TUYA_QSPI_WRITE, {0x32}, 0, TUYA_QSPI_1WIRE,  {0x00, 0x2c, 0x00},0x0, TUYA_QSPI_1WIRE,  {0}, 2, TUYA_QSPI_4WIRE,0x0},
};


STATIC TUYA_QSPI_CMD_T  init_cmd31[] =
{
	{TUYA_QSPI_WRITE, {0x32}, 1, TUYA_QSPI_1WIRE,  {0x00, 0x2c, 0x00},0x3, TUYA_QSPI_4WIRE,  {0x00}, 0, TUYA_QSPI_1WIRE,0x0},
};

STATIC TUYA_QSPI_CMD_T  init_cmd32[] =
{
	{TUYA_QSPI_WRITE, {0x32}, 1, TUYA_QSPI_1WIRE,  {0x2c},0x1, TUYA_QSPI_1WIRE,  {0x00}, 0, TUYA_QSPI_1WIRE,0x0},
};

STATIC TUYA_QSPI_CMD_T  init_cmd33[] =
{
	{TUYA_QSPI_WRITE, {0x32}, 1, TUYA_QSPI_1WIRE,  {0x01, 0x2c},0x2, TUYA_QSPI_4WIRE,  {0x00}, 0, TUYA_QSPI_1WIRE,0x0},
};


STATIC TUYA_QSPI_CMD_T  init_cmd34[] =
{
	{TUYA_QSPI_WRITE, {0x32}, 1, TUYA_QSPI_1WIRE,  {0x01, 0x2,0x3,0x4},0x4, TUYA_QSPI_4WIRE,  {0x00}, 0, TUYA_QSPI_1WIRE,0x0},
};


STATIC TUYA_QSPI_CMD_T  init_cmd35[] =
{
	{TUYA_QSPI_WRITE, {0x32}, 1, TUYA_QSPI_1WIRE,  {0x0},0x0, TUYA_QSPI_4WIRE,  {0x0}, 0, TUYA_QSPI_1WIRE,0x0},
};

STATIC TUYA_QSPI_CMD_T  init_cmd36[] =
{
	{TUYA_QSPI_WRITE, {0x32,0x31}, 2, TUYA_QSPI_4WIRE,  {0x0},0x0, TUYA_QSPI_4WIRE,  {0x0}, 0, TUYA_QSPI_1WIRE,0x0},
};
#endif

#define LCD_QSPI_RESET_PIN      GPIO_40
bk_err_t lcd_qspi_hardware_reset(void)
{
    gpio_dev_unmap(LCD_QSPI_RESET_PIN);
    gpio_dev_map(LCD_QSPI_RESET_PIN, 0);
    bk_gpio_enable_pull(LCD_QSPI_RESET_PIN);
    bk_gpio_pull_up(LCD_QSPI_RESET_PIN);
    rtos_delay_milliseconds(10);
    bk_gpio_pull_down(LCD_QSPI_RESET_PIN);
    rtos_delay_milliseconds(10);
    bk_gpio_pull_up(LCD_QSPI_RESET_PIN);
    rtos_delay_milliseconds(120);

    return BK_OK;
}

#define LCD_QSPI_CO5300_RESET_PIN      GPIO_29
bk_err_t lcd_qspi_co5300_hardware_reset(void)
{
    gpio_dev_unmap(LCD_QSPI_CO5300_RESET_PIN);
    gpio_dev_map(LCD_QSPI_CO5300_RESET_PIN, 0);
    bk_gpio_enable_pull(LCD_QSPI_CO5300_RESET_PIN);
    bk_gpio_pull_up(LCD_QSPI_CO5300_RESET_PIN);
    rtos_delay_milliseconds(20);
    bk_gpio_pull_down(LCD_QSPI_CO5300_RESET_PIN);
    rtos_delay_milliseconds(200);
    bk_gpio_pull_up(LCD_QSPI_CO5300_RESET_PIN);
    rtos_delay_milliseconds(120);

    return BK_OK;
}


static beken_semaphore_t lcd_qspi_semaphore = NULL;
extern bk_err_t bk_lcd_qspi_quad_write_stops(qspi_id_t qspi_id);

VOID_T qspi_tx_done_cb(TUYA_QSPI_NUM_E port, TUYA_QSPI_IRQ_EVT_E event)
{
    if (lcd_qspi_semaphore)
        rtos_set_semaphore(&lcd_qspi_semaphore);
    bk_printf("----------qspi_tx_done_cb:%x,%x \r\n", port, event);
}


STATIC TUYA_QSPI_CMD_T  init_cmd_53001[] =
{
    {TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0x11,0x0},0x3, TUYA_QSPI_1WIRE,  {0x0}, 0, TUYA_QSPI_1WIRE,0x0},
};
//delay 120

STATIC UCHAR_T init_data_53002_data[] =
{
	0x20,
	0x10,
    0xa0,
    0x00,
    0x80,
    0x55,
    0x00,
    0x20,
    0xff,
    0xff,
};

STATIC TUYA_QSPI_CMD_T  init_cmd_53002[] =
{
    {TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0xfe,0x0},0x3, TUYA_QSPI_1WIRE,  {0x0}, 0, TUYA_QSPI_1WIRE,0x0},
    {TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0x19,0x0},0x3, TUYA_QSPI_1WIRE,  {0x0}, 0, TUYA_QSPI_1WIRE,0x0},
    {TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0x1c,0x0},0x3, TUYA_QSPI_1WIRE,  {0x1}, 1, TUYA_QSPI_1WIRE,0x0},
    {TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0xfe,0x0},0x3, TUYA_QSPI_1WIRE,  {0x1}, 1, TUYA_QSPI_1WIRE,0x0},

    {TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0xc4,0x0},0x3, TUYA_QSPI_1WIRE,  {0x0}, 1, TUYA_QSPI_1WIRE,0x0},
    {TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0x3a,0x0},0x3, TUYA_QSPI_1WIRE,  {0x0}, 1, TUYA_QSPI_1WIRE,0x0},
    {TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0x35,0x0},0x3, TUYA_QSPI_1WIRE,  {0x0}, 1, TUYA_QSPI_1WIRE,0x0},
    {TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0x53,0x0},0x3, TUYA_QSPI_1WIRE,  {0x0}, 1, TUYA_QSPI_1WIRE,0x0},

    {TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0x51,0x0},0x3, TUYA_QSPI_1WIRE,  {0x0}, 1, TUYA_QSPI_1WIRE,0x0},
    {TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0x63,0x0},0x3, TUYA_QSPI_1WIRE,  {0x0}, 1, TUYA_QSPI_1WIRE,0x0},
    
};

STATIC UCHAR_T init_data_53003_1_data[] =
{
	0x00,0x06,0x01,0xd7,
};

STATIC UCHAR_T init_data_53003_2_data[] =
{
    0x00,0x00,0x01,0xd1,
};

STATIC TUYA_QSPI_CMD_T  init_cmd_53003[] =
{
    {TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0x2a,0x0},0x3, TUYA_QSPI_1WIRE,  {0x0}, 4, TUYA_QSPI_1WIRE,0x0},
    {TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0x2b,0x0},0x3, TUYA_QSPI_1WIRE,  {0x0}, 4, TUYA_QSPI_1WIRE,0x0},
};

//delay 600


STATIC TUYA_QSPI_CMD_T  init_cmd_53004[] =
{
    {TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0x11,0x0},0x3, TUYA_QSPI_1WIRE,  {0x0}, 0, TUYA_QSPI_1WIRE,0x0},
};

//delay 600

STATIC TUYA_QSPI_CMD_T  init_cmd_53005[] =
{
    {TUYA_QSPI_WRITE, {LCD_SPD2010_WRITE_COMMAND}, 1, TUYA_QSPI_1WIRE,  {0x0,0x29,0x0},0x3, TUYA_QSPI_1WIRE,  {0x0}, 0, TUYA_QSPI_1WIRE,0x0},
};

#define BSP_LCD_H_RES              (466)
#define BSP_LCD_V_RES              (466)

void cli_xqspi_lcd_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    int i = 0;
    OPERATE_RET ret = 0;
    bk_printf("argc: %d\r\n", argc);
    for (int i = 0; i < argc; i++) {
        bk_printf("argv[%d]: %s\r\n", i, argv[i]);
    }

    TUYA_QSPI_BASE_CFG_T spi_cfg;

    spi_cfg.freq_hz = 30000000;
    spi_cfg.use_dma = true;
    spi_cfg.mode = TUYA_QSPI_MODE0;
    spi_cfg.role = TUYA_QSPI_ROLE_MASTER;
    spi_cfg.type = TUYA_QSPI_TYPE_LCD;
    spi_cfg.dma_data_lines = TUYA_QSPI_4WIRE;

    if (argc == 2) {
        if (os_strcmp(argv[1], "co5300") == 0) {

            if (lcd_qspi_semaphore == NULL) {
                ret = rtos_init_semaphore(&lcd_qspi_semaphore, 1);
                if (ret != kNoErr) {
                    bk_printf("lcd qspi semaphore init failed.\r\n");
                    return BK_FAIL;
                }
            }

            lcd_qspi_co5300_hardware_reset();
            tkl_qspi_init(TUYA_QSPI_NUM_0, &spi_cfg);
            tkl_qspi_irq_init(TUYA_QSPI_NUM_0, qspi_tx_done_cb);
            tkl_qspi_irq_enable(TUYA_QSPI_NUM_0);

            for (i = 0; i < sizeof(init_cmd_53002) / sizeof(TUYA_QSPI_CMD_T); i ++) {
                init_cmd_53002[i].data = &init_data_53002_data[i];
                tkl_qspi_comand(TUYA_QSPI_NUM_0, &init_cmd_53002[i]);
            }

            init_cmd_53003[0].data = &init_data_53003_1_data[0];
            init_cmd_53003[1].data = &init_data_53003_2_data[0];

            for (i = 0; i < sizeof(init_cmd_53003) / sizeof(TUYA_QSPI_CMD_T); i ++) {
                tkl_qspi_comand(TUYA_QSPI_NUM_0, &init_cmd_53003[i]);
            }
            tkl_system_sleep(600);
            
            tkl_qspi_comand(TUYA_QSPI_NUM_0, &init_cmd_53004[0]);
            tkl_system_sleep(600);
            tkl_qspi_comand(TUYA_QSPI_NUM_0, &init_cmd_53005[0]);
            tkl_qspi_force_cs_pin(TUYA_QSPI_NUM_0, TUYA_GPIO_LEVEL_HIGH);
            tkl_system_sleep(60);

            uint8_t *write_buffer = (uint8_t *)psram_malloc(BSP_LCD_H_RES * BSP_LCD_V_RES * 2);
            if (write_buffer == NULL) {
                bk_printf("------- qspi test 3 failed, malloc write buffer error ------\r\n");
                return;
            }
        
            for(i = 0; i < BSP_LCD_H_RES * BSP_LCD_V_RES; i ++) {
                write_buffer[2 * i] = 0xf8;
                write_buffer[2*i + 1] = 0x00;
            }

            tkl_qspi_force_cs_pin(TUYA_QSPI_NUM_0, TUYA_GPIO_LEVEL_LOW);
            tkl_qspi_comand(TUYA_QSPI_NUM_0, &init_cmd3[0]);
            tkl_qspi_send(TUYA_QSPI_NUM_0, write_buffer,  BSP_LCD_H_RES * BSP_LCD_V_RES * 2);

            ret = rtos_get_semaphore(&lcd_qspi_semaphore, 3000);
            if (ret != kNoErr) {
                bk_printf("ret = %d, lcd qspi get semaphore failed!\r\n", ret);
                return BK_FAIL;
            }
            tkl_system_sleep(600);
            tkl_qspi_force_cs_pin(TUYA_QSPI_NUM_0, TUYA_GPIO_LEVEL_HIGH);

            for(i = 0; i < BSP_LCD_H_RES * BSP_LCD_V_RES; i ++) {
                write_buffer[2 * i] = 0x00;
                write_buffer[2*i + 1] = 0x1f;
            }
            tkl_qspi_force_cs_pin(TUYA_QSPI_NUM_0, TUYA_GPIO_LEVEL_LOW);
            tkl_qspi_comand(TUYA_QSPI_NUM_0, &init_cmd3[0]);
            tkl_qspi_send(TUYA_QSPI_NUM_0, write_buffer,  BSP_LCD_H_RES * BSP_LCD_V_RES * 2);
            ret = rtos_get_semaphore(&lcd_qspi_semaphore, 3000);
            if (ret != kNoErr) {
                bk_printf("ret = %d, lcd qspi get semaphore failed!\r\n", ret);
                return BK_FAIL;
            }
            tkl_system_sleep(600);
            tkl_qspi_force_cs_pin(TUYA_QSPI_NUM_0, TUYA_GPIO_LEVEL_HIGH);

            for(i = 0; i < BSP_LCD_H_RES * BSP_LCD_V_RES; i ++) {
                write_buffer[2 * i] = 0x07;
                write_buffer[2*i + 1] = 0xe0;
            }
            tkl_qspi_force_cs_pin(TUYA_QSPI_NUM_0, TUYA_GPIO_LEVEL_LOW);
            tkl_qspi_comand(TUYA_QSPI_NUM_0, &init_cmd3[0]);
            tkl_qspi_send(TUYA_QSPI_NUM_0, write_buffer,  BSP_LCD_H_RES * BSP_LCD_V_RES * 2);
            ret = rtos_get_semaphore(&lcd_qspi_semaphore, 3000);
            if (ret != kNoErr) {
                bk_printf("ret = %d, lcd qspi get semaphore failed!\r\n", ret);
                return BK_FAIL;
            }
            tkl_system_sleep(600);
            tkl_qspi_force_cs_pin(TUYA_QSPI_NUM_0, TUYA_GPIO_LEVEL_HIGH);

            bk_printf("end close cs\r\n");
            psram_free(write_buffer);

            tkl_qspi_irq_disable(TUYA_QSPI_NUM_0);
            tkl_qspi_deinit(TUYA_QSPI_NUM_0);

            if (lcd_qspi_semaphore != NULL) {
                ret = rtos_deinit_semaphore(&lcd_qspi_semaphore);
                lcd_qspi_semaphore = NULL;
            }
            bk_printf("test rgb end:%d\r\n",ret);
            return;
        }else if(os_strcmp(argv[1], "spd2010") == 0){
            lcd_qspi_hardware_reset();

            tkl_qspi_init(TUYA_QSPI_NUM_0, &spi_cfg);
            tkl_qspi_irq_init(TUYA_QSPI_NUM_0, qspi_tx_done_cb);
            tkl_qspi_irq_enable(TUYA_QSPI_NUM_0);

            for (i = 0; i < sizeof(init_cmd1) / sizeof(TUYA_QSPI_CMD_T); i ++) {
                init_cmd1[i].data = &init_data1[i];
                tkl_qspi_comand(TUYA_QSPI_NUM_0, &init_cmd1[i]);
            }
            rtos_delay_milliseconds(120);
            tkl_qspi_comand(TUYA_QSPI_NUM_0, &init_cmd2[0]);
            rtos_delay_milliseconds(20);
            //up init

            uint8_t *write_buffer = (uint8_t *)psram_malloc(261120);
            if (write_buffer == NULL) {
                bk_printf("------- qspi test 3 failed, malloc write buffer error ------\r\n");
                return;
            }
        
            for(i = 0; i < 261120 / 2; i ++) {
                write_buffer[2 * i] = 0xf8;
                write_buffer[2*i + 1] = 0x00;
            }
            tkl_qspi_force_cs_pin(TUYA_QSPI_NUM_0, TUYA_GPIO_LEVEL_LOW);
            tkl_qspi_comand(TUYA_QSPI_NUM_0, &init_cmd3[0]);

            tkl_qspi_send(TUYA_QSPI_NUM_0, write_buffer,  261120);
            tkl_system_sleep(1000);
            tkl_qspi_force_cs_pin(TUYA_QSPI_NUM_0, TUYA_GPIO_LEVEL_HIGH);


            for(i = 0; i < 261120 / 2; i ++) {
                write_buffer[2 * i] = 0x00;
                write_buffer[2*i + 1] = 0x1f;
            }
            tkl_qspi_force_cs_pin(TUYA_QSPI_NUM_0, TUYA_GPIO_LEVEL_LOW);
            tkl_qspi_comand(TUYA_QSPI_NUM_0, &init_cmd3[0]);
            tkl_qspi_send(TUYA_QSPI_NUM_0, write_buffer,  261120);
            tkl_system_sleep(1000);
            tkl_qspi_force_cs_pin(TUYA_QSPI_NUM_0, TUYA_GPIO_LEVEL_HIGH);
        
            for(i = 0; i < 261120 / 2; i ++) {
                write_buffer[2 * i] = 0x07;
                write_buffer[2*i + 1] = 0xe0;
            }
        
        
            tkl_qspi_force_cs_pin(TUYA_QSPI_NUM_0, TUYA_GPIO_LEVEL_LOW);
            tkl_qspi_comand(TUYA_QSPI_NUM_0, &init_cmd3[0]);
            // uint8_t data[3] = {0x1,0x2,0x3};
            // init_cmd30[0].data = &data[0];
            // tkl_qspi_comand(TUYA_QSPI_NUM_0, &init_cmd30[0]);
            // tkl_qspi_comand(TUYA_QSPI_NUM_0, &init_cmd31[0]);
            // tkl_qspi_comand(TUYA_QSPI_NUM_0, &init_cmd32[0]);
            // tkl_qspi_comand(TUYA_QSPI_NUM_0, &init_cmd33[0]);
            // tkl_qspi_comand(TUYA_QSPI_NUM_0, &init_cmd34[0]);
            // tkl_qspi_comand(TUYA_QSPI_NUM_0, &init_cmd35[0]);
            // tkl_qspi_comand(TUYA_QSPI_NUM_0, &init_cmd36[0]);
        
            tkl_qspi_send(TUYA_QSPI_NUM_0, write_buffer,  261120);
            tkl_system_sleep(1000);
            bk_printf("close cs\r\n");
            // bk_lcd_qspi_quad_write_stops(0);
            tkl_qspi_force_cs_pin(TUYA_QSPI_NUM_0, TUYA_GPIO_LEVEL_HIGH);
            psram_free(write_buffer);

            tkl_qspi_irq_disable(TUYA_QSPI_NUM_0);
            ret = tkl_qspi_deinit(TUYA_QSPI_NUM_0);
            bk_printf("deinit ret:%d ,ram:%d, psram:%d \r\n", ret, tkl_system_get_free_heap_size(), tkl_system_psram_get_free_heap_size());
            return;
        }
    }
}
