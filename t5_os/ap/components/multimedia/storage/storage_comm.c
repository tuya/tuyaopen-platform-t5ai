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

#include <os/os.h>
#include <stdio.h>
#include <os/mem.h>
#include <os/str.h>
#include <components/log.h>
#include <driver/psram.h>
#include <driver/flash.h>
#include <driver/flash_partition.h>

#if (CONFIG_FATFS)
#include "ff.h"
#include "diskio.h"
#endif

#include "storage_act.h"

#define TAG "storage"

#define SECTOR                  0x1000

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

storage_flash_t storge_flash;

bk_err_t bk_sdcard_read_to_mem(char *filename, uint32_t* paddr, uint32_t *total_len)
{
	bk_err_t ret = BK_FAIL;
#if (CONFIG_FATFS)
	char cFileName[FF_MAX_LFN];
	FIL file;
	FRESULT fr;
	FSIZE_t size_64bit = 0;
	unsigned int uiTemp = 0;
	uint32_t once_read_len = 1024 * 2;

	// step 1: read picture from sd to psram
	sprintf(cFileName, "%d:/%s", DISK_NUMBER_SDIO_SD, filename);

	fr = f_open(&file, cFileName, FA_OPEN_EXISTING | FA_READ);
	if (fr != FR_OK)
	{
		LOGE("open %s fail.\r\n", filename);
		return ret;
	}

	uint8_t * sram_addr = os_malloc(once_read_len);
	if (sram_addr == NULL)
	{
		LOGE("sd buffer malloc failed\r\n");
		return ret;
	}

	char *ucRdTemp = (char *)sram_addr;
	size_64bit = f_size(&file);
	uint32_t total_size = (uint32_t)size_64bit;// total byte
	LOGD("read file total_size = %d.\r\n", total_size);
	*total_len = total_size;

	while(1)
	{
		fr = f_read(&file, ucRdTemp, once_read_len, &uiTemp);
		if (fr != FR_OK) {
			LOGE("read file fail.\r\n");
			goto out;
		}
		if (uiTemp == 0)
		{
			LOGD("read file complete.\r\n");
			ret = BK_OK;
			break;
		}
		if(once_read_len != uiTemp)
		{
			if (uiTemp % 4)
			{
				uiTemp = (uiTemp / 4 + 1) * 4;
			}
			bk_psram_word_memcpy(paddr, sram_addr, uiTemp);
		}
		else
		{
			bk_psram_word_memcpy(paddr, sram_addr, once_read_len);
			paddr += (once_read_len / 4);
		}
	}

out:

	if (sram_addr)
	{
		os_free(sram_addr);
		sram_addr == NULL;
	}

	fr = f_close(&file);
	if (fr != FR_OK)
	{
		LOGE("close %s fail!\r\n", filename);
	}
#else
	LOGW("Not support\r\n");
#endif

	return ret;
}

bk_err_t bk_mem_save_to_sdcard(char *filename, uint8_t *paddr, uint32_t total_len)
{
	bk_err_t ret = BK_FAIL;
#if (CONFIG_FATFS)
	FIL fp1;
	unsigned int uiTemp = 0;
	char file_name[50] = {0};

	sprintf(file_name, "%d:/%s", DISK_NUMBER_SDIO_SD, filename);

	FRESULT fr = f_open(&fp1, file_name, FA_CREATE_ALWAYS | FA_WRITE);
	if (fr != FR_OK)
	{
		LOGE("can not open file: %s, error: %d\n", file_name, fr);
		return ret;
	}

	LOGV("open file:%s!\n", file_name);

	fr = f_write(&fp1, (char *)paddr, total_len, &uiTemp);
	if (fr != FR_OK)
	{
		LOGE("f_write failed 1 fr = %d\r\n", fr);
		ret = BK_FAIL;
	}
	else
	{
		ret = BK_OK;
	}

	f_close(&fp1);

#endif

	return ret;
}

bk_err_t bk_mem_save_to_flash(char *filename, uint8_t *paddr, uint32_t total_len, storage_flash_t **info)
{
	bk_err_t ret = BK_FAIL;

	bk_logic_partition_t *pt = bk_flash_partition_get_info(BK_PARTITION_USR_CONFIG);
	LOGD("flash addr %x \n", pt->partition_start_addr);

	storge_flash.flash_image_addr = pt->partition_start_addr;
	storge_flash.flasg_img_length = total_len;

	bk_flash_set_protect_type(FLASH_PROTECT_NONE);
	for (int i = 0; i < total_len / SECTOR + 1; i++)
	{
		bk_flash_erase_sector(pt->partition_start_addr + (SECTOR * i));
	}

	ret = bk_flash_write_bytes(pt->partition_start_addr, (uint8_t *)paddr, total_len);
	if (ret != BK_OK)
	{
		LOGD("%s: storge to flsah error \n", __func__);
	}

	*info = &storge_flash;

	bk_flash_set_protect_type(FLASH_UNPROTECT_LAST_BLOCK);

	return ret;
}


bk_err_t bk_mem_append_save_to_sdcard(char *filename, uint8_t *paddr, uint32_t total_len)
{
	bk_err_t ret = BK_FAIL;

#if (CONFIG_FATFS)
	FIL fp1;
	unsigned int uiTemp = 0;
	char file_name[50] = {0};

	sprintf(file_name, "%d:/%s", DISK_NUMBER_SDIO_SD, filename);

	FRESULT fr = f_open(&fp1, file_name, FA_OPEN_APPEND | FA_WRITE);
	if (fr != FR_OK)
	{
		LOGE("can not open file: %s, error: %d\n", file_name, fr);
		return ret;
	}

	fr = f_write(&fp1, (char *)paddr, total_len, &uiTemp);
	if (fr != FR_OK)
	{
		LOGE("f_write failed 1 fr = %d\r\n", fr);
		ret = BK_FAIL;
	}
	else
	{
		ret = BK_OK;
	}

	f_close(&fp1);
#endif

	return ret;
}

bk_err_t bk_read_sdcard_file_length(char *filename)
{
	int ret = BK_FAIL;

#if (CONFIG_FATFS)
	char cFileName[FF_MAX_LFN];
	FIL file;
	FRESULT fr;

	do{
		if(!filename)
		{
			LOGE("%s param is null\r\n", __FUNCTION__);
			ret = BK_ERR_PARAM;
			break;
		}

		// step 1: read picture from sd to psram
		sprintf(cFileName, "%d:/%s", DISK_NUMBER_SDIO_SD, filename);

		fr = f_open(&file, cFileName, FA_OPEN_EXISTING | FA_READ);
		if (fr != FR_OK)
		{
			LOGE("open %s fail.\r\n", filename);
			ret = BK_ERR_OPEN;
			break;
		}

		ret = f_size(&file);

		f_close(&file);

	} while(0);
#else
	LOGW("Not support\r\n");
	ret = BK_ERR_NOT_SUPPORT;
#endif

	return ret;
}

