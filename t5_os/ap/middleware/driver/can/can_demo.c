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
#include <driver/trng.h>

#if (CONFIG_SYSTEM_CTRL)
#include "sys_driver.h"
#endif

#define CAN_STB_DEMO_SEND 1
#define CAN_STB_DEMO_RECEIVE 2

typedef struct {
	int op;
	void *param;
} can_stb_demo_msg_t;

typedef enum {
	CAN_STB_DEMO_SENDER,
	CAN_STB_DEMO_RECEIVER,
	CAN_STB_DEMO_MONITER,
	CAN_STB_DEMO_INTERFERE,
	CAN_STB_DEMO_INVALID,
} can_stb_demo_identity_e;

static void cli_can_help(void)
{
	CLI_LOGD("can_filter_demo {aid} {acode} {amask}");
	CLI_LOGD("can_speed_demo {s_spend} {f_speed}");
	CLI_LOGD("can_stb_demo1 [can_id] [sender|receiver|moniter] [start|stop]");

}

static void can_demo_filter_cfg(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	can_acc_filter_cmd_s cmd;
	if (argc < 4) {
		CLI_LOGD("param too few \r\n");
		cli_can_help();
		return;
	}
	cmd.aide = os_strtoul(argv[1], NULL, 10);
	cmd.code = os_strtoul(argv[2], NULL, 16);
	cmd.mask = os_strtoul(argv[3], NULL, 16);
	cmd.seq = 0;
	cmd.onoff = DRIVER_ENABLE;
	bk_can_acc_filter_set(&cmd);
}

static void can_demo_speed_cfg(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	can_bit_rate_e s_speed;
	can_bit_rate_e f_speed;
	if (argc < 3) {
		cli_can_help();
		return;
	}

	s_speed = os_strtoul(argv[1], NULL, 10);
	f_speed = os_strtoul(argv[2], NULL, 10);

	if (s_speed < CAN_BR_250K || s_speed > CAN_BR_5M || f_speed < CAN_BR_250K || f_speed > CAN_BR_5M) {
		CLI_LOGD("beyond configurable range!!!\r\n");
		return;
	}
	can_hal_set_reset(1);
	can_hal_bit_rate_config(s_speed, f_speed);
	can_hal_set_reset(0);

}

#define CAN_FD_STB_DEMO1_ENABLE 1

#if CAN_FD_STB_DEMO1_ENABLE

#define DEMO1_SEND_MAX_NUMBER 64
#define DEMO1_RECEIVE_DLC_C 1
#define DEMO1_RECEIVE_ID_C 4

typedef struct {
	beken_thread_t can_stb_demo_thread;
	beken_queue_t can_stb_demo_msg_que;
	uint32_t can_id;
	can_stb_demo_identity_e identity;
	uint8_t can_stb_demo_send_buf[DEMO1_SEND_MAX_NUMBER];
	uint8_t receive_dlc;
	uint8_t receive_id;
	uint8_t can_stb_demo_receive_buf[DEMO1_SEND_MAX_NUMBER+DEMO1_RECEIVE_DLC_C+DEMO1_RECEIVE_ID_C];
} can_fd_stb_demo;

static can_fd_stb_demo s_stb_demo1;

static bk_err_t can_fd_stb_demo1_send_msg(int op, void *param)
{
	bk_err_t ret;
	can_fd_stb_demo *demo1 = &s_stb_demo1;
	can_stb_demo_msg_t msg;

	msg.op = op;
	msg.param = param;

	if (demo1->can_stb_demo_msg_que) {
		ret = rtos_push_to_queue(&demo1->can_stb_demo_msg_que, &msg, BEKEN_NO_WAIT);
		if (kNoErr != ret) {
			return BK_FAIL;
		}

		return ret;
	}
	return BK_OK;
}

static void can_fd_stb_demo1_rx_cb(void *param)
{
	can_fd_stb_demo *demo1 = &s_stb_demo1;

	can_fd_stb_demo1_send_msg(CAN_STB_DEMO_RECEIVE, (void *)demo1);
}

static void can_fd_stb_demo1_tx_cb(void *param)
{
	can_fd_stb_demo *demo1 = &s_stb_demo1;

	switch (demo1->identity) {
		case CAN_STB_DEMO_SENDER:
		case CAN_STB_DEMO_RECEIVER:
		case CAN_STB_DEMO_MONITER:
			break;
		case CAN_STB_DEMO_INTERFERE:
			can_fd_stb_demo1_send_msg(CAN_STB_DEMO_SEND, (void *)demo1);
			break;
		case CAN_STB_DEMO_INVALID:
			break;
		default:
			break;
	}
}

static void can_fd_stb_demo1_random_number(can_fd_stb_demo *demo1)
{
	uint8_t *send_buf = demo1->can_stb_demo_send_buf;

	uint32_t random_1 = 0;

	for(uint32_t index = 0; index < DEMO1_SEND_MAX_NUMBER; (index += 4)) {
		random_1 = bk_rand();
		send_buf[index] = (random_1 >> 0) & 0xFF;
		send_buf[index + 1] = (random_1 >> 8) & 0xFF;
		send_buf[index + 2] = (random_1 >> 16) & 0xFF;
		send_buf[index + 3] = (random_1 >> 24) & 0xFF;
	}
}

static void can_fd_stb_demo1_send_random_data(can_fd_stb_demo *demo1)
{
	can_frame_s frame;
	uint8_t *send_buf = demo1->can_stb_demo_send_buf;
	uint8_t *receive_buf = demo1->can_stb_demo_receive_buf;

	switch (demo1->identity) {
		case CAN_STB_DEMO_SENDER:
			os_memset(send_buf, 0, sizeof(send_buf));
			os_memset(receive_buf, 0, sizeof(receive_buf));
			can_fd_stb_demo1_random_number(demo1);
			break;
		case CAN_STB_DEMO_RECEIVER:
			memcpy(&send_buf[0], &receive_buf[5], DEMO1_SEND_MAX_NUMBER);
			break;
		case CAN_STB_DEMO_MONITER:
			return;
			break;
		case CAN_STB_DEMO_INTERFERE:
			os_memset(send_buf, 0, sizeof(send_buf));
			os_memset(receive_buf, 0, sizeof(receive_buf));
			can_fd_stb_demo1_random_number(demo1);
			rtos_delay_milliseconds(200);
			break;
		case CAN_STB_DEMO_INVALID:
			return;
			break;
		default:
			CLI_LOGD("WARING IDENTITY FAIL!!!\r\n");
			return;
			break;
	}

	frame.tag.fdf = CAN_PROTO_FD;
	frame.tag.id = demo1->can_id;
	frame.tag.rtr = 0;
	frame.tag.ide = 1;
	frame.tag.brs = 1;
	frame.size = DEMO1_SEND_MAX_NUMBER;
	frame.data = send_buf;

	bk_can_send(&frame, 1000);
}

static void can_fd_stb_demo1_check_data(can_fd_stb_demo *demo1)
{
	uint8_t *send_buf = demo1->can_stb_demo_send_buf;
	uint8_t *receive_buf = demo1->can_stb_demo_receive_buf;
	uint32_t index = 0;
	uint32_t max_number = DEMO1_SEND_MAX_NUMBER;

	for (index = 0; index < max_number; index++) {
		if(send_buf[index] != receive_buf[index + 5]) {
			CLI_LOGD("====CAN DEMO1 FAIL==== send_buf[%02d]:0x%02x receive_buf[%02d]:0x%02x\r\n", index, send_buf[index], (index+5), receive_buf[index+5]);
		}
	}
}

static void can_fd_stb_demo1_receive_data(can_fd_stb_demo *demo1)
{
	uint32_t rec_size = 0;
	uint32_t all_size = (DEMO1_SEND_MAX_NUMBER+DEMO1_RECEIVE_DLC_C+DEMO1_RECEIVE_ID_C);
	uint8_t *receive_buf = demo1->can_stb_demo_receive_buf;

	bk_can_receive(receive_buf, all_size, &rec_size, 1000);
	if (rec_size) {
		CLI_LOGD("recev data %d\r\n", rec_size);
		demo1->receive_dlc = receive_buf[0];
		demo1->receive_id = (receive_buf[4] << 24) | \
			(receive_buf[3] << 16) | \
			(receive_buf[2] << 8) | \
			(receive_buf[1]);
		CLI_LOGD("id is 0x%02x %x-%x-%x-%x\r\n", demo1->receive_id, receive_buf[4], receive_buf[3], receive_buf[2], receive_buf[1]);
	}

	switch (demo1->identity) {
		case CAN_STB_DEMO_SENDER:
			can_fd_stb_demo1_check_data(demo1);
			rtos_delay_milliseconds(200);
			can_fd_stb_demo1_send_msg(CAN_STB_DEMO_SEND, (void *)demo1);
			break;
		case CAN_STB_DEMO_RECEIVER:
			can_fd_stb_demo1_send_msg(CAN_STB_DEMO_SEND, (void *)demo1);
			break;
		case CAN_STB_DEMO_MONITER:
			break;
		case CAN_STB_DEMO_INTERFERE:
			break;
		case CAN_STB_DEMO_INVALID:
			break;
		default:
			break;
	}

}

static void can_fd_stb_demo1_handler(void *param)
{
	can_fd_stb_demo *demo1 = param;
	bk_err_t ret;

	while (1) {
		can_stb_demo_msg_t msg;
		ret = rtos_pop_from_queue(&demo1->can_stb_demo_msg_que, &msg, BEKEN_WAIT_FOREVER);
		if (kNoErr == ret) {
			switch (msg.op) {
				case CAN_STB_DEMO_SEND:
					can_fd_stb_demo1_send_random_data(demo1);
					break;
				case CAN_STB_DEMO_RECEIVE:
					can_fd_stb_demo1_receive_data(demo1);
					break;
				default:
					break;
			}
		}
	}
}

static void can_fd_stb_demo1(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 2) {
		cli_can_help();
		return;
	}

	int ret;
	can_fd_stb_demo *demo1 = &s_stb_demo1;
	can_callback_des_t rx_cb;
	can_callback_des_t tx_cb;

	demo1->can_id = os_strtoul(argv[1], NULL, 16);

	if (os_strcmp(argv[2], "sender") == 0) {
		demo1->identity = CAN_STB_DEMO_SENDER;
	} else if (os_strcmp(argv[2], "receiver") == 0) {
		demo1->identity = CAN_STB_DEMO_RECEIVER;
	} else if (os_strcmp(argv[2], "moniter") == 0){
		demo1->identity = CAN_STB_DEMO_MONITER;
	} else if (os_strcmp(argv[2], "interfere") == 0){
		demo1->identity = CAN_STB_DEMO_INTERFERE;
	} else {
		demo1->identity = CAN_STB_DEMO_INVALID;
		return;
	}

	if (os_strcmp(argv[3], "start") == 0) {
		rx_cb.cb = can_fd_stb_demo1_rx_cb;
		rx_cb.param = demo1;

		tx_cb.cb= can_fd_stb_demo1_tx_cb;
		tx_cb.param = demo1;
		bk_can_register_isr_callback(&rx_cb, &tx_cb);

		ret = rtos_create_thread(&demo1->can_stb_demo_thread, 4, "can_demo1", can_fd_stb_demo1_handler, 1024, (void *)demo1);
		if (ret != kNoErr) {
			CLI_LOGD("rtos_create_thread failed!!!\r\n");
			return;
		}

		ret = rtos_init_queue(&demo1->can_stb_demo_msg_que,
						  "can_demo_queue",
						  sizeof(can_stb_demo_msg_t),
							  10);
		if (ret != kNoErr) {
			CLI_LOGD("ceate can demo message queue fail \r\n");
			return;
		}

		switch (demo1->identity) {
			case CAN_STB_DEMO_SENDER:
			case CAN_STB_DEMO_INTERFERE:
				can_fd_stb_demo1_send_msg(CAN_STB_DEMO_SEND, (void *)demo1);
				break;
			case CAN_STB_DEMO_RECEIVER:
			case CAN_STB_DEMO_MONITER:
			case CAN_STB_DEMO_INVALID:
				break;
			default:
				break;
		}

	} else if (os_strcmp(argv[3], "stop") == 0) {
		if(demo1->can_stb_demo_thread)
		{
			rtos_delete_thread(&demo1->can_stb_demo_thread);
		}
		if(demo1->can_stb_demo_msg_que) {
			rtos_deinit_queue(&demo1->can_stb_demo_msg_que);
			demo1->can_stb_demo_msg_que = NULL;
		}
		rx_cb.cb = NULL;
		rx_cb.param = NULL;

		tx_cb.cb= NULL;
		tx_cb.param = NULL;
		bk_can_register_isr_callback(&rx_cb, &tx_cb);
	} else {
		cli_can_help();
	}

}
#endif

#define CAN_20_STB_DEMO2_ENABLE 1

#if CAN_20_STB_DEMO2_ENABLE

#define DEMO2_SEND_MAX_NUMBER 8
#define DEMO2_RECEIVE_DLC_C 1
#define DEMO2_RECEIVE_ID_C 4

typedef struct {
	beken_thread_t can_20_demo_thread;
	beken_queue_t can_20_demo_msg_que;
	uint32_t can_id;
	can_stb_demo_identity_e identity;
	uint8_t can_stb_demo_send_buf[DEMO2_SEND_MAX_NUMBER];
	uint8_t receive_dlc;
	uint8_t receive_id;
	uint8_t can_stb_demo_receive_buf[DEMO2_RECEIVE_DLC_C+DEMO2_RECEIVE_DLC_C+DEMO2_RECEIVE_ID_C];
} can_20_stb_demo;

static can_20_stb_demo s_stb_demo2;

static bk_err_t can_20_stb_demo2_send_msg(int op, void *param)
{
	bk_err_t ret;
	can_20_stb_demo *demo2 = &s_stb_demo2;
	can_stb_demo_msg_t msg;

	msg.op = op;
	msg.param = param;

	if (demo2->can_20_demo_msg_que) {
		ret = rtos_push_to_queue(&demo2->can_20_demo_msg_que, &msg, BEKEN_NO_WAIT);
		if (kNoErr != ret) {
			return BK_FAIL;
		}

		return ret;
	}
	return BK_OK;
}

static void can_20_stb_demo2_rx_cb(void *param)
{
	can_20_stb_demo *demo2 = &s_stb_demo2;

	can_20_stb_demo2_send_msg(CAN_STB_DEMO_RECEIVE, (void *)demo2);
}

static void can_20_stb_demo2_tx_cb(void *param)
{

}

static void can_20_stb_demo2_random_number(can_20_stb_demo *demo2)
{
	uint8_t *send_buf = demo2->can_stb_demo_send_buf;

	uint32_t random_1 = 0;

	for(uint32_t index = 0; index < DEMO2_SEND_MAX_NUMBER; (index += 4)) {
		random_1 = bk_rand();
		send_buf[index] = (random_1 >> 0) & 0xFF;
		send_buf[index + 1] = (random_1 >> 8) & 0xFF;
		send_buf[index + 2] = (random_1 >> 16) & 0xFF;
		send_buf[index + 3] = (random_1 >> 24) & 0xFF;
	}
}

static void can_20_stb_demo2_send_random_data(can_20_stb_demo *demo2)
{
	can_frame_s frame;
	uint8_t *send_buf = demo2->can_stb_demo_send_buf;
	uint8_t *receive_buf = demo2->can_stb_demo_receive_buf;

	switch (demo2->identity) {
		case CAN_STB_DEMO_SENDER:
			os_memset(send_buf, 0, sizeof(send_buf));
			os_memset(receive_buf, 0, sizeof(receive_buf));
			can_20_stb_demo2_random_number(demo2);
			break;
		case CAN_STB_DEMO_RECEIVER:
			memcpy(&send_buf[0], &receive_buf[5], DEMO2_SEND_MAX_NUMBER);
			break;
		case CAN_STB_DEMO_MONITER:
			return;
			break;
		case CAN_STB_DEMO_INTERFERE:
			os_memset(send_buf, 0, sizeof(send_buf));
			os_memset(receive_buf, 0, sizeof(receive_buf));
			can_20_stb_demo2_random_number(demo2);
			rtos_delay_milliseconds(200);
			break;
		case CAN_STB_DEMO_INVALID:
			return;
			break;
		default:
			CLI_LOGD("WARING IDENTITY FAIL!!!\r\n");
			return;
			break;
	}

	frame.tag.fdf = CAN_PROTO_20;
	frame.tag.id = demo2->can_id;
	frame.tag.rtr = 0;
	frame.tag.ide = 0;
	frame.tag.brs = 0;
	frame.size = DEMO2_SEND_MAX_NUMBER;
	frame.data = send_buf;

	bk_can_send(&frame, 1000);
}

static void can_20_stb_demo2_check_data(can_20_stb_demo *demo2)
{
	uint8_t *send_buf = demo2->can_stb_demo_send_buf;
	uint8_t *receive_buf = demo2->can_stb_demo_receive_buf;
	uint32_t index = 0;
	uint32_t max_number = DEMO2_SEND_MAX_NUMBER;

	for (index = 0; index < max_number; index++) {
		if(send_buf[index] != receive_buf[index + 5]) {
			CLI_LOGD("====CAN DEMO1 FAIL==== send_buf[%02d]:0x%02x receive_buf[%02d]:0x%02x\r\n", index, send_buf[index], (index+5), receive_buf[index+5]);
		}
	}
}

static void can_20_stb_demo2_receive_data(can_20_stb_demo *demo2)
{
	uint32_t rec_size = 0;
	uint32_t all_size = (DEMO2_SEND_MAX_NUMBER+DEMO2_RECEIVE_DLC_C+DEMO2_RECEIVE_ID_C);
	uint8_t *receive_buf = demo2->can_stb_demo_receive_buf;

	bk_can_receive(receive_buf, all_size, &rec_size, 1000);
	if (rec_size) {
		CLI_LOGD("recev data %d\r\n", rec_size);
		demo2->receive_dlc = receive_buf[0];
		demo2->receive_id = (receive_buf[4] << 24) | \
			(receive_buf[3] << 16) | \
			(receive_buf[2] << 8) | \
			(receive_buf[1]);
		CLI_LOGD("id is 0x%02x\r\n", demo2->receive_id);
	}

	switch (demo2->identity) {
		case CAN_STB_DEMO_SENDER:
			can_20_stb_demo2_check_data(demo2);
			rtos_delay_milliseconds(200);
			can_20_stb_demo2_send_msg(CAN_STB_DEMO_SEND, (void *)demo2);
			break;
		case CAN_STB_DEMO_RECEIVER:
			can_20_stb_demo2_send_msg(CAN_STB_DEMO_SEND, (void *)demo2);
			break;
		default:
			break;
	}

}

static void can_20_demo2_handler(void *param)
{
	can_20_stb_demo *demo2 = param;
	bk_err_t ret;

	while (1) {
		can_stb_demo_msg_t msg;
		ret = rtos_pop_from_queue(&demo2->can_20_demo_msg_que, &msg, BEKEN_WAIT_FOREVER);
		if (kNoErr == ret) {
			switch (msg.op) {
				case CAN_STB_DEMO_SEND:
					can_20_stb_demo2_send_random_data(demo2);
					break;
				case CAN_STB_DEMO_RECEIVE:
					can_20_stb_demo2_receive_data(demo2);
					break;
				default:
					break;
			}
		}
	}
}

static void can_20_stb_demo2(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
	if (argc < 2) {
		cli_can_help();
		return;
	}

	int ret;
	can_20_stb_demo *demo2 = &s_stb_demo2;
	can_callback_des_t rx_cb;
	can_callback_des_t tx_cb;

	demo2->can_id = os_strtoul(argv[1], NULL, 16);

	if (os_strcmp(argv[2], "sender") == 0) {
		demo2->identity = CAN_STB_DEMO_SENDER;
	} else if (os_strcmp(argv[2], "receiver") == 0) {
		demo2->identity = CAN_STB_DEMO_RECEIVER;
	} else if (os_strcmp(argv[2], "moniter") == 0){
		demo2->identity = CAN_STB_DEMO_MONITER;
	} else if (os_strcmp(argv[2], "interfere") == 0){
		demo2->identity = CAN_STB_DEMO_INTERFERE;
	} else {
		demo2->identity = CAN_STB_DEMO_INVALID;
		return;
	}

	if (os_strcmp(argv[3], "start") == 0) {
		rx_cb.cb = can_20_stb_demo2_rx_cb;
		rx_cb.param = demo2;

		tx_cb.cb= can_20_stb_demo2_tx_cb;
		tx_cb.param = demo2;
		bk_can_register_isr_callback(&rx_cb, &tx_cb);

		ret = rtos_create_thread(&demo2->can_20_demo_thread, 4, "can_demo2", can_20_demo2_handler, 1024, (void *)demo2);
		if (ret != kNoErr) {
			CLI_LOGD("rtos_create_thread failed!!!\r\n");
			return;
		}

		ret = rtos_init_queue(&demo2->can_20_demo_msg_que,
						  "can_demo2_queue",
						  sizeof(can_stb_demo_msg_t),
							  10);
		if (ret != kNoErr) {
			CLI_LOGD("ceate can demo message queue fail \r\n");
			return;
		}

		switch (demo2->identity) {
			case CAN_STB_DEMO_SENDER:
				can_20_stb_demo2_send_msg(CAN_STB_DEMO_SEND, (void *)demo2);
				break;
			default:
				break;
		}

	} else if (os_strcmp(argv[3], "stop") == 0) {
		if(demo2->can_20_demo_thread)
		{
			rtos_delete_thread(&demo2->can_20_demo_thread);
		}
		if(demo2->can_20_demo_msg_que) {
			rtos_deinit_queue(&demo2->can_20_demo_msg_que);
			demo2->can_20_demo_msg_que = NULL;
		}
		rx_cb.cb = NULL;
		rx_cb.param = NULL;

		tx_cb.cb= NULL;
		tx_cb.param = NULL;
		bk_can_register_isr_callback(&rx_cb, &tx_cb);
	} else {
		cli_can_help();
	}

}
#endif

static const struct cli_command s_can_demo_commands[] = {
	{"can_filter_demo", "can_filter_demo {aid} {acode} {amask}", can_demo_filter_cfg},
	{"can_speed_demo", "can_speed_demo {s_spend} {f_speed}", can_demo_speed_cfg},
#if CAN_FD_STB_DEMO1_ENABLE
	{"can_stb_demo1", "can_demo1 [can_id] [sender/receiver] [start|stop]", can_fd_stb_demo1},
#endif
#if CAN_20_STB_DEMO2_ENABLE
	{"can_stb_demo2", "can_demo1 [can_id] [sender/receiver] [start|stop]", can_20_stb_demo2},
#endif
};
#define CAN_CMD_CNT (sizeof(s_can_demo_commands) / sizeof(struct cli_command))

int bk_can_register_cli_demo(void)
{
	return cli_register_commands(s_can_demo_commands, CAN_CMD_CNT);
}

