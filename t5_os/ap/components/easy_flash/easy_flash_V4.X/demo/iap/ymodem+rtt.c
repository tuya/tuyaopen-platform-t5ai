/*
 * This file is part of the EasyFlash Library.
 *
 * Copyright (c) 2015, Armink, <armink.ztl@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * 'Software'), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Function: It's a demo for EasyFlash IAP fucnction. You must use RT-Thread to run this demo.
 * Created on: 2015-07-06
 */


/**
 * IAP ���²��裺
 * 1����RT-Thread��Finsh�ն�������"update"��
 * 2����Ymodem���俪ʼʱ��ͨ��ymodem_on_begin�ص���������ʹ��ef_erase_bak_app�����������������洢��Ӧ�ã�
 * 3��ʹ��ef_write_data_to_bak���������յ����ݱ��浽Flash�С�������̽�����Ymodem�������ݵĻص�ymodem_on_data�У�
 * 4����������ɺ��޸Ļ�������"iap_need_copy_app"ֵΪ1����������"change iap_copy_app_size"ֵΪ������APP��С�����滷��������
 *    ������Ϊ�˱�֤�ڸ���ʱ���������´��ϵ���Ȼ���Զ��ָ����£�
 * 5������������Ӧ����Ӧ�ó�����ڣ�
 * 6���޸Ĳ����滷������"iap_need_copy_app"��"iap_copy_app_size"��ֵΪ0��
 *
 * IAP update step:
 * 1. Input "update" command in RT-Thread finsh terminal.
 * 2. It will use ef_erase_bak_app to erase backup application section when ymodem_on_begin.
 * 3. Save received data(ef_write_data_to_bak) to flash.
 *    This process is in Ymodem received data callback funciton ymodem_on_data.
 * 4. After received finish. Change and save the Env "iap_need_copy_app to" 1 and "change iap_copy_app_size" to downloaded size.
 *    Make sure if update process case power-down, the update will auto resume on next reboot.
 * 5. Erase and copy downloaded application to application entry.
 * 6. Change and save the Env "iap_need_copy_app" and "iap_copy_app_size" to 0.
 */

#include <rthw.h>
#include <rtthread.h>
#include "finsh.h"
#include "ymodem.h"
#include "easyflash.h"
#include "stdlib.h"

static uint32_t update_file_total_size, update_file_cur_size;
static enum rym_code ymodem_on_begin(struct rym_ctx *ctx, rt_uint8_t *buf, rt_size_t len)
{
	char *file_name, *file_size;

	/* calculate and store file size */
	file_name = (char *) &buf[0];
	file_size = (char *) &buf[rt_strlen(file_name) + 1];
	update_file_total_size = atol(file_size);
	update_file_cur_size = 0;

	/* erase backup section */
	if (ef_erase_bak_app(update_file_total_size)) {
		/* if erase fail then end session */
		return RYM_CODE_CAN;
	}

	return RYM_CODE_ACK;
}

static enum rym_code ymodem_on_data(struct rym_ctx *ctx, rt_uint8_t *buf, rt_size_t len)
{
	/* write data of application to backup section  */
	if (ef_write_data_to_bak(buf, len, &update_file_cur_size, update_file_total_size)) {
		/* if write fail then end session */
		return RYM_CODE_CAN;
	}
	return RYM_CODE_ACK;
}

/**
 * update command for RT-Thread finsh-msh command.
 */
void update(uint8_t argc, char **argv)
{
	uint16_t size;
	char *recv_buff, c_file_size[11] = {0};
	struct rym_ctx rctx;

	BK_LOGD(NULL, "Please select a update file and use Ymodem to send.\r\n");

	if (!rym_recv_on_device(&rctx, serial_get_device(),
							RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_INT_RX, ymodem_on_begin, ymodem_on_data, NULL,
							bk_get_ticks_per_second())) {
		/* wait some time for terminal response finish */
		rtos_delay_milliseconds(bk_get_ticks_per_second());
		/* set need copy application from backup section flag is 1, backup application length */
		ef_set_env("iap_need_copy_app", "1");
		rt_sprintf(c_file_size, "%ld", update_file_total_size);
		ef_set_env("iap_copy_app_size", c_file_size);
		ef_save_env();
		/* copy downloaded application to application entry */
		if (ef_erase_user_app(iap_get_app_addr(), update_file_total_size)
			|| ef_copy_app_from_bak(iap_get_app_addr(), update_file_total_size))
			BK_LOGD(NULL, "Update user app fail.\n");
		else
			BK_LOGD(NULL, "Update user app success.\n");
		/* clean need copy application from backup section flag */
		ef_set_env("iap_need_copy_app", "0");
		ef_set_env("iap_copy_app_size", "0");
		ef_save_env();
	} else {
		/* wait some time for terminal response finish */
		rtos_delay_milliseconds(bk_get_ticks_per_second());
		BK_LOGD(NULL, "Update user app fail.\n");
	}
}
MSH_CMD_EXPORT(update, Update user application);
