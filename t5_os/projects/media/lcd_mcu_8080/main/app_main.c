#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>

#include "media_service.h"

#include "cli.h"
#include "lcd_mcu_8080.h"

extern void user_app_main(void);
extern void rtos_set_user_app_entry(beken_thread_function_t entry);

#if (CONFIG_SYS_CPU0 && !CONFIG_SOC_BK7256)
#define CMDS_COUNT  (sizeof(s_lcd_8080_commands) / sizeof(struct cli_command))

/*
lcd mcu 8080 display demo cli commond:
1: lcd_8080 open
2: lcd_8080 start
3: lcd_8080 close
*/
void cli_lcd_8080_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (os_strcmp(argv[1], "open") == 0)
	{
		os_printf("open lcd\n\r");
		lcd_mcu_8080_open();

	}
	else if (os_strcmp(argv[1], "start") == 0)
	{
		os_printf("start display\n\r");
		lcd_mcu_8080_display();
	}
	else if (os_strcmp(argv[1], "close") == 0)
	{
		os_printf("close lcd\n\r");
		lcd_mcu_8080_close();
	}
}

static const struct cli_command s_lcd_8080_commands[] =
{
	{"lcd_8080", "lcd_8080", cli_lcd_8080_cmd},
};

int cli_lcd_8080_init(void)
{
	return cli_register_commands(s_lcd_8080_commands, CMDS_COUNT);
}
#endif

void user_app_main(void)
{

}

int main(void)
{
#if (CONFIG_SYS_CPU0 && !CONFIG_SOC_BK7256)
	rtos_set_user_app_entry((beken_thread_function_t)user_app_main);
	// bk_set_printf_sync(true);
	// shell_set_log_level(BK_LOG_WARN);
#endif
	bk_init();
    media_service_init();
	#if (CONFIG_SYS_CPU0 && !CONFIG_SOC_BK7256)
	cli_lcd_8080_init();
	#endif
	return 0;
}
