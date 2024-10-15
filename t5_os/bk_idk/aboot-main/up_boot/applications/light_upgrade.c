#if 0
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "BK_System.h"
#include "driver_system.h"
#include "driver_gpio.h"
#include "drv_timer.h"
#include "drv_pwm.h"
#include "bk_timer.h"
#include "gpio_pub.h"
#include "driver_flash.h"
#include "drv_uart.h"

#define FLASH_UPGRADE_LIGHT_DATA_ADDR       0x1E1000
#define FLASH_UPGRADE_LIGHT_DATA_SIZE       4096

#define PIN_NOEXIST        0xFF  ///< default pin def


typedef enum {
    DRIVER_MODE_PWM = 0,
    DRIVER_MODE_SM16726B
}DRIVER_MODE_T;

typedef struct  __attribute__((packed)) 
{
    u16 frequency;
    u8  channel_num;
    u8  pwm_list[5];
}PWM_INIT_S;

typedef struct  __attribute__((packed)) 
{
    u8 ucSDA_IO;
    u8 ucSCL_IO;
}I2C_PIN_S;

typedef struct  __attribute__((packed)) 
{
    u16      magic_code; 	//0xA55A
    u8       ucMode;			//驱动方式
    u8       ucCTRL_IO;  	// ctrl 管脚
    u8       bCTRL_LV;  		// ctrl 管脚有效点评
	u16      duty[5];   		// 占空比
	PWM_INIT_S          pwm_init;  		// pwm设置
    I2C_PIN_S           i2c_pin;  		// i2c管脚
}LIGHT_CUR_DATA_FLASH_S;


void upgrade_boot_flash_earse(void)
{
    LIGHT_CUR_DATA_FLASH_S light_cur_data_flash;
	
	flash_read_data( (u8 *)&light_cur_data_flash, FLASH_UPGRADE_LIGHT_DATA_ADDR, sizeof(LIGHT_CUR_DATA_FLASH_S));
	
	if(light_cur_data_flash.magic_code != 0xA55A) {	// magic check!
		return ;
	}
	
    flash_erase_sector(FLASH_UPGRADE_LIGHT_DATA_ADDR);
}

static int upgrade_boot_jude(LIGHT_CUR_DATA_FLASH_S* light_cur_data_flash)
{	
	flash_read_data((u8 *)light_cur_data_flash, FLASH_UPGRADE_LIGHT_DATA_ADDR,  sizeof(LIGHT_CUR_DATA_FLASH_S));
	
	if(light_cur_data_flash->magic_code != 0xA55A) {	// magic check!
		return -1;
	}
	
	return 0;
}

static void vI2CSDAReset(I2C_PIN_S i2c_pin)
{
    BkGpioInitialize(i2c_pin.ucSDA_IO,OUTPUT_NORMAL);
    BkGpioOutputLow(i2c_pin.ucSDA_IO);
}

static void vI2CSDASet(I2C_PIN_S i2c_pin)
{
    BkGpioInitialize(i2c_pin.ucSDA_IO,INPUT_PULL_UP);
}

static void vI2CSCLSet(I2C_PIN_S i2c_pin)
{
    BkGpioInitialize(i2c_pin.ucSCL_IO,INPUT_PULL_UP);
}

static void vI2CSCLReset(I2C_PIN_S i2c_pin)
{
    BkGpioInitialize(i2c_pin.ucSCL_IO,OUTPUT_NORMAL);
    BkGpioOutputLow(i2c_pin.ucSCL_IO);
}

static void vI2CDelay(u16 usDelayTime)
{
    while(usDelayTime)
    {
        usDelayTime --; 
    }
}

static void SM16726B_I2C_Send_Byte(I2C_PIN_S i2c_pin, u8 SendByte)
{
    u8 i = 0;

    for(i = 0; i < 8; i ++)
    {
        vI2CSCLReset(i2c_pin);
        vI2CDelay(10);

        if(SendByte & 0x80) {
            vI2CSDASet(i2c_pin); 
        } else {
            vI2CSDAReset(i2c_pin);  
        }
    
        SendByte <<= 1;
        vI2CDelay(10);

        vI2CSCLSet(i2c_pin);
        vI2CDelay(10);  
    }
    vI2CSCLReset(i2c_pin);
}

static void i2c_init_send(I2C_PIN_S i2c_pin, u8 *pBuffer, u8 NumByteToWrite)
{
    u8 start_bit_cnt = 6;

    while(start_bit_cnt--){
        SM16726B_I2C_Send_Byte(i2c_pin, 0x00);
    }
    SM16726B_I2C_Send_Byte(i2c_pin,0x01);     /* send 50bits '0' firstly at least */

    while(NumByteToWrite --)
    {
        SM16726B_I2C_Send_Byte(i2c_pin,*pBuffer ++);  
    }

    vI2CSDASet(i2c_pin);       /* send action addition pulse pull sda pin high */
    vI2CDelay(10);
    vI2CSCLReset(i2c_pin);
    vI2CDelay(10);
    vI2CSCLSet(i2c_pin);
    vI2CDelay(10);  
    vI2CSCLReset(i2c_pin);     /* send one addition pulse to scl pin */
    
    vI2CDelay(200);
}

static void pwm_init_send(LIGHT_CUR_DATA_FLASH_S *light_cur_data_flash)
{
    u8 i = 0;
    unsigned int period = 0;

    period = (unsigned int) (26000000 / light_cur_data_flash->pwm_init.frequency);
    
    for(i = 0; i < light_cur_data_flash->pwm_init.channel_num; i++ ) {
        bk_pwm_initialize(light_cur_data_flash->pwm_init.pwm_list[i], period, (unsigned int)(light_cur_data_flash->duty[i] / 1000.0 * period));
        bk_pwm_start(light_cur_data_flash->pwm_init.pwm_list[i]);
    }
}

void boot_ota_pre_process(void)
{
    u8 offset = 0;
    u8 IIC_Sendbuf[3] = {0,0,0};
    LIGHT_CUR_DATA_FLASH_S light_cur_data_flash;

    if(upgrade_boot_jude(&light_cur_data_flash) != 0) {
		return;
	}
    #if 1
            bk_printf("opL %x %x %x %x %x %x\r\n",light_cur_data_flash.magic_code,
            light_cur_data_flash.ucMode,
            light_cur_data_flash.ucCTRL_IO,
            light_cur_data_flash.bCTRL_LV,
            light_cur_data_flash.pwm_init.frequency,
            light_cur_data_flash.pwm_init.channel_num);
            bk_printf(" %x %x %x %x %x\r\n",
            light_cur_data_flash.pwm_init.pwm_list[0],
            light_cur_data_flash.pwm_init.pwm_list[1],
            light_cur_data_flash.pwm_init.pwm_list[2],
            light_cur_data_flash.pwm_init.pwm_list[3],
            light_cur_data_flash.pwm_init.pwm_list[4]);
            bk_printf(" %x %x %x %x %x \r\n\r\n",
            light_cur_data_flash.duty[0],
            light_cur_data_flash.duty[1],
            light_cur_data_flash.duty[2],
            light_cur_data_flash.duty[3],
            light_cur_data_flash.duty[4]);
            bk_printf(" %x %x \r\n\r\n",
            light_cur_data_flash.i2c_pin.ucSDA_IO,
            light_cur_data_flash.i2c_pin.ucSCL_IO);
            uart1_wait_tx_finish();
#endif

    if (light_cur_data_flash.pwm_init.channel_num <= 2) {
        offset = 2;
    }

    if(light_cur_data_flash.ucCTRL_IO != PIN_NOEXIST) {
        if ((light_cur_data_flash.duty[offset + 0] != 0) || (light_cur_data_flash.duty[offset + 1] != 0) || (light_cur_data_flash.duty[offset + 2] != 0)) {
            BkGpioInitialize(light_cur_data_flash.ucCTRL_IO,OUTPUT_NORMAL);
            if(light_cur_data_flash.bCTRL_LV) {
                BkGpioOutputHigh(light_cur_data_flash.ucCTRL_IO);
            } else {
                BkGpioOutputLow(light_cur_data_flash.ucCTRL_IO);
            }
        }
    }

    pwm_init_send(&light_cur_data_flash);

    if (DRIVER_MODE_SM16726B == light_cur_data_flash.ucMode) {
        IIC_Sendbuf[0] = light_cur_data_flash.duty[2]*255/1000;
        IIC_Sendbuf[1] = light_cur_data_flash.duty[3]*255/1000;
        IIC_Sendbuf[2] = light_cur_data_flash.duty[4]*255/1000;
        i2c_init_send(light_cur_data_flash.i2c_pin, IIC_Sendbuf, 3);
    }
	
	return;
}

#endif
