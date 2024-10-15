#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "BK_System.h"
#include "driver_system.h"
#include "driver_gpio.h"
#include "drv_timer.h"
#include "drv_pwm.h"
#include "driver_flash.h"
#include "crc.h"
#include "interrupt.h"

#define SYSTEM_TIMEOUT_STARTUP   (400)

u32 boot_downloading = TRUE;
u32 startup = FALSE;
u32 uart_download_status = 0;
//extern u32 uart_download_status;
extern int ota_main(void);
extern int jump_to_app(void);

 void __attribute__((section(".itcm_write_flash"))) system_startup(void)
{
        ota_main();
        bk_printf("system_startup \r\n");
        //printf("J 0x%x\r\n", OS_EX_ADDR);
#if (CFG_SOC_NAME != SOC_BK7256)
        sys_forbidden_interrupts();
#endif
        jump_to_app();
        while(1);
}

 void __attribute__((section(".itcm_write_flash"))) system_timeout_startup(void)
{
        if((1 == uart_download_status) )
        {
                return;
        }
        bk_printf("system_timeout_startup begin  \r\n");
        //if(fclk_get_tick() > SYSTEM_TIMEOUT_STARTUP && startup == FALSE)
        {
                bk_printf("system_timeout_startup middle  \r\n");
                boot_downloading = FALSE;
                system_startup();
        }
}




