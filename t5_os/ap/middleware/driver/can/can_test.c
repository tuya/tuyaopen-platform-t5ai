#include "FreeRTOS.h"
#include "task.h"
#include "cli.h"
#include "gpio_driver.h"
#include "bk_icu.h"
#include "bk_gpio.h"
#include <driver/can.h>
#include <driver/can_types.h>
#include <driver/int_types.h>
#include <driver/int.h>
#include "can_hal.h"

#if (CONFIG_SYSTEM_CTRL)
#include "sys_driver.h"
#endif

#define CLI_CAN_STB 			0
#define CLI_CAN_PTB 			1
#define CLI_CAN_RCV_BUF_SIZE	69 // default 256
#define CLI_CAN_SEND_BUF_SIZE   64 // default 256

static uint32_t cli_can_id = 0x10000001;
static uint32_t cli_select_tb = CLI_CAN_STB;

static uint8_t demo_rcv_buf[CLI_CAN_RCV_BUF_SIZE];
static uint8_t demo_send_buf[CLI_CAN_SEND_BUF_SIZE];

static void cli_can_help(void)
{
	CLI_LOGD("can_transmit {rx|tx} {rx size|stb|ptb} {tx szie}");
	CLI_LOGD("can_init");
	CLI_LOGD("can_exit");
	CLI_LOGD("can_filter_cfg {aid} {acode} {amask}");
	CLI_LOGD("can_loop {i|e}");
	CLI_LOGD("can_speed {s_spend} {f_speed}");
	CLI_LOGD("can_statis {dump|reset}");

}

static void cli_can_driver_init(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (bk_can_driver_init() == BK_OK) {
		CLI_LOGD("init success\r\n");
	} else {
		CLI_LOGD("init failed\r\n");
	}
}

static void cli_can_driver_exit(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	bk_can_driver_deinit();
	CLI_LOGD("exit success\r\n");
}

static void cli_can_recevie(void *param)
{
	uint32_t id;
	uint32_t rec_size = 0, i = 0;
	uint32_t ex_size = (uint32_t)param;
	while (1) {
		bk_can_receive(demo_rcv_buf, ex_size, &rec_size, 1000);
		if (rec_size) {
			CLI_LOGD("recev data %d\r\n", rec_size);
			CLI_LOGD("dlc is 0x%02x\r\n", demo_rcv_buf[0]);
			id = (demo_rcv_buf[4] << 24) | \
				(demo_rcv_buf[3] << 16) | \
				(demo_rcv_buf[2] << 8) | \
				(demo_rcv_buf[1]);
			CLI_LOGD("id is 0x%02x\r\n", id);
			for (i = 5; i < rec_size; i++) {
				CLI_LOGD("data[%02d]:0x%02x\r\n", i, demo_rcv_buf[i]);
			}
		}
		os_memset(demo_rcv_buf, 0, sizeof(demo_rcv_buf));
		rtos_delay_milliseconds(100);
	}
	CLI_LOGD("recev end\r\n");
}

static void cli_can_send(void *param)
{
	can_frame_s frame;
	uint32_t i = 0;
	uint32_t ex_size = (uint32_t)param;
	for (i = 0; i < ex_size; i++) {
		demo_send_buf[i] = 0xbb;
	}
	frame.tag.fdf = CAN_PROTO_20;
	frame.tag.id = cli_can_id;
	frame.tag.rtr = 0;
	frame.tag.ide = 1;
	frame.tag.brs = 1;
	frame.size = ex_size;
	frame.data = demo_send_buf;
	if (cli_select_tb == CLI_CAN_STB) {
		bk_can_send(&frame, 1000);
	} else {
		bk_can_send_ptb(&frame);
	}
}

static void cli_can_transmit(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 3) {
		CLI_LOGD("param too few \r\n");
		cli_can_help();
		return;
	}
	uint32_t ex_size = 0;

	if (os_strcmp(argv[1], "rx") == 0) {
		ex_size = os_strtoul(argv[2], NULL, 10);
		cli_can_recevie((void *)ex_size);
	} else if (os_strcmp(argv[1], "tx") == 0) {
		if (os_strcmp(argv[2], "stb") == CLI_CAN_STB) {
			cli_select_tb = CLI_CAN_STB;
		} else {
			cli_select_tb = CLI_CAN_PTB;
		}
		ex_size = os_strtoul(argv[3], NULL, 10);
		cli_can_send((void *)ex_size);
	} else {
		CLI_LOGD("param err \r\n");
	}
}

static void can_filter_cfg(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	can_acc_filter_cmd_s cmd;
	if (argc < 4) {
		CLI_LOGD("param too few \r\n");
		cli_can_help();
		return;
	}
	cmd.aide = os_strtoul(argv[1], NULL, 10);;
	cmd.code = os_strtoul(argv[2], NULL, 16);
	cmd.mask = os_strtoul(argv[3], NULL, 16);
	cmd.seq = 0;
	cmd.onoff = DRIVER_ENABLE;
	bk_can_acc_filter_set(&cmd);
}

static void can_loop(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	beken_thread_t rcv_thread = NULL;
	int ret;
	uint32_t ex_size = 2;
	if (argc < 2) {
		CLI_LOGD("param too few \r\n");
		cli_can_help();
		return;
	}

	if (os_strcmp(argv[1], "i") == 0) {
		can_hal_set_lbmi(1);

	} else {
		can_hal_set_lbme(1);
	}

	ret = rtos_create_thread(&rcv_thread, 2, "can_rcv", cli_can_recevie, configMINIMAL_STACK_SIZE * 20, (void *)(ex_size + 5));
	if (ret != kNoErr) {
		CLI_LOGD("rtos_create_thread failed!!!\r\n");
		return;
	}

	cli_can_send((void *)ex_size);
	rtos_delay_milliseconds(1000);

	if(rcv_thread)
	{
		rtos_delete_thread(&rcv_thread);
	}

	if (os_strcmp(argv[1], "i") == 0) {
		can_hal_set_lbmi(0);

	} else {
		can_hal_set_lbme(0);
	}
}


static void cli_can_speed_cfg(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	can_bit_rate_e s_speed;
	can_bit_rate_e f_speed;
	if (argc < 3) {
		cli_can_help();
		return;
	}

	s_speed = os_strtoul(argv[1], NULL, 10);
	f_speed = os_strtoul(argv[2], NULL, 10);
	can_driver_bit_rate_config(s_speed, f_speed);
}

static void cli_can_statis(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 2) {
		cli_can_help();
		return;
	}

#if CONFIG_LIN_STATIS
	if (os_strcmp(argv[1], "dump") == 0) {
		can_statis_dump();
		CLI_LOGD("lin dump statis ok\r\n");
	} else if (os_strcmp(argv[1], "reset") == 0) {
		can_statis_init();
		CLI_LOGD("lin reset statis ok\r\n");
	}
#endif

	return;
}

static const struct cli_command s_can_commands[] = {
	{"can_transmit", "can_transmit {rx|tx} {rx size|stb|ptb} {tx szie}", cli_can_transmit},
	{"can_init", "can_init", cli_can_driver_init},
	{"can_exit", "can_exit", cli_can_driver_exit},
	{"can_filter_cfg", "can_filter_cfg {aid} {acode} {amask}", can_filter_cfg},
	{"can_loop", "can_loop {i|e}", can_loop},
	{"can_speed", "can_speed {s_spend} {f_speed}", cli_can_speed_cfg},
	{"can_statis", "can_statis {dump|reset}", cli_can_statis},
};
#define CAN_CMD_CNT (sizeof(s_can_commands) / sizeof(struct cli_command))

int bk_can_register_cli_test_feature(void)
{
	return cli_register_commands(s_can_commands, CAN_CMD_CNT);
}

