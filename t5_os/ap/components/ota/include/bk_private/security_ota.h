// Copyright 2020-2025 Beken
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

#pragma once

#include <common/bk_err.h>
#include "sdkconfig.h"
#include "CheckSumUtils.h"

#define BK_ERR_OTA_HDR_MAGIC   (BK_ERR_OTA_BASE - 1)
#define BK_ERR_OTA_OOM         (BK_ERR_OTA_BASE - 2)
#define BK_ERR_OTA_IMG_HDR_CRC (BK_ERR_OTA_BASE - 3)
#define BK_ERR_OTA_IMG_ID      (BK_ERR_OTA_BASE - 4)
#define BK_ERR_OTA_IMG_CRC     (BK_ERR_OTA_BASE - 5)

typedef struct ota_header_s {
        UINT64 magic;
        UINT32 crc;
        UINT32 version;
        UINT16 header_len;
        UINT16 image_num;
        UINT32 flags;
        UINT32 reserved[2];
} ota_header_t;

typedef struct ota_image_header_s {
        UINT32 image_len;
        UINT32 image_offset;
        UINT32 flash_offset;
        UINT32 checksum;
        UINT32 version;
        UINT32 flags;
        UINT32 reserved[2];
} ota_image_header_t;

typedef enum {
	OTA_PARSE_HEADER = 0,
	OTA_PARSE_IMG_HEADER,
	OTA_PARSE_IMG,
} ota_parse_type;

typedef struct ota_parse_s {
	ota_parse_type phase;
	ota_header_t ota_header;
	ota_image_header_t *ota_image_header;
	UINT32 offset;
	UINT32 index;
	UINT32 write_offset;
	CRC32_Context ota_crc;
	uint32_t total_rx_len;
	uint32_t percent;
} ota_parse_t;

typedef struct ota_image_info_s {
	uint32_t partition_offset;
	uint32_t partition_size;
	uint8_t fwu_image_id;
} ota_partition_info_t;


void bk_ota_accept_image(void);
void bk_ota_clear_flag(void);
uint32_t bk_ota_get_flag(void);
void bk_ota_set_flag(uint32_t flag);
int security_ota_finish(void);

#if (CONFIG_TFM_FWU)
typedef uint32_t psa_image_id_t;
int bk_ota_check(psa_image_id_t ota_image);
#endif

void security_ota_init(void);
int security_ota_deinit(void);
int security_ota_parse_data(char *data, int len);

