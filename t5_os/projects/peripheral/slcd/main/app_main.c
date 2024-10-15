#include "bk_private/bk_init.h"
#include <components/system.h>
#include <os/os.h>
#include <components/shell_task.h>
#include "cli.h"
#include <driver/slcd_types.h>
#include <driver/slcd.h>


extern void user_app_main(void);
extern void rtos_set_user_app_entry(beken_thread_function_t entry);
extern void bk_set_jtag_mode(uint32_t cpu_id, uint32_t group_id);


#define CMDS_COUNT  (sizeof(s_slcd_commands) / sizeof(struct cli_command))

void cli_slcd_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	os_printf("%s\r\n", __func__);
}

static const struct cli_command s_slcd_commands[] =
{
	{"slcd", "slcd", cli_slcd_cmd},
};

int cli_slcd_init(void)
{
	return cli_register_commands(s_slcd_commands, CMDS_COUNT);
}


#if (CONFIG_SYS_CPU0)
void bk_slcd_main(void)
{
	cli_slcd_init();

	slcd_config_t slcd_config;

    bk_slcd_set_seg_port_enable(0xFFFFFFF);
    slcd_config.com_num = SLCD_COM_NUM_4;
    slcd_config.slcd_bias = SLCD_BIAS_1_PER_OF_3;
    slcd_config.slcd_rate = SLCD_RATE_LEVEL_1;
    bk_slcd_driver_init(slcd_config);

	bk_slcd_set_seg00_03_value(0x0F0F0F0F);
	bk_slcd_set_seg04_07_value(0x0F0F0F0F);
	bk_slcd_set_seg08_11_value(0x0F0F0F0F);
	bk_slcd_set_seg12_15_value(0x0F0F0F0F);
	bk_slcd_set_seg16_19_value(0x0F0F0F0F);
	bk_slcd_set_com_port_enable(0xF);
}
#endif

void user_app_main(void){

}

int main(void)
{
#if (CONFIG_SYS_CPU0)
	rtos_set_user_app_entry((beken_thread_function_t)user_app_main);
	// bk_set_printf_sync(true);
	// shell_set_log_level(BK_LOG_WARN);
#endif
	bk_init();

#if (CONFIG_SYS_CPU0)
	bk_slcd_main();
#endif

	return 0;
}