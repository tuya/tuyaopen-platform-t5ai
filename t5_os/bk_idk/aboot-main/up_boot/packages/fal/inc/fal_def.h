/*
 * File      : fal_def.h
 * This file is part of FAL (Flash Abstraction Layer) package
 * COPYRIGHT (C) 2006 - 2019, RT-Thread Development Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-05-17     armink       the first version
 */

#ifndef _FAL_DEF_H_
#define _FAL_DEF_H_

#include <stdint.h>
#include <stdio.h>
#include "drv_uart.h"

#define FAL_SW_VERSION                 "0.4.0"

#ifndef FAL_MALLOC
#define FAL_MALLOC                     malloc
#endif

#ifndef FAL_CALLOC
#define FAL_CALLOC                     calloc
#endif

#ifndef FAL_REALLOC
#define FAL_REALLOC                    realloc
#endif

#ifndef FAL_FREE
#define FAL_FREE                       free
#endif

#ifndef FAL_DEBUG
#define FAL_DEBUG                      1
#endif

#if FAL_DEBUG
#ifdef assert
#undef assert
#endif
#define assert(EXPR)                                                           \
if (!(EXPR))                                                                   \
{                                                                              \
    bk_printf("(%s) assert at %s.\n", #EXPR, __FUNCTION__);            \
    while (1);                                                                 \
}

#define LOG_TAG "[FAL]"
/* debug level log */
#ifdef  log_d
#undef  log_d
#endif

#define log_d(format, ...)                     bk_printf//bk_printf(format, ##__VA_ARGS__);

#else

#ifdef assert
#undef assert
#endif
#define assert(EXPR)                   ((void)0);

/* debug level log */
#ifdef  log_d
#undef  log_d
#endif
#define log_d(...)
#endif /* FAL_DEBUG */

/* error level log */
#ifdef  log_e
#undef  log_e
#endif
#define log_e(format,...)                      bk_printf//("[FAL] (%s:%d) ", __FUNCTION__, __LINE__);bk_printf(__VA_ARGS__);printf("\n")

/* info level log */
#ifdef  log_i
#undef  log_i
#endif
#define log_i(format,...)                      bk_printf//("[FAL] ");bk_printf(__VA_ARGS__);printf("\n")


/* FAL flash and partition device name max length */
#ifndef FAL_DEV_NAME_MAX
#define FAL_DEV_NAME_MAX 24
#endif

struct fal_flash_dev
{
    char name[FAL_DEV_NAME_MAX];

    /* flash device start address and len  */
    uint32_t addr;
    size_t len;
    /* the block size in the flash for erase minimum granularity */
    size_t blk_size;

    struct
    {
        int (*init)(void);
        int (*read)(long offset, uint8_t *buf, size_t size);
        int (*write)(long offset, const uint8_t *buf, size_t size);
        int (*erase)(long offset, size_t size);
    } ops;
};
typedef struct fal_flash_dev *fal_flash_dev_t;

/**
 * FAL partition
 */
struct fal_partition
{
    uint32_t magic_word;

    /* partition name */
    char name[FAL_DEV_NAME_MAX];
    /* flash device name for partition */
    char flash_name[FAL_DEV_NAME_MAX];

    /* partition offset address on flash device */
    long offset;
    size_t len;

    uint32_t reserved;
};
typedef struct fal_partition *fal_partition_t;


/**
 * OTA firmware encryption algorithm and compression algorithm
 */
enum ota_algo
{
    OTA_CRYPT_ALGO_NONE    = 0L,               /**< no encryption algorithm and no compression algorithm */
    OTA_CRYPT_ALGO_XOR     = 1L,               /**< XOR encryption */
    OTA_CRYPT_ALGO_AES256  = 2L,               /**< AES256 encryption */
    OTA_CMPRS_ALGO_GZIP    = 1L << 8,          /**< Gzip: zh.wikipedia.org/wiki/Gzip */
    OTA_CMPRS_ALGO_QUICKLZ = 2L << 8,          /**< QuickLZ: www.quicklz.com */
};
typedef enum ota_algo ota_algo_t;
/**
 * OTA '.rbl' file header, also as firmware information header
 */
struct ota_rbl_hdr
{
    char magic[4];

    ota_algo_t algo;
    uint32_t timestamp;
    char name[16];
    char version[24];

    char sn[24];

    /* crc32(aes(zip(rtt.bin))) */
    uint32_t crc32;
    /* hash(rtt.bin) */
    uint32_t hash;

    /* len(rtt.bin) */
    uint32_t size_raw;
    /* len(aes(zip(rtt.bin))) */
    uint32_t size_package;

    /* crc32(rbl_hdr - info_crc32) */
    uint32_t info_crc32;
};
typedef struct ota_rbl_hdr *ota_rbl_hdr_p;

#endif /* _FAL_DEF_H_ */
