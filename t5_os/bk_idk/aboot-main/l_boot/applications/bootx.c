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
#include "platform.h"

#define SYSTEM_TIMEOUT_STARTUP   (400)
extern int jump_to_app(void);
//static u32 reset;
u32 boot_downloading = 0x1;
u32 startup = FALSE;

void system_startup(void)
{
        bk_printf("jump_to_app to");
        //reset = UP_BOOT_ADDR;
        jump_to_app();   
}

void system_timeout_startup(void)
{

        if((1 == uart_download_status) )
        {
                return;
        }

        if(fclk_get_tick() > SYSTEM_TIMEOUT_STARTUP && startup == FALSE)
        {
                bk_printf("timeout startup");
                boot_downloading = FALSE;
                system_startup();
        }
}
// eof
