/*
 * File      : ota.c
 * COPYRIGHT (C) 2012-2018, Shanghai Real-Thread Technology Co., Ltd
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-01-27     MurphyZhao   the first version
 */

#include <fal.h>
#include <string.h>
#include "BK_System.h"
#include "driver_flash.h"
#include "interrupt.h"

#if CFG_BEKEN_OTA
#include "data_move.h"
#endif

#if CFG_BEKEN_OTA
//int ota_main(void)
int __attribute__((section(".itcm_write_flash"))) ota_main(void)
{
	int ret;
	ret = fal_init();
	if (ret == -1)
    {
    	bk_printf("partition table not found!\r\n");
		return ret;
    }else if(ret == -2){
		bk_printf("download partition not found!\r\n");
		return ret;
	}

	bk_printf("ota_main start \r\n");
#if (CFG_SOC_NAME != SOC_BK7256)
	sys_forbidden_interrupts();
#endif

	set_flash_protect(NONE);
	// ret = data_move_handler();
	if (ty_bsdiff_entry(NULL) != 0)
	{
		bk_printf("ty_bsdiff_entry error!\n");
	}
	
	bk_printf("\r\n");
	bk_printf("data_move_handler return -");
	bk_print_hex(-ret);
	bk_printf("\r\n");

        if (ret == RET_DM_FAILURE)
        {
                bk_printf("RET_DM_FAILURE!\r\n");
                return ret;
        }
        else if(ret == RET_DM_NO_OTA_DATA)
        {
                bk_printf("download partition NO_OTA_DATA!\r\n");
                return ret;
        }
        else if(ret == RET_DM_SUCCESS)
        {   
                bk_printf("download partition over!\r\n");
                wdt_reboot();
        }
	
	set_flash_protect(ALL);
	return 0;
}
#endif

// eof

