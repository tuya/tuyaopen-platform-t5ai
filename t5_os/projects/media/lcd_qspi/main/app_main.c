#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/mem.h>
#include <os/str.h>
#include <os/os.h>
#include <components/shell_task.h>
#include <common/bk_include.h>

#include <driver/qspi.h>
#include <driver/qspi_types.h>
#include <driver/lcd_qspi.h>
#include <driver/lcd_qspi_types.h>
#include <driver/lcd.h>

extern void user_app_main(void);
extern void rtos_set_user_app_entry(beken_thread_function_t entry);
extern int bk_cli_init(void);
extern void bk_set_jtag_mode(uint32_t cpu_id, uint32_t group_id);

void user_app_main(void){

}

extern int bk_rand(void);
#if CONFIG_SH8601A_PARTIAL
extern void sh8601a_set_display_mem_area(uint16 xs, uint16 xe, uint16 ys, uint16 ye);
#endif

void lcd_qspi_sh8601a(void){
	uint32_t random;
	char *device_name="sh8601a";//lcd屏幕名称
	const lcd_device_t *lcd_device = NULL;

#if CONFIG_SH8601A_PARTIAL
	uint16_t xs, xe, ys, ye;//显示区域的x方向和y方向的起点和结束点
#endif

	os_printf("\n\n\nsh8601a qspi_lcd tset start!\n\n\r");

    lcd_device = get_lcd_device_by_name(device_name);
    if (lcd_device == NULL) {
        os_printf("lcd device not found, please input correct device name!\r\n");
        return;
    }

    bk_lcd_qspi_init(lcd_device);
    lcd_driver_backlight_open();

	while(1)
	{
		random=bk_rand()&0xffffff;//获取随机颜色

#if CONFIG_SH8601A_PARTIAL
		/*获取随机坐标和大小*/
		xs = bk_rand() & 0x1FF;
		if(xs>453)//确保获取的随机坐标小于屏幕最大坐标
		xs=453;
		else{
			if(xs%2!=0)
			xs=xs+1;//确保获取的随机数为偶数
			}

		xe = bk_rand() & 0x1FF;
		if(xe>453)
			xe=453;
		else{
			if(xe%2!=0)
				xe=xe+1;
		}

		ys = bk_rand() & 0x1FF;
		if(ys>453)
			ys=453;
		else{
			if(ys%2!=0)
				ys=ys+1;
		}

		ye = bk_rand() & 0x1FF;
		if(ye>453)
			ye=453;
		else{
			if(ye%2!=0)
				ye=ye+1;
		}
		
		/*确保随机的结束地址大于或等于起始地址*/
		if(ys>ye)
		{
			ye=ys;
			ys=ye;
		}
		if(xs>xe)
		{
			xe=xs;
			xs=xe;
		}

		/*设置获取的随机显示区域*/
		sh8601a_set_display_mem_area(xs, xe, ys, ye);


		os_printf("xs,ys:(0x%x,0x%x),xe,ye:(0x%x,0x%x)\r\n",xs,ys,xe,ye);
#endif
		
		bk_lcd_qspi_send_data(LCD_QSPI_ID, lcd_device, &random, 1);//将随机颜色填充到屏幕
		rtos_delay_milliseconds(1000);
	}
}

int main(void)
{
#if (CONFIG_SYS_CPU0)
	rtos_set_user_app_entry((beken_thread_function_t)user_app_main);
	// bk_set_printf_sync(true);
	// shell_set_log_level(BK_LOG_WARN);
	bk_set_jtag_mode(0, 0);
#endif
	bk_init();
	lcd_qspi_sh8601a();
	return 0;
}