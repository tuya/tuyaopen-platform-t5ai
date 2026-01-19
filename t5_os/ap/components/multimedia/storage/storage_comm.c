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

#include "bk_posix.h"
#include "storage_act.h"

#define TAG "storage"

#define SECTOR                  0x1000

#define LOGI(...) BK_LOGI(TAG, ##__VA_ARGS__)
#define LOGW(...) BK_LOGW(TAG, ##__VA_ARGS__)
#define LOGE(...) BK_LOGE(TAG, ##__VA_ARGS__)
#define LOGD(...) BK_LOGD(TAG, ##__VA_ARGS__)
#define LOGV(...) BK_LOGV(TAG, ##__VA_ARGS__)

storage_flash_t storge_flash;


#if (CONFIG_VFS)
static int bk_vfs_mount_sd0_fatfs(void) {
	int ret = BK_OK;
	static bool is_mounted = false;

	if(!is_mounted) {
		struct bk_fatfs_partition partition;
		char *fs_name = NULL;
		fs_name = "fatfs";
		partition.part_type = FATFS_DEVICE;
		partition.part_dev.device_name = FATFS_DEV_SDCARD;
		partition.mount_path = VFS_SD_0_PATITION_0;
		ret = mount("SOURCE_NONE", partition.mount_path, fs_name, 0, &partition);
		is_mounted = true;
	}
	return ret;
}
#endif

bk_err_t bk_sdcard_read_to_mem(char *filename, uint32_t* paddr, uint32_t *total_len)
{
	bk_err_t ret = BK_OK;
#if (CONFIG_VFS)
	int fd = 0;
	int bytes_read = 0;
	uint32_t once_read_len = 1024 * 2;
	char cFileName[VFS_FILE_MAX_LEN] = {0};
	sprintf(cFileName, "%s/%s", VFS_SD_0_PATITION_0, filename);

	bk_vfs_mount_sd0_fatfs();
	fd = open(cFileName, O_RDONLY);
	if (fd < 0) {
		LOGE("can't open %s\n", cFileName);
		return BK_FAIL;
	}

	uint8_t * sram_addr = os_malloc(once_read_len);
	if (sram_addr == NULL)
	{
		LOGE("sd buffer malloc failed\r\n");
		return BK_FAIL;
	}
	char *ucRdTemp = (char *)sram_addr;
	struct stat statbuf;
	ret = stat(cFileName, &statbuf);
	if (ret < 0) {
		LOGE("stat file %s fail\n", cFileName);
		close(fd);
		return BK_FAIL;
	}

	LOGD("statbuf->st_size =%d, statbuf->st_mode = %d.\r\n", statbuf.st_size, statbuf.st_mode);
	uint32_t total_size = (uint32_t)statbuf.st_size;// total byte
	LOGD("read file total_size = %d.\r\n", total_size);
	*total_len = total_size;

    do {
        bytes_read = read(fd, ucRdTemp, once_read_len);
		LOGV("read from %s, bytes_read=%d\n", cFileName, bytes_read);
        if (bytes_read > 0) {
            if(once_read_len != bytes_read) {
                if (bytes_read % 4) {
                    bytes_read = (bytes_read / 4 + 1) * 4;
                }
                bk_psram_word_memcpy(paddr, sram_addr, bytes_read);
            } else {
                bk_psram_word_memcpy(paddr, sram_addr, once_read_len);
                paddr += (once_read_len / 4);
            }
        } else if (bytes_read < 0) {
            LOGE("Read %s error", cFileName);
			ret = BK_FAIL;
            break;
        }
    } while (bytes_read != 0);  // 0表示EOF

	if (sram_addr) {
		os_free(sram_addr);
		sram_addr == NULL;
	}
	close(fd);
#else
	LOGW("VFS Not support\r\n");
#endif

	return ret;
}

bk_err_t bk_mem_save_to_sdcard(char *filename, uint8_t *paddr, uint32_t total_len)
{
	bk_err_t ret = BK_FAIL;
#if  (CONFIG_VFS)
	int fd = 0;
	int bytes_write = 0;
	char cFileName[VFS_FILE_MAX_LEN] = {0};


	bk_vfs_mount_sd0_fatfs();

	sprintf(cFileName, "%s/%s", VFS_SD_0_PATITION_0, filename);

	fd = open(cFileName, O_RDWR | O_CREAT | O_TRUNC);
	if (fd < 0) {
		LOGE("can't open %s\n", cFileName);
		return -1;
	}

	bytes_write = write(fd, (char *)paddr, total_len);
	LOGV("write to %s, bytes_write=%d\n", cFileName, bytes_write);
	close(fd);

	if(bytes_write == total_len) {
		ret = BK_OK;
	}
#else
	LOGW("Not support\r\n");
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

#if  (CONFIG_VFS)
	int fd = 0;
	int bytes_write = 0;
	char cFileName[VFS_FILE_MAX_LEN] = {0};

	bk_vfs_mount_sd0_fatfs();

	sprintf(cFileName, "%s/%s", VFS_SD_0_PATITION_0, filename);
	fd = open(cFileName, O_RDWR | O_CREAT | O_APPEND);
	if (fd < 0) {
		LOGE("can't open %s\n", cFileName);
		return -1;
	}

	bytes_write = write(fd, (char *)paddr, total_len);
	LOGV("write to %s, bytes_write=%d\n", cFileName, bytes_write);
	close(fd);

	if(bytes_write == total_len) {
		ret = BK_OK;
	}
#else
	LOGW("Not support\r\n");
#endif

	return ret;
}

bk_err_t bk_read_sdcard_file_length(char *filename)
{
	int ret = BK_FAIL;

#if (CONFIG_VFS)
	char cFileName[VFS_FILE_MAX_LEN] = {0};
	struct stat statbuf;

	bk_vfs_mount_sd0_fatfs();
	sprintf(cFileName, "%s/%s", VFS_SD_0_PATITION_0, filename);
	ret = stat(cFileName, &statbuf);
	if (ret < 0) {
		LOGE("stat file %s fail\n", cFileName);
		return BK_FAIL;
	}

	LOGD("statbuf->st_size =%d, statbuf->st_mode = %d.\r\n", statbuf.st_size , statbuf.st_mode);
	ret = (uint32_t)statbuf.st_size;// total byte
#else
	LOGW("Not support\r\n");
	ret = BK_ERR_NOT_SUPPORT;
#endif

	return ret;
}

