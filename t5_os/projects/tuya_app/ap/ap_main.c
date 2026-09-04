#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>
#include <modules/pm.h>
#include <driver/pwr_clk.h>
#include <media_service.h>

#include "FreeRTOS.h"
#include "task.h"
#include "tuya_cloud_types.h"

extern OPERATE_RET tuya_ipc_init(void);
extern int tuya_upgrade_main(void);
extern void tuya_app_main(void);
extern TUYA_OTA_PATH_E tkl_ota_is_under_seg_upgrade(void);
extern void test_core_mark(void);

#define OTP_FLASH_DATA_SIZE        1024
#define OTP_FLASH_RFDATA_SIZE       512
#define PARTITION_SIZE         (1 << 12) /* 4KB */

#include "string.h"
#include "bk_wifi_types.h"
#include "flash_bypass.h"
#include <common/bk_typedef.h>
#include "driver/flash.h"
#include <driver/flash_partition.h>
#include <os/mem.h>
#include <modules/wifi.h>

static void __read_otp_flash_rfcali_data(uint8_t *otp_data, uint16_t len)
{
    flash_bypass_otp_ctrl_t otp_op     = {0};
    otp_op.otp_idx		= 1;  // "1 or 2 or 3"
    otp_op.addr_offset  = 0;
    otp_op.write_len 	= 0;
    otp_op.write_buf	= NULL;
    otp_op.read_len		= len;
    otp_op.read_buf		= otp_data;

    int ret = flash_bypass_otp_operation(FLASH_BYPASS_OTP_READ, &otp_op);
	if (ret != BK_OK)
		bk_printf("otp flash read failed\n");
}

static int __check_otp_flash_rfcali_data(uint8_t *otp_data, uint16_t len)
{
    struct txpwr_elem_st
    {
        UINT32 type;
        UINT32 len;
    } *head;

    head = (struct txpwr_elem_st *)otp_data;
    if (head->type != INFO_TLV_HEADER) {
        bk_printf("otp flash data type error %x\n", head->type);
        return -1;
    }
    return 0;
}

int backup_rfcali_data(void)
{
    uint32_t addr;
    uint8_t *dst;
    uint32_t size;

    dst = os_malloc(OTP_FLASH_DATA_SIZE);
    if (dst == NULL) {
        bk_printf("malloc rfcali data failed\n");
        return -1;
    }

    memset(dst, 0, OTP_FLASH_DATA_SIZE);

    flash_bypass_otp_ctrl_t otp_op     = {0};
    otp_op.otp_idx      = 1;  // "1 or 2 or 3"
    otp_op.addr_offset  = 0;
    otp_op.write_len	= 0;
    otp_op.write_buf	= NULL;
    otp_op.read_len		= OTP_FLASH_DATA_SIZE;
    otp_op.read_buf		= dst;
    int ret = flash_bypass_otp_operation(FLASH_BYPASS_OTP_READ, &otp_op);
	if (ret != BK_OK) {
        bk_printf("read otp flash failed\n");
		if (dst){
			os_free(dst);
            dst = NULL;
        }

		return -1;
	}

	bk_logic_partition_t *pt = bk_flash_partition_get_info(BK_PARTITION_SYS_RF);
    addr = pt->partition_start_addr;
    size = OTP_FLASH_RFDATA_SIZE;
	bk_flash_read_bytes(addr, (uint8_t *)dst, size);

	memset(&otp_op, 0, sizeof(flash_bypass_otp_ctrl_t));
    otp_op.otp_idx      = 1;  // "1 or 2 or 3"
    otp_op.addr_offset  = 0;
    otp_op.write_len	= OTP_FLASH_DATA_SIZE;
    otp_op.write_buf	= dst;
    otp_op.read_len		= 0;
    otp_op.read_buf		= NULL;

    ret = flash_bypass_otp_operation(FLASH_BYPASS_OTP_WRITE, &otp_op);
	if (ret != BK_OK) {
        bk_printf("write otp flash failed\n");

        if (dst) {
            os_free(dst);
            dst = NULL;
        }

        return -1;
    }

    if (dst) {
        os_free(dst);
        dst = NULL;
    }

    return 0;
}

static void __recovery_rfcali_data(uint8_t *otp_data, uint16_t len)
{
    uint32_t addr;

	bk_logic_partition_t *pt = bk_flash_partition_get_info(BK_PARTITION_SYS_RF);
    addr = pt->partition_start_addr;

    bk_flash_set_protect_type(FLASH_PROTECT_NONE);
	bk_flash_erase_sector(addr);
    bk_flash_write_bytes(addr, (const uint8_t *)otp_data, OTP_FLASH_RFDATA_SIZE);
    bk_flash_set_protect_type(FLASH_UNPROTECT_LAST_BLOCK);

	bk_printf("recovery rfcali data success\n");
}
static int user_recovery_rfcali_data(void)
{
	int stat = bk_wifi_manual_cal_rfcali_status();

    if(stat == BK_OK) {
		return 0;
    }

    bk_printf("[NOTE]: rfcali data isn't exist\n");

    uint8_t *otp_data = os_malloc(OTP_FLASH_DATA_SIZE);
    if(otp_data == NULL) {
        bk_printf("malloc rfcali data failed\n");
        return -1;
    }
    memset(otp_data, 0, OTP_FLASH_DATA_SIZE);
    __read_otp_flash_rfcali_data(otp_data, OTP_FLASH_DATA_SIZE);

    if (__check_otp_flash_rfcali_data(otp_data, OTP_FLASH_DATA_SIZE) < 0) {
        bk_printf("check rfcali data failed\n");
    } else {
        bk_printf("check rfcali data success\n");
        __recovery_rfcali_data(otp_data, OTP_FLASH_RFDATA_SIZE);
    }
    os_free(otp_data);

    return 0;
}

static void entry_app_main(void)
{
    bk_printf("-------- app startup, left sram: %d, psram: %d, reset reason: %x\r\n",
            xPortGetFreeHeapSize(), xPortGetPsramFreeHeapSize(), bk_misc_get_reset_reason() & 0xFF);

    bk_pm_module_vote_cpu_freq(PM_DEV_ID_CPU1, PM_CPU_FRQ_480M);

    // tuya_ipc_init();
    
        extern int tkl_sleep_param_check_and_set(void);
        tkl_sleep_param_check_and_set();

        user_recovery_rfcali_data();

        bk_printf("go to tuya\r\n");
        tuya_app_main();
        // test_core_mark();

#if (CONFIG_TUYA_TEST_CLI)
        extern int ap_cli_tuya_test_init(void);
        ap_cli_tuya_test_init();
#endif
}

int main(void)
{
    bk_init();
    media_service_init();

    entry_app_main();

    return 0;
}
