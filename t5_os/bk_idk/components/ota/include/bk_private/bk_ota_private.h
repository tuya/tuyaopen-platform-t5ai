// Copyright 2020-2021 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef __OTA_PRIVATE_H__
#define __OTA_PRIVATE_H__

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <components/log.h>
#include <os/os.h>
#include "modules/ota.h"

#define OTA_TAG "OTA"

#ifdef CONFIG_OTA_DEBUG_LOG_OPEN
#define OTA_LOGI(...)	BK_LOGI(OTA_TAG, ##__VA_ARGS__)
#define OTA_LOGW(...)	BK_LOGW(OTA_TAG, ##__VA_ARGS__)
#define OTA_LOGE(...)	BK_LOGE(OTA_TAG, ##__VA_ARGS__)
#define OTA_LOGD(...)	BK_LOGD(OTA_TAG, ##__VA_ARGS__)
#else
#define OTA_LOGI(...)	BK_LOGI(OTA_TAG, ##__VA_ARGS__)
#define OTA_LOGW(...) 
#define OTA_LOGE(...)	BK_LOGE(OTA_TAG, ##__VA_ARGS__)
#define OTA_LOGD(...) 
#endif

#define OTA_ASSERT_ERR(cond)                              \
    do {                                             \
        if (!(cond)) {                                \
            os_printf("%s,condition %s,line = %d\r\n",__func__, #cond, __LINE__);  \
        }                                             \
    } while(0)


#define RBL_HEAD_POS            (0x1000)
#define RT_OTA_HASH_FNV_SEED    (0x811C9DC5)
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

struct ota_rbl_head
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

int32_t ota_get_rbl_head(const bk_logic_partition_t *bk_ptr, struct ota_rbl_head *hdr);
int32_t ota_hash_verify(const bk_logic_partition_t *part, const struct ota_rbl_head *hdr);
#endif


