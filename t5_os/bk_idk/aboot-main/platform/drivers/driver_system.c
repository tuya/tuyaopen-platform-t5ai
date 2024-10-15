/*************************************************************
 * @file        driver_system.c
 * @brief       code of ICU driver of BK7231
 * @author      GuWenFu
 * @version     V1.0
 * @date        2016-09-29
 * @par
 * @attention
 *
 * @history     2016-09-29 gwf    create this file
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "BK_System.h"
#include "driver_system.h"

#include "driver_gpio.h"
#include "driver_uart0.h"
#include "driver_uart1.h"
#include "driver_uart2.h"
#include "driver_sys_ctrl.h"

void ICU_init(void)
{
//    REG_WIFI_PWD = 0x0;
//    REG_DSP_PWD  = 0x0;

//    REG_USB_PWD  = 0x0;

    DelayUS(100);
        
}
// eof


