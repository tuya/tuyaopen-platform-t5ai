#include "sdkconfig.h"
#include <string.h>
#include "cli.h"
#include <components/system.h>
#include "driver/flash.h"
#include "modules/ota.h"
#include "_ota.h"
#include "utils_httpc.h"
#include "modules/wifi.h"

#include <driver/flash.h>
#include "partitions.h"
#include "CheckSumUtils.h"
#include "security_ota.h"
#include <driver/wdt.h>
#include <components/log.h>

#define OTA_MAGIC_WORD "\x42\x4B\x37\x32\x33\x36\x35\x38"
#define MANIFEST_SIZE  (4 * 1024)
extern void vPortEnableTimerInterrupt( void );
extern void vPortDisableTimerInterrupt( void );

static ota_parse_t ota_parse = {0};

#define TAG "ota"

#if (CONFIG_TFM_FWU)

#if CONFIG_INT_WDT
#include <driver/wdt.h>
#include <bk_wdt.h>
#endif


#include "sys_ctrl/sys_driver.h"

void wdt_init(void);
static uint32_t ota_image_flag = 0;

const ota_partition_info_t s_ota_partition_info[] = {
	{CONFIG_PRIMARY_MANIFEST_PHY_PARTITION_OFFSET,   CONFIG_PRIMARY_MANIFEST_PHY_PARTITION_SIZE,   FWU_IMAGE_TYPE_PRIMARY_MANIFEST},
	{CONFIG_SECONDARY_MANIFEST_PHY_PARTITION_OFFSET, CONFIG_SECONDARY_MANIFEST_PHY_PARTITION_SIZE, FWU_IMAGE_TYPE_SECONDARY_MANIFEST},
	{CONFIG_PRIMARY_BL2_PHY_PARTITION_OFFSET,        CONFIG_PRIMARY_BL2_PHY_PARTITION_SIZE,        FWU_IMAGE_TYPE_PRIMARY_BL2},
	{CONFIG_SECONDARY_BL2_PHY_PARTITION_OFFSET,      CONFIG_SECONDARY_BL2_PHY_PARTITION_SIZE,      FWU_IMAGE_TYPE_SECONDARY_BL2},
#if CONFIG_DIRECT_XIP
	{CONFIG_PRIMARY_ALL_PHY_PARTITION_OFFSET,        CONFIG_PRIMARY_ALL_PHY_PARTITION_SIZE,        FWU_IMAGE_TYPE_FULL},
#endif
#if CONFIG_OTA_OVERWRITE
	{CONFIG_OTA_PHY_PARTITION_OFFSET,                CONFIG_OTA_PHY_PARTITION_SIZE,                FWU_IMAGE_TYPE_FULL},
#endif
	{CONFIG_SECONDARY_ALL_PHY_PARTITION_OFFSET,      CONFIG_SECONDARY_ALL_PHY_PARTITION_SIZE,      FWU_IMAGE_TYPE_FULL}
};

static void security_ota_dump_partition_info(void)
{
	BK_LOGI(TAG, "%8s  %8s  %8s\r\n", "offset", "size", "fwu_id");
	for (uint32_t partition_id = 0; partition_id < sizeof(s_ota_partition_info)/sizeof(ota_partition_info_t); partition_id++) {
		const ota_partition_info_t *p = &s_ota_partition_info[partition_id];
		BK_LOGI(TAG, "%8x  %8x  %-6d\r\n", p->partition_offset, p->partition_size, p->fwu_image_id);
	}
}

static const ota_partition_info_t* security_ota_get_partition_info(void)
{
	uint32_t flash_offset;

	if (ota_parse.phase != OTA_PARSE_IMG
		|| ota_parse.index >= ota_parse.ota_header.image_num) {
		return NULL;
	}

	flash_offset = ota_parse.ota_image_header[ota_parse.index].flash_offset;

	for (uint32_t partition_id = 0; partition_id < sizeof(s_ota_partition_info)/sizeof(ota_partition_info_t); partition_id++) {
		if (flash_offset == s_ota_partition_info[partition_id].partition_offset) {
			return &s_ota_partition_info[partition_id];
		}
	}

	return NULL;
}

static uint32_t security_ota_get_fwu_image_id(void)
{
	const ota_partition_info_t *partition_info = security_ota_get_partition_info();

	if (!partition_info) {
		return FWU_IMAGE_TYPE_INVALID;
	}

	//TODO for BL2, update image_id per boot_flag
	return partition_info->fwu_image_id;
}

static psa_image_id_t security_ota_fwu2psa_image_id(uint32_t fwu_image_id)
{
	return (psa_image_id_t)FWU_CALCULATE_IMAGE_ID(FWU_IMAGE_ID_SLOT_STAGE, fwu_image_id, 0);
}

int bk_ota_check(psa_image_id_t ota_image)
{
	psa_status_t status;
	psa_image_id_t dependency_uuid;
	psa_image_version_t dependency_version;
	psa_image_info_t info;

#if CONFIG_INT_WDT
	bk_wdt_stop();
	bk_task_wdt_stop();
#endif 

	status = psa_fwu_query(ota_image, &info);
	if (status != PSA_SUCCESS) {
		BK_LOGE(TAG, "query status %d\r\n", status);
		goto _ret_fail;
	}
	if (info.state != PSA_IMAGE_CANDIDATE) {
		BK_LOGE(TAG, "info state %d\r\n", info.state);
		goto _ret_fail;
	}

	status = psa_fwu_install(ota_image, &dependency_uuid, &dependency_version);
	if (status != PSA_SUCCESS_REBOOT) {
		BK_LOGE(TAG, "install fail %d\r\n", status);
		goto _ret_fail;
	}

	status = psa_fwu_query(ota_image, &info);
	if (status != PSA_SUCCESS) {
		BK_LOGE(TAG, "query fail %d\r\n", status);
		goto _ret_fail;
	}
	if (info.state != PSA_IMAGE_REBOOT_NEEDED) {
		BK_LOGE(TAG, "info fail %d\r\n", info.state);
		goto _ret_fail;
	}

#if CONFIG_INT_WDT
	bk_wdt_start(CONFIG_INT_WDT_PERIOD_MS);
#endif
	return 0;

_ret_fail:
#if CONFIG_INT_WDT
	wdt_init();
#endif
	return -1;
}

void bk_ota_set_flag(uint32_t flag)
{
	ota_image_flag |= flag;
}

uint32_t bk_ota_get_flag(void)
{
	return ota_image_flag;
}

void bk_ota_clear_flag(void)
{
	ota_image_flag = 0;
}

void bk_ota_accept_image(void)
{
	int32_t ns_interface_lock_init(void);
	psa_image_id_t psa_image_id = (psa_image_id_t)FWU_CALCULATE_IMAGE_ID(FWU_IMAGE_ID_SLOT_ACTIVE, FWU_IMAGE_TYPE_FULL, 0);
	BK_LOGI(TAG, "accept image\r\n");
	ns_interface_lock_init();
	psa_fwu_accept(psa_image_id);
}


#endif // (CONFIG_TFM_FWU)


static int security_ota_parse_header(uint8_t **data, int *len);
static int security_ota_parse_image_header(uint8_t **data, int *len);

static int security_ota_parse_header(uint8_t **data, int *len)
{
	uint32_t data_len, offset;
	uint8_t *tmp;

	if (*len == 0) return 0;

	if (ota_parse.offset == 0) {
		BK_LOGI(TAG, "downloading OTA global header...\r\n");
	}

	tmp = (uint8_t *)&ota_parse.ota_header;
	data_len = sizeof(ota_header_t) - ota_parse.offset;
	if (*len < data_len) {
		os_memcpy(tmp + ota_parse.offset, *data, *len);
		ota_parse.offset += *len;
		return 0;
	} else {
		os_memcpy(tmp + ota_parse.offset, *data, data_len);
		*data += data_len;
		*len -= data_len;

		//check global header magic code!
		if(os_memcmp(OTA_MAGIC_WORD,tmp,8) != 0){
			BK_LOGE(TAG, "magic error\r\n");
			return BK_ERR_OTA_HDR_MAGIC;
		}

		/*calculate global header crc*/
		offset = sizeof(ota_parse.ota_header.magic) + sizeof(ota_parse.ota_header.crc);
		tmp += offset;
		CRC32_Update(&ota_parse.ota_crc, tmp, sizeof(ota_header_t) - offset);

		/*to next parse*/
		ota_parse.phase = OTA_PARSE_IMG_HEADER;
		ota_parse.offset = 0;
		if (ota_parse.ota_image_header) {
			os_free(ota_parse.ota_image_header);
		}
		offset = ota_parse.ota_header.image_num * sizeof(ota_image_header_t);
		ota_parse.ota_image_header = (ota_image_header_t *)os_malloc(offset);
		if (!ota_parse.ota_image_header) {
			BK_LOGE(TAG, "ota parse image header: oom\r\n");
			return BK_ERR_OTA_OOM;
		}
		BK_LOGI(TAG, "crc %x, version %x, header_len %x, image_num %x\r\n",
			ota_parse.ota_header.crc, ota_parse.ota_header.version, ota_parse.ota_header.header_len, ota_parse.ota_header.image_num);
	}

	return 0;
}

static int security_ota_parse_image_header(uint8_t **data, int *len)
{
	int i;
	uint32_t data_len, offset, crc_control;
	uint8_t *tmp;

	if (*len == 0) return 0;

	if (ota_parse.offset == 0) {
		BK_LOGI(TAG, "downloading OTA image header...\r\n");
	}

	tmp = (uint8_t *)ota_parse.ota_image_header;
	data_len = ota_parse.ota_header.image_num * sizeof(ota_image_header_t) - ota_parse.offset;
	if (*len < data_len) {
		os_memcpy(tmp + ota_parse.offset, *data, *len);
		ota_parse.offset += *len;
		return 0;
	} else {
		os_memcpy(tmp + ota_parse.offset, *data, data_len);
		*data += data_len;
		*len -= data_len;

		/*calculate header crc*/
		offset = ota_parse.ota_header.image_num * sizeof(ota_image_header_t);
		CRC32_Update(&ota_parse.ota_crc, tmp, offset);

		//TODO check image CRC!
		CRC32_Final(&ota_parse.ota_crc,&crc_control);
		if(crc_control != ota_parse.ota_header.crc){
			BK_LOGE(TAG, "crc error\r\n");
			return BK_ERR_OTA_IMG_HDR_CRC;
		}

		/*to next parse*/
		ota_parse.phase = OTA_PARSE_IMG;
		ota_parse.offset = 0;
		for (i = 0; i < ota_parse.ota_header.image_num; i++) {
			BK_LOGI(TAG, "image[%d], image_len=%x, image_offset=%x, flash_offset=%x\r\n", i,
				ota_parse.ota_image_header[i].image_len,
				ota_parse.ota_image_header[i].image_offset,
				ota_parse.ota_image_header[i].flash_offset);
		}
	}

	return 0;
}

uint32_t get_http_flash_wr_buf_max(void);
static void security_ota_write_flash(uint8_t **data, uint32_t len, uint32_t psa_image_id)
{
	uint32_t copy_len;
	uint32_t data_idx = 0;
	uint32_t http_flash_wr_buf_max = get_http_flash_wr_buf_max();

	while(data_idx < len){
		copy_len = min(len-(data_idx),http_flash_wr_buf_max - bk_http_ptr->wr_last_len);
		os_memcpy(bk_http_ptr->wr_buf + bk_http_ptr->wr_last_len, *data + data_idx, copy_len);
		data_idx += copy_len;
		bk_http_ptr->wr_last_len += copy_len;
		if(bk_http_ptr->wr_last_len >= http_flash_wr_buf_max){
#if CONFIG_TFM_FWU
			psa_fwu_write(psa_image_id, ota_parse.write_offset, (const void *)bk_http_ptr->wr_buf, http_flash_wr_buf_max);
#else
			extern void bk_flash_xip_update(uint32_t off, const void *src, uint32_t len);
			bk_flash_xip_update(ota_parse.write_offset, (const void *)bk_http_ptr->wr_buf, http_flash_wr_buf_max);
#endif
			ota_parse.write_offset += http_flash_wr_buf_max;
			bk_http_ptr->wr_last_len = 0;
		}
	}
}

static int security_ota_handle_image(uint8_t **data, int *len)
{
	uint32_t image_crc;

	vPortDisableTimerInterrupt();

	do {
		bk_wdt_feed();
		if (ota_parse.offset == 0) {
			BK_LOGI(TAG, "downloading OTA image%d, expected data len=%x...\r\n", ota_parse.index, ota_parse.ota_image_header[ota_parse.index].image_len);
			ota_parse.percent = 0;
			CRC32_Init(&ota_parse.ota_crc);
		}
#if CONFIG_TFM_FWU
		uint32_t fwu_image_id = security_ota_get_fwu_image_id();
		if (fwu_image_id == FWU_IMAGE_TYPE_INVALID) {
			if (*len) {
				BK_LOGE(TAG, "Invalid image ID, parse index=%d, parse offset=%x, len=%d, total_rx_len=%x\r\n",
					ota_parse.index, ota_parse.offset, *len, ota_parse.total_rx_len);
					vPortEnableTimerInterrupt();
				return BK_ERR_OTA_IMG_ID;
			}
			vPortEnableTimerInterrupt();
			return BK_OK;
		}

		bk_ota_set_flag(BIT(fwu_image_id));
		psa_image_id_t psa_image_id = security_ota_fwu2psa_image_id(fwu_image_id);
#else
		uint32_t psa_image_id = 0;
#endif
		uint32_t image_len = ota_parse.ota_image_header[ota_parse.index].image_len;
		uint32_t data_len = image_len - ota_parse.offset;

		if (ota_parse.offset >= (image_len/10 + image_len*ota_parse.percent/100)) {
			ota_parse.percent += 10;
			if (ota_parse.percent < 100) {
				BK_LOGI(TAG, "download %d%%\r\n", ota_parse.percent);
			}
		}

		if (*len < data_len) {
			security_ota_write_flash(data, *len, psa_image_id);
			CRC32_Update(&ota_parse.ota_crc,*data,*len);
			ota_parse.offset += *len;
			*len = 0;
		} else {
			security_ota_write_flash(data, *len, psa_image_id);
			uint32_t align_len = (((bk_http_ptr->wr_last_len) + ((32) - 1)) & ~((32) - 1));  // align_up 32
			memset(bk_http_ptr->wr_buf + bk_http_ptr->wr_last_len,0xFF,align_len - bk_http_ptr->wr_last_len);
#if CONFIG_TFM_FWU
			psa_fwu_write(psa_image_id, ota_parse.write_offset, (const void *)bk_http_ptr->wr_buf, align_len);
#else
			extern void bk_flash_xip_update(uint32_t off, const void *src, uint32_t len);
			bk_flash_xip_update(ota_parse.write_offset, (const void *)bk_http_ptr->wr_buf, align_len);
#endif
			bk_http_ptr->wr_last_len = 0;
			CRC32_Update(&ota_parse.ota_crc,*data,*len);
			*data += data_len;
			*len = 0;

			BK_LOGI(TAG, "downloaded OTA image%d\r\n", ota_parse.index);
			//check image CRC, then we can abort quickly!
			CRC32_Final(&ota_parse.ota_crc,&image_crc);
			if(image_crc !=  ota_parse.ota_image_header[ota_parse.index].checksum){
				BK_LOGE(TAG, "image crc error!\r\n");
				vPortEnableTimerInterrupt();
				return BK_ERR_OTA_IMG_CRC;
			}

			/*to next image*/
			BK_LOGI(TAG, "\r\n");
			ota_parse.index++;
			ota_parse.offset = 0;
			ota_parse.write_offset = 0;
		}
	} while(*len);

	vPortEnableTimerInterrupt();

	return BK_OK;
}

int security_ota_parse_data(char *data, int len)
{
	int ret = 0;

	ota_parse.total_rx_len += len;
	if (ota_parse.phase == OTA_PARSE_HEADER) {
		ret = security_ota_parse_header((uint8_t **)&data, &len);
		if (ret != BK_OK)
			return ret;
	}

	if (ota_parse.phase == OTA_PARSE_IMG_HEADER) {
		ret = security_ota_parse_image_header((uint8_t **)&data, &len);
		if (ret != BK_OK)
			return ret;
	}

	if (ota_parse.phase == OTA_PARSE_IMG) {
		if (len == 0) return 0;

		ret = security_ota_handle_image((uint8_t **)&data, &len);
		if (ret != BK_OK)
			return ret;

	}

	return BK_OK;
}

void security_ota_init(void)
{
	if (ota_parse.phase != OTA_PARSE_HEADER) {
		BK_LOGI(TAG, "abort previous OTA\r\n");
#if CONFIG_TFM_FWU
		psa_image_id_t id = security_ota_fwu2psa_image_id(FWU_IMAGE_TYPE_FULL);
		psa_fwu_abort(id);
#endif
	}
	bk_flash_set_protect_type(FLASH_PROTECT_NONE);
#if CONFIG_TFM_FWU
	security_ota_dump_partition_info();
	bk_ota_clear_flag();
#else
	extern void bk_flash_xip_erase(void);
	bk_flash_xip_erase();
#endif
	os_memset(&ota_parse, 0, sizeof(ota_parse_t));
	CRC32_Init(&ota_parse.ota_crc);

	if (!bk_http_ptr->wr_buf) {
		bk_http_ptr->wr_buf = (uint8_t*)os_malloc(4096 * sizeof(char));
	}
	bk_http_ptr->wr_last_len = 0;
}

int security_ota_deinit(void)
{
	if (ota_parse.ota_image_header) {
		os_free(ota_parse.ota_image_header);
		ota_parse.ota_image_header = NULL;
	}

	os_free(bk_http_ptr->wr_buf);
	bk_http_ptr->wr_buf = NULL;

	return 0;
}

int security_ota_finish(void)
{
#if (CONFIG_TFM_FWU)
	int ret;
	psa_image_id_t psa_image_id = 0;
	uint8_t fwu_image_id = 0;

	uint32_t ota_flags = bk_ota_get_flag();
	while (ota_flags) {
		if (ota_flags & 1) {
#if CONFIG_DIRECT_XIP
			/*when run in B cannot read A,so don't check A*/
			BK_LOGI(TAG, "reboot\r\n");
			psa_fwu_request_reboot();
#endif
			BK_LOGI(TAG, "checking fwu image%d...\r\n", fwu_image_id);
			psa_image_id = (psa_image_id_t)FWU_CALCULATE_IMAGE_ID(FWU_IMAGE_ID_SLOT_STAGE, fwu_image_id, 0);
			ret = bk_ota_check(psa_image_id);
			if (ret != BK_OK) {
				BK_LOGI(TAG, "check fwu image%d failed\r\n", fwu_image_id);
				return BK_FAIL;
			} else {
				BK_LOGI(TAG, "check fwu image%d success\r\n", fwu_image_id);
			}
		}
		ota_flags >>= 1;
		fwu_image_id++;
	}
	
	BK_LOGI(TAG, "reboot\r\n");
	psa_fwu_request_reboot();
#else // CONFIG_TFM_FWU
#if CONFIG_DIRECT_XIP
		extern uint32_t flash_get_excute_enable();
		uint32_t update_id = (flash_get_excute_enable() ^ 1);
		uint32_t fa_addr;
		if (update_id  == 0) {
			fa_addr = CONFIG_PRIMARY_ALL_PHY_PARTITION_OFFSET;
		} else {
			fa_addr = CONFIG_SECONDARY_ALL_PHY_PARTITION_OFFSET;
		}
		uint32_t fa_size = CONFIG_PRIMARY_ALL_PHY_PARTITION_SIZE;
		uint32_t copy_done_offset = CEIL_ALIGN_34(fa_addr + fa_size - 4096) + 32;
		uint32_t status = XIP_SET;
		const uint8_t * value = (const uint8_t *)&(status);
		bk_flash_write_bytes(copy_done_offset,value,sizeof(value));
#endif // CONFIG_DIRECT_XIP
	bk_reboot_ex(RESET_SOURCE_OTA_REBOOT);
#endif /* CONFIG_TFM_FWU*/
	return BK_OK;
}
