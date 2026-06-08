/**
 ****************************************************************************************
 *
 * @file bk_modem_at_cmd.c
 *
 * @brief AT Command Processing Module
 *        This module implements the AT command processing functionality for communication
 *        with the modem, including command sending, response parsing, and timeout handling.
 *
 ****************************************************************************************
 */
#include <os/mem.h>
#include "sys_rtos.h"
#include <os/os.h>
#include <common/bk_kernel_err.h>
#include "bk_sys_ctrl.h"
#include <os/str.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include "bk_modem_main.h"
#include "bk_modem_at_cmd.h"
#include "bk_modem_dte.h"
#include "bk_modem_dce.h"

/* Maximum length definitions for AT command and response buffers */
#define AT_CMD_LEN_MAX (128)        /* Maximum length of AT command buffer */
#define AT_RSP_LEN_MAX (256)        /* Maximum length of AT response buffer */

/* AT command response timeout configurations */
#define AT_RSP_TIMER_MS_ONCE (200)   /* Time interval for each timeout check (in milliseconds) */
#define AT_RSP_TIMER_MS_TOTAL (5000)/* Total timeout duration for AT command response (in milliseconds) */

/* Global variables for AT command processing */
static beken_semaphore_t g_modem_at_semaphore = NULL;  /* Semaphore for AT command synchronization */
beken2_timer_t func_proc = {0};                       /* Timer for AT command timeout handling */
uint8_t g_modem_at_cmd_buf[AT_CMD_LEN_MAX];           /* Buffer for storing AT commands */
uint8_t g_modem_at_rsp_buf[AT_RSP_LEN_MAX];           /* Buffer for storing AT command responses */
uint8_t g_modem_at_rsp_segment_cnt = 0;               /* Counter for response segments */
uint32_t g_modem_at_rsp_len = 0;                      /* Length of received response */
uint32_t g_modem_at_rsp_timer_ms_to = AT_RSP_TIMER_MS_TOTAL; /* Current timeout value */

extern struct bk_modem_dce_pdp_ctx_s dce_pdp_ctx;

/**
 * @brief Parse arguments from AT command response
 * @param rsp_buf Pointer to the response buffer
 * @param resp_expr Format string for parsing
 * @return Number of successfully parsed arguments
 *        This function uses vsscanf to parse arguments from the AT command response
 *        according to the specified format string.
 */
int bk_modem_at_rsp_parse_args(char *rsp_buf, const char *resp_expr, ...)
{
	va_list args;
	int resp_args_num = 0;
	va_start(args, resp_expr);
	resp_args_num = vsscanf(rsp_buf, resp_expr, args);
	va_end(args);

	return resp_args_num;
}

/**
 * @brief Analyze AT command response
 * @param cmd Pointer to the AT command sent
 * @param resp Pointer to the received response
 * @return BK_OK if response indicates success, BK_FAIL otherwise
 *        This function checks if the AT command response contains an OK or ERROR indication,
 *        with special handling for certain commands like CONNECT and ATO.
 */
static bk_err_t bk_modem_at_rsp_analysis(uint8_t *cmd,uint8_t *resp)
{
	if (NULL != os_strstr((const char *)resp, AT_RSP_OK))
	{
		if (0 == os_strcmp((const char *)cmd, AT_CEREG))
		{
			if (!((NULL != os_strstr((const char *)resp, AT_RSP_CEREG1))
				|| (NULL != os_strstr((const char *)resp, AT_RSP_CEREG5))))
			{
				BK_MODEM_LOGI("at_rsp_analysis: rsp is fail, resp %s\r\n", resp);
				return BK_FAIL;
			}
		}
		BK_MODEM_LOGI("at_rsp_analysis: rsp is ok, cmd %s\r\n", cmd);
		return BK_OK;
	}
	else
	{
		if(((0 == os_strcmp((const char *)cmd, AT_CONNECT_CMD)) && (NULL != os_strstr((const char *)resp, AT_RSP_CONNECT)))||
			((0 == os_strcmp((const char *)cmd, ATO)) && (NULL != os_strstr((const char *)resp, AT_RSP_CONNECT)))||
			((0 == os_strcmp((const char *)cmd, AT_CBC)) && (NULL != os_strstr((const char *)resp, AT_CBC))))
		{
			return BK_OK;
		}
		BK_MODEM_LOGI("at_rsp_analysis: rsp is fail, resp %s\r\n", resp);
		return BK_FAIL;
	}
}

/**
 * @brief AT command timeout callback function
 * @param larg Left argument (unused)
 * @param rarg Right argument (unused)
 *        This function is called periodically to check if an AT command response
 *        has been received within the timeout period. If the timeout is reached,
 *        it signals the semaphore to unblock the command sender.
 */
static void bk_modem_at_timeout_cb(void* larg, void* rarg)
{
	static uint8_t timer_cnt = 0;

	if (timer_cnt * AT_RSP_TIMER_MS_ONCE >= g_modem_at_rsp_timer_ms_to)
		BK_MODEM_LOGI("AT command_timer is too long\r\n");

	if (g_modem_at_semaphore == NULL)
	{
		BK_MODEM_LOGI("at_semaphore is deinited.\r\n");
		timer_cnt = 0;
		return;
	}

	if ((g_modem_at_rsp_segment_cnt < 1) && (timer_cnt * AT_RSP_TIMER_MS_ONCE < g_modem_at_rsp_timer_ms_to))
	{
		timer_cnt ++;
		rtos_start_oneshot_timer(&func_proc);
		return;
	}

	timer_cnt = 0;

	int ret = rtos_set_semaphore(&g_modem_at_semaphore);
	if (ret)
	{
		BK_MODEM_LOGI("rtos_set_semaphore fail\r\n");
	}
	return;
}

/**
 * @brief Receive AT command response data
 * @param resp Pointer to the received response data
 * @param len Length of the response data
 *        This function accumulates received AT command response segments
 *        into the global response buffer, with bounds checking to prevent overflow.
 */
void bk_modem_at_rcv_resp(const char *resp,uint32_t len)
{
	if(len>=AT_RSP_LEN_MAX)
	{
		BK_MODEM_LOGI("rcv len err len %\r\n",len);
		return;
	}

	if (g_modem_at_rsp_segment_cnt > 3)
	{
		BK_MODEM_LOGI("rcv cnt is more than 3. len is %d, %d \r\n", g_modem_at_rsp_len, len);
		return;
	}

	if (g_modem_at_rsp_len + len > AT_RSP_LEN_MAX)
	{
		BK_MODEM_LOGI("len is %d now, rcv len %d, total len is more than 256. cnt is %d \r\n",
			g_modem_at_rsp_len, len, g_modem_at_rsp_segment_cnt);
		return;
	}

	g_modem_at_rsp_segment_cnt ++;
	os_memcpy(g_modem_at_rsp_buf+g_modem_at_rsp_len,resp,len);
	g_modem_at_rsp_len += len;
}

/**
 * @brief Send AT command to modem
 * @param cmd AT command string to send
 * @param max_retry Maximum number of retry attempts
 * @param timeout Timeout duration in milliseconds
 * @return BK_OK if command sent successfully and response received, BK_FAIL otherwise
 *        This function sends an AT command to the modem, waits for a response
 *        with timeout handling, and retries up to max_retry times if necessary.
 */
bk_err_t bk_modem_at_cmd_send(const char *cmd, uint8_t max_retry, uint32_t timeout)
{
	bk_err_t ret = BK_FAIL;
	bk_err_t ret_value  = BK_FAIL;
	//rt_device_t dev = NULL;
	uint32_t len = 0;
	//uint32_t write_len = 0;
	uint8_t retry = max_retry;

	if ((NULL == cmd) || (max_retry == 0))
	{
		BK_MODEM_LOGI("cmd pointer is null!\r\n");
		return BK_FAIL;
	}

	// CMD
	uint8_t at_cmd_buf[AT_CMD_LEN_MAX];
	len = os_strlen(cmd);
	os_memset(at_cmd_buf, 0x0, sizeof(at_cmd_buf));
	os_memcpy(at_cmd_buf, cmd, len);

	BK_MODEM_LOGI("at cmd send: len=%d, cmd=%s\r\n", len, at_cmd_buf);

	while (retry--)
	{
		BK_MODEM_LOGI("modem_device_write: %d, %d\r\n", retry, bk_modem_env.comm_if);
		os_memset(g_modem_at_rsp_buf, 0x0, sizeof(g_modem_at_rsp_buf));
		g_modem_at_rsp_segment_cnt = 0;
		g_modem_at_rsp_len = 0;
	    
		g_modem_at_rsp_timer_ms_to = timeout;
		rtos_start_oneshot_timer(&func_proc);

		//:send at cmd to uart or usb
		if (bk_modem_env.comm_if == USB_IF)
		{
			bk_modem_dte_send_data(len, at_cmd_buf, PPP_CMD_MODE);
		}
		else if (bk_modem_env.comm_if == UART_IF)
		{
			bk_modem_dte_send_data_uart(len, at_cmd_buf, AT_CMD_MODE);
		}

		ret = rtos_get_semaphore(&g_modem_at_semaphore, BEKEN_WAIT_FOREVER);
		if (ret) 
		{
			BK_MODEM_LOGI("semaphore failed\r\n");
			break;
		}
		if(g_modem_at_rsp_len == 0)
		{
			BK_MODEM_LOGI("len is 0, no handle\r\n");
			break;
		}

		if (BK_OK == ret)
		{
			if (BK_OK == bk_modem_at_rsp_analysis(at_cmd_buf,g_modem_at_rsp_buf))
			{
				ret_value = BK_OK;
				break;
			}
			else
			{
				BK_MODEM_LOGI("at rsp analysis fail!\r\n");
				continue;
			}
		}
		else
		{
			BK_MODEM_LOGI("recv fail[%d]!\r\n", ret);
			continue;
		}
	}

	BK_MODEM_LOGI("at_cmd_send end, retry=%d, ret_value=%d!\r\n", retry, ret_value);
	return ret_value;
}


/**
 * @brief Check if modem is ready by sending AT command
 * @return BK_OK if modem responds, BK_FAIL otherwise
 *        This command is used to check if the modem is ready to receive commands.
 */
bk_err_t bk_modem_at_ready(void)
{
	if (BK_OK == bk_modem_at_cmd_send(AT, 3, 5000))
	{
		BK_MODEM_LOGI("AT, rsp:%s\r\n",g_modem_at_rsp_buf);
		return BK_OK;
	}
	else
	{
		BK_MODEM_LOGI("at_cmd_send fail!,AT\r\n");
		return BK_FAIL;
	}
}

bk_err_t bk_modem_at_change_baudrate(uint32_t baudrate)
{
	char command[AT_CMD_LEN_MAX];
	sprintf(command, "%s%u\r\n", AT_IPR, baudrate);
	if (BK_OK == bk_modem_at_cmd_send(command, 3, 5000))
	{
		BK_MODEM_LOGI("AT+IPR=%u, rsp:%s\r\n", baudrate, g_modem_at_rsp_buf);
		return BK_OK;
	}
	else
	{
		BK_MODEM_LOGI("at_cmd_send fail!, AT+IPR=%u\r\n", baudrate);
		return BK_FAIL;
	}
}

/**
 * @brief Get packet service registration status
 * @return BK_OK if command succeeds, BK_FAIL otherwise
 *        This function sends the AT+CGREG command to check the network registration status.
 */
bk_err_t bk_modem_at_get_ps_reg(void)
{
	if (BK_OK == bk_modem_at_cmd_send(AT_CEREG, 3, 5000))
	{
		BK_MODEM_LOGI("AT_CEREG, rsp:%s\r\n",g_modem_at_rsp_buf);
		//bk_modem_at_rsp_parse_args
		return BK_OK;
	}
	else
	{
		BK_MODEM_LOGI("at_cmd_send fail!,AT\r\n");
		return BK_FAIL;
	}
}

/**
 * @brief Get current operator name
 * @return BK_OK if command succeeds, BK_FAIL otherwise
 *        This function sends the AT+COPS command to retrieve the current mobile network operator name.
 */
bk_err_t bk_modem_at_get_operator_name(void)
{
	if (BK_OK == bk_modem_at_cmd_send(AT_COPS, 3, 5000))
	{
		BK_MODEM_LOGI("AT_COPS, rsp:%s\r\n",g_modem_at_rsp_buf);
		//bk_modem_at_rsp_parse_args
		return BK_OK;
	}
	else
	{
		BK_MODEM_LOGI("at_cmd_send fail!,AT\r\n");
		return BK_FAIL;
	}
}

/**
 * @brief Check SIM card PIN status
 * @return BK_OK if SIM is ready (no PIN required or PIN entered), BK_FAIL otherwise
 *        This function checks if the module waits for entering the PIN code.
 */
bk_err_t bk_modem_at_cpin(void)
{
	if (BK_OK == bk_modem_at_cmd_send(AT_CPIN, 3, 5000))
	{
		BK_MODEM_LOGI("AT_CPIN, rsp:%s\r\n",g_modem_at_rsp_buf);
		if (NULL != os_strstr((const char *)g_modem_at_rsp_buf, AT_RSP_CPIN))
			return BK_OK;
		else
			return BK_FAIL;
	}
	else
	{
		BK_MODEM_LOGI("at_cmd_send fail!,AT\r\n");
		return BK_FAIL;
	}
}

/**
 * @brief Get signal strength
 * @return BK_OK if command succeeds, BK_FAIL otherwise
 *        This function sends the AT+CSQ command to retrieve the current signal strength indicator (RSSI) and bit error rate (BER).
 */
bk_err_t bk_modem_at_csq(void)
{
	int rssi = 0;
	int ber = 0;
	char *ptr, *token, *saveptr, *endptr;

	if (BK_OK == bk_modem_at_cmd_send(AT_CSQ, 3, 5000))
	{
		BK_MODEM_LOGI("AT_CSQ, rsp:%s\r\n",g_modem_at_rsp_buf);

		ptr = os_strstr((char *)g_modem_at_rsp_buf, "\r\n+CSQ:");
		if (NULL == ptr) {
			BK_MODEM_LOGI("parse AT_CSQ response fail!\r\n");
			return BK_FAIL;
		}

		ptr += 8; // offset \r\n+CSQ: 
		token = strtok_r(ptr, ",", &saveptr);
		if (NULL != token) {
			rssi = (int)strtol(token, &endptr, 0);
			token = strtok_r(NULL, ",", &saveptr);
		}
		if (NULL != token) {
			ber = (int)strtol(token, &endptr, 0);
		}
		ber = ber; //avoid complie warning

		dce_pdp_ctx.rssi = -112 + rssi * 2;
		BK_MODEM_LOGI("at_csq rssi %d, cal %d\r\n", rssi, dce_pdp_ctx.rssi);
		if (BK_MODEM_INVALID_RSSI_VAL == rssi) {
			BK_MODEM_LOGI("at_csq no signal\r\n");
			return BK_FAIL;
		}

		return BK_OK;
	}
	else
	{
		BK_MODEM_LOGI("at_cmd_send fail!,AT\r\n");
		dce_pdp_ctx.rssi = rssi;
		return BK_FAIL;
	}
}

/**
 * @brief Define PDP context
 * @param cid Context ID
 * @param type PDP type (e.g., "IP")
 * @param apn Access Point Name
 * @return BK_OK if command succeeds, BK_FAIL otherwise
 *        This function configures a Packet Data Protocol (PDP) context with the specified parameters.
 */
bk_err_t bk_modem_at_cgdcont(uint8_t cid,char *type,char *apn)
{
	char command[AT_CMD_LEN_MAX];
	sprintf(command,"AT+CGDCONT=%d,\"%s\",\"%s\"\r\n", cid, type, apn);
	if (BK_OK == bk_modem_at_cmd_send(command, 3, 5000))
	{
		BK_MODEM_LOGI("AT+CGDCONT, rsp:%s\r\n",g_modem_at_rsp_buf);
		return BK_OK;
	}
	else
	{
		BK_MODEM_LOGI("at_cmd_send fail!,AT+CGDCONT\r\n");
		return BK_FAIL;
	}
}

/**
 * @brief Check PDP context configuration
 * @return BK_OK if command succeeds, BK_FAIL otherwise
 *        This function queries the current PDP context configuration.
 */
bk_err_t bk_modem_at_cgdcont_check(void)
{
	if (BK_OK == bk_modem_at_cmd_send(AT_CGDCONT, 3, 5000))
	{
		BK_MODEM_LOGI("AT+CGDCONT, rsp:%s\r\n",g_modem_at_rsp_buf);
		return BK_OK;
	}
	else
	{
		BK_MODEM_LOGI("at_cmd_send fail!,AT+CGDCONT\r\n");
		return BK_FAIL;
	}
}


bk_err_t bk_modem_at_ccid(void)
{
	char *ptr = NULL;

    // TODO 获取蜂窝类型，发送ECICCID 或者 CCID指令
	if (BK_OK == bk_modem_at_cmd_send(AT_ECICCID, 3, 5000))
	{
		BK_MODEM_LOGI("AT+CCID, rsp:%s\r\n",g_modem_at_rsp_buf);
		if ((ptr = os_strstr((char *)g_modem_at_rsp_buf, "\r\n+ECICCID: ")) != NULL)
		{
			ptr += 12;
		}
		else if ((ptr = os_strstr((char *)g_modem_at_rsp_buf, "\r\n+CCID: ")) != NULL)
		{
			ptr += 9;
		}
		if (ptr != NULL)
		{
			os_strncpy((char *)dce_pdp_ctx.cid, ptr, BK_MODEM_DCE_CID_LEN);
		}
		return BK_OK;
	}
	else
	{
		BK_MODEM_LOGI("at_cmd_send fail!,AT+CCID\r\n");
		return BK_FAIL;
	}
}

bk_err_t bk_modem_at_cbc(void)
{
	char *ptr, *token, *saveptr, *endptr;

	if (BK_OK != bk_modem_at_cmd_send(AT_CBC, 3, 5000))	{
		BK_MODEM_LOGI("at_cmd_send fail!,AT+CBC\r\n");
		return BK_FAIL;
	}

	BK_MODEM_LOGI("AT+CBC, rsp:%s\r\n",g_modem_at_rsp_buf);

	ptr = os_strstr((char *)g_modem_at_rsp_buf, "\r\n+CBC:");
	if (NULL == ptr) {
		BK_MODEM_LOGI("parse AT_CBC response fail!\r\n");
		return BK_FAIL;
	}

	ptr += 8; // offset \r\n+CBC: 
	token = strtok_r(ptr, ",", &saveptr);
	if (NULL != token) {
		dce_pdp_ctx.volt = (uint32_t)strtol(token, &endptr, 0);
	} else {
		dce_pdp_ctx.volt = 0;
	}

	return BK_OK;
}

/**
 * @brief Enter command mode from data mode
 * @return BK_OK if command succeeds, BK_FAIL otherwise
 *        This function sends the escape sequence to switch from data mode to command mode.
 */
bk_err_t bk_modem_at_enter_cmd_mode(void)
{
	if (BK_OK == bk_modem_at_cmd_send(AT_CHANGE_TO_AT_MODE, 3, 5000))
	{
		BK_MODEM_LOGI("+++, rsp:%s\r\n",g_modem_at_rsp_buf);
		return BK_OK;
	}
	else
	{
		BK_MODEM_LOGI("at_cmd_send fail!,+++\r\n");
		return BK_FAIL;
	}
}

/**
 * @brief Establish PPP connection
 * @return BK_OK if connection established, BK_FAIL otherwise
 *        This function sends the ATD*99# command to establish a PPP data connection.
 */
bk_err_t bk_modem_at_ppp_connect(void)
{
	if (BK_OK == bk_modem_at_cmd_send(AT_CONNECT_CMD, 3, 5000))
	{
		BK_MODEM_LOGI("ATD*99#, rsp:%s\r\n",g_modem_at_rsp_buf);
		return BK_OK;
	}
	else
	{
		BK_MODEM_LOGI("at_cmd_send fail!,ATD*99#\r\n");
		return BK_FAIL;
	}
}

#if 0
// Set command is used to set plat configure, if set parameter error, +CME ERROR: <err> is returned.
// Read command returns the current plat configure setting.
//Parameter
//	<mode>	String type
//		usbCtrl	Set usb control mode
//		usbNet	Set usb network interface type to RNDIS or ECM
//	<value> Integer type
//For usbCtrl, the values range is from 0 to 2
//0: usb is enabled and initialized, RNDIS is enumerated
//1: usb is enabled and initialized, RNDIS is not enumerated
//2: usb is disabled and not initialized
//
//For usbNet, the values range is from 0 to 1
//0: RNDIS network interface
//1: ECM network interface
bk_err_t bk_modem_at_set_plat_configure(void)
{
	if (BK_OK == bk_modem_at_cmd_send(AT_ECPCFG, 3, 5000))
	{
		BK_MODEM_LOGI("AT_ECPCFG, rsp:%s\r\n",g_modem_at_rsp_buf);
		return BK_OK;
	}
	else
	{
		BK_MODEM_LOGI("at_cmd_send fail!, AT_ECPCFG\r\n");
		return BK_FAIL;
	}
}

//The command set the network adapter parameter configuration. Set <nat> to enable or disable NAT (Network Address 
//Translation). When NAT is enable, configure local host IP address by <host_addr> if set or default 
//address ¡°192.168.10.2¡±. When NAT is disable, configure global IP address allocated by LTE network. Set 
//<pppauthselect> to select the PPP authentication parameters. When <pppauthselect> is set to 0, the authentication
//parameters are pre-defined by AT+CGAUTH. When <pppauthselect> is set to 1, the authentication parameters are 
//defined in PPP LCP procedure.
//AT+ECNETCFG=¡°nat¡±,<nat>[,<host_addr>]
//AT+ECNETCFG=¡°pppauthselect¡±,<pppauthselect>

//Parameter
//	<nat> Integer type
//		0 Disable network address translation
//		1 Enable network address translation
//	<host_addr>	String type
//		Local host IP address supported IPv4 type only
//			Note:
//		a) Supported values: ¡°192.168.a.b¡±, a:0-255, b:2-254
//		b) Default value: ¡°192.168.10.2¡±
//	<pppauthselect> Integer type
//		0 The PPP authentication parameters are pre-defined by AT+CGAUTH. The default value is 0.
//		1 The PPP authentication parameters are defined in PPP LCP procedure.
bk_err_t bk_modem_at_set_network_adapter_parameter_configuration(void)
{
	if (BK_OK == bk_modem_at_cmd_send(AT_ECNETCFG, 3, 5000))
	{
		BK_MODEM_LOGI("AT_ECNETCFG, rsp:%s\r\n",g_modem_at_rsp_buf);
		return BK_OK;
	}
	else
	{
		BK_MODEM_LOGI("at_cmd_send fail!, AT_ECNETCFG\r\n");
		return BK_FAIL;
	}
}

//The command set data path control for the network adapter. Control data path open by setting <op> to bind LWIP to 
//PDN context with <cid>. Control data path close by setting <op> to unbind LWIP to PDN context with <cid>.
//	AT+ECNETDEVCTL=<op>,<cid>,[,<urc_en>]
//Parameter
//	<op>		Integer type; specifies device control option.
//			0 Unbind cid for LWIP
//			1 Bind cid for LWIP once and no rebind if re-activate PDN context with same cid after deactivation.
//			2 Bind cid for LWIP and rebind if re-activate PDN context with the same cid after deactivation.
//			3 Auto dial and bind cid when power on, saved in NVM after power down.
//	<cid>		Integer type; specifies a particular non secondary PDP context definition.
//			<cid> values of 1-15 are supported.
//	<urc_en>	Integer type; specifies whether report URC +ECNETDEVCTL:<state>
//			0 Disable URC +ECNETDEVCTL:<state>
//			1 Enable URC +ECNETDEVCTL:<state>
//	<state>		Integer type; specifies cid bound state.
//			0 Bind cid for LWIP failure
//			1 Bind cid for LWIP success
//Example
//		AT+ECNETDEVCTL?
//		+ECNETDEVCTL: 0,0,0,0
//		OK
//		AT+ECDNSCFG=1,1,1
//		OK
bk_err_t bk_modem_at_set_data_path_control(void)
{
	if (BK_OK == bk_modem_at_cmd_send(AT_ECNETDEVCTL, 3, 5000))
	{
		BK_MODEM_LOGI("AT_ECNETDEVCTL, rsp:%s\r\n",g_modem_at_rsp_buf);
		return BK_OK;
	}
	else
	{
		BK_MODEM_LOGI("at_cmd_send fail!, AT_ECNETDEVCTL\r\n");
		return BK_FAIL;
	}
}

//This command causes the MT to perform whatever actions are necessary to establish communication between the TE and 
//the external PDN..
//	ATD*<GPRS_SC>[*[<called_address>][*[<L2P>][*[<cid>]]]]#
//Parameter
//<GPRS_SC>		String type. GPRS Service Code
//			99 identifies a request to use the Packet Domain service
//<called_address>	String type. The called party in the address space applicable to the PDP
//			Note: This item is currently not supported
//<L2P>			String type. The layer 2 protocol to be used PPP or 1
//<cid>			Integer type. Specifies a particular PDP context definition 1-15.
//Example
//		ATD*99#
//		CONNECT

bk_err_t bk_modem_at_atd(void)
{
	if (BK_OK == bk_modem_at_cmd_send(ATD, 3, 5000))
	{
		BK_MODEM_LOGI("ATD, rsp:%s\r\n",g_modem_at_rsp_buf);
		return BK_OK;
	}
	else
	{
		BK_MODEM_LOGI("at_cmd_send fail!, ATD\r\n");
		return BK_FAIL;
	}
}

//This command causes the DCE to return to online data state and issue a CONNECT result code.
//	ATO[<value>]
//Parameter
//0		Return to online data state from online command state;
//Example:	ATO0
//		ATO
bk_err_t bk_modem_at_get_dce_data_state(void)
{
	if (BK_OK == bk_modem_at_cmd_send(ATO, 3, 5000))
	{
		BK_MODEM_LOGI("ATO, rsp:%s\r\n",g_modem_at_rsp_buf);
		return BK_OK;
	}
	else
	{
		BK_MODEM_LOGI("at_cmd_send fail!, ATO\r\n");
		return BK_FAIL;
	}
}

// This command controls the Circuit 109 behavior of UE¡¯s DCD (data carrier detection) line.
//Parameter
//0		DCD function is always ON; The default value is 0.
//1		DCD function is ON only in the presence of data carrier;
//Example:	AT&C1
bk_err_t bk_modem_at_control_dcd(void)
{
	if (BK_OK == bk_modem_at_cmd_send(AT_C, 3, 5000))
	{
		BK_MODEM_LOGI("AT_C, rsp:%s\r\n",g_modem_at_rsp_buf);
		return BK_OK;
	}
	else
	{
		BK_MODEM_LOGI("at_cmd_send fail!, AT_C\r\n");
		return BK_FAIL;
	}
}

//This command determines how the UE responds when circuit 108/2 (DTR) is changed from ON to the OFF condition 
//during online data state.
//Parameter:
//0		Ignore DTR;
//1		ON->OFF on DTR: Enter online command state while the call remains connected;
//2	ON->OFF on DTR: Disconnect the call, and change to command mode; The default value is 2.
//Example:	AT&D2
bk_err_t bk_modem_at_change_ue_resp_mode(void)
{
	if (BK_OK == bk_modem_at_cmd_send(AT_D, 3, 5000))
	{
		BK_MODEM_LOGI("AT_D, rsp:%s\r\n",g_modem_at_rsp_buf);
		return BK_OK;
	}
	else
	{
		BK_MODEM_LOGI("at_cmd_send fail!, AT_D\r\n");
		return BK_FAIL;
	}
}
#endif

/**
 * @brief Disconnect the current call
 * @return BK_OK if disconnection succeeds, BK_FAIL otherwise
 *        This command instructs the DCE to disconnect from the line and terminate any call in progress.
 */
bk_err_t bk_modem_at_disconnect(void)
{
	if (BK_OK == bk_modem_at_cmd_send(ATH, 3, 5000))
	{
		BK_MODEM_LOGI("ATH, rsp:%s\r\n",g_modem_at_rsp_buf);
		return BK_OK;
	}
	else
	{
		BK_MODEM_LOGI("at_cmd_send fail!, ATH\r\n");
		return BK_FAIL;
	}
}

/**
 * @brief Set mobile functionality level
 * @param value Functionality level (0: minimum functionality, 1: full functionality)
 * @return BK_OK if command succeeds, BK_FAIL otherwise
 *        This function sets the mobile equipment's functionality level.
 */
bk_err_t bk_modem_at_cfun(uint8_t value)
{
	char *cmd;
	if (value == 0)
		cmd = AT_CFUN_0;
	else if (value == 1)
		cmd = AT_CFUN_1;
	else
		return BK_FAIL;

	if (BK_OK == bk_modem_at_cmd_send(cmd, 3, 5000))
	{
		BK_MODEM_LOGI("%s, rsp:%s\r\n",cmd,g_modem_at_rsp_buf);
		return BK_OK;
	}
	else
	{
		BK_MODEM_LOGI("at_cmd_send fail!, %s\r\n",cmd);
		return BK_FAIL;
	}
    
	return BK_FAIL;
}


/**
 * @brief Initialize AT command processing module
 * @return BK_OK if initialization succeeds, BK_FAIL otherwise
 *        This function initializes the semaphore and timer for AT command processing.
 */
bk_err_t bk_modem_at_init(void)
{
	bk_err_t ret = BK_FAIL;
	if (g_modem_at_semaphore != NULL)
	{
		BK_MODEM_LOGI("sem already exist\r\n");
		return BK_FAIL;
	}
	ret = rtos_init_semaphore(&g_modem_at_semaphore, 32);
	if (kNoErr != ret)
	{
		BK_MODEM_LOGI("sem init fail[%d]!\r\n", ret);
		return BK_FAIL;
	}

	ret = rtos_init_oneshot_timer(&func_proc,AT_RSP_TIMER_MS_ONCE,bk_modem_at_timeout_cb,NULL,NULL);
	if(ret != BK_OK){
		BK_MODEM_LOGI("init timer failed\r\n");
		return BK_FAIL;
	}
	return ret;
}

/**
 * @brief Deinitialize AT command processing module
 * @return BK_OK if deinitialization succeeds, BK_FAIL otherwise
 *        This function cleans up the semaphore and timer used for AT command processing.
 */
bk_err_t bk_modem_at_dinit(void)
{
	bk_err_t ret = BK_FAIL;
	if (g_modem_at_semaphore != NULL)
	{
		ret = rtos_deinit_semaphore(&g_modem_at_semaphore);
		g_modem_at_semaphore = NULL;
		if (kNoErr != ret)
		{
			BK_MODEM_LOGI("sem dinit fail[%d]!\r\n", ret);
			return BK_FAIL;
		}
	}

	if(rtos_is_oneshot_timer_init(&func_proc))
	{
		ret = rtos_stop_oneshot_timer(&func_proc);
		if(ret!=0)
		{
			BK_MODEM_LOGE("AT stop timer failed\r\n");
		}

		ret = rtos_deinit_oneshot_timer(&func_proc);
		if(ret!=0)
		{
			BK_MODEM_LOGE("AT deinit timer failed\r\n");
		}
	}
	return ret;
}
  
/// ec at begin
/**
 * @brief Check NAT configuration in EC mode
 * @return BK_OK if command succeeds, BK_FAIL otherwise
 *        This function checks the current NAT (Network Address Translation) configuration.
 */
bk_err_t bk_modem_ec_at_check_nat(void)
{
	if (BK_OK == bk_modem_at_cmd_send(AT_ECNETCFG_Q, 3, 5000))
	{
		BK_MODEM_LOGI("rsp:%s\r\n",g_modem_at_rsp_buf);
		return BK_OK;
	}
	else
	{
		BK_MODEM_LOGI("at_cmd_send fail!, AT_ECNETCFG_Q\r\n");
		return BK_FAIL;
	}
    
	return BK_FAIL;
}

/**
 * @brief Set NAT configuration in EC mode
 * @return BK_OK if command succeeds, BK_FAIL otherwise
 *        This function configures the NAT (Network Address Translation) settings.
 */
bk_err_t bk_modem_ec_at_set_nat(void)
{
	if (BK_OK == bk_modem_at_cmd_send(AT_ECNETCFG_S, 3, 5000))
	{
		BK_MODEM_LOGI("rsp:%s\r\n",g_modem_at_rsp_buf);
		return BK_OK;
	}
	else
	{
		BK_MODEM_LOGI("at_cmd_send fail!, AT_ECNETCFG_S\r\n");
		return BK_FAIL;
	}
    
	return BK_FAIL;
}

/**
 * @brief Close RNDIS interface in EC mode
 * @return BK_OK if command succeeds, BK_FAIL otherwise
 *        This function disables the RNDIS network interface.
 */
bk_err_t bk_modem_ec_at_close_rndis(void)
{
	if (BK_OK == bk_modem_at_cmd_send(AT_ECPCFG, 3, 5000))
	{
		BK_MODEM_LOGI("rsp:%s\r\n",g_modem_at_rsp_buf);
		return BK_OK;
	}
	else
	{
		BK_MODEM_LOGI("at_cmd_send fail!, AT_ECPCFG\r\n");
		return BK_FAIL;
	}
    
	return BK_FAIL;
}

/**
 * @brief Open data path in EC mode
 * @return BK_OK if command succeeds, BK_FAIL otherwise
 *        This function establishes the data path for network communication.
 */
bk_err_t bk_modem_ec_at_open_datapath(void)
{
	if (BK_OK == bk_modem_at_cmd_send(AT_ECNETDEVCTL, 3, 5000))
	{
		BK_MODEM_LOGI("rsp:%s\r\n",g_modem_at_rsp_buf);
		return BK_OK;
	}
	else
	{
		BK_MODEM_LOGI("at_cmd_send fail!, AT_ECNETDEVCTL\r\n");
		return BK_FAIL;
	}
    
	return BK_FAIL;
}

/**
 * @brief Reset EC module
 * @return BK_OK if command succeeds, BK_FAIL otherwise
 *        This function sends a reset command to the EC module.
 */
bk_err_t bk_modem_ec_at_rst(void)
{
	if (BK_OK == bk_modem_at_cmd_send(AT_ECRST, 1, 5000))
	{
		BK_MODEM_LOGI("rsp:%s\r\n",g_modem_at_rsp_buf);
		return BK_OK;
	}
	else
	{
		BK_MODEM_LOGI("at_cmd_send fail!, AT_ECRST\r\n");
		return BK_FAIL;
	}
    
	return BK_OK;
}

bk_err_t bk_modem_at_get_ati(void)
{
	char *ptr;

	if (BK_OK == bk_modem_at_cmd_send(ATI, 3, 5000))
	{
		BK_MODEM_LOGI("ATI, rsp:%s\r\n",g_modem_at_rsp_buf);
		return BK_OK;
	}
	else
	{
		BK_MODEM_LOGI("at_cmd_send fail!,ATI\r\n");
		return BK_FAIL;
	}
}

bk_err_t bk_modem_at_get_cgsn(void)
{
	char *ptr;

	if (BK_OK == bk_modem_at_cmd_send(AT_CGSN, 3, 5000))
	{
		BK_MODEM_LOGI("AT+CGSN, rsp:%s\r\n",g_modem_at_rsp_buf);
		if ((ptr = os_strstr((char *)g_modem_at_rsp_buf, "\r\n+CGSN: ")) != NULL)
		{
			ptr += 10; // offset "\r\n+CGSN: "
			os_strncpy((char *)dce_pdp_ctx.imei, ptr, BK_MODEM_DCE_IMEI_LEN);
		}
		return BK_OK;
	}
	else
	{
		BK_MODEM_LOGI("at_cmd_send fail!,AT+CGSN\r\n");
		return BK_FAIL;
	}
}

bk_err_t bk_modem_at_get_cfsn(void)
{
	char *ptr;

	if (BK_OK == bk_modem_at_cmd_send(AT_CFSN, 3, 5000))
	{
		BK_MODEM_LOGI("AT+CFSN, rsp:%s\r\n",g_modem_at_rsp_buf);
		if ((ptr = os_strstr((char *)g_modem_at_rsp_buf, "\r\n+CFSN: ")) != NULL)
		{
			ptr += 10; // offset "\r\nCFSN: "
			os_strncpy((char *)dce_pdp_ctx.sn, ptr, BK_MODEM_DCE_SN_LEN);
		}
		return BK_OK;
	}
	else
	{
		BK_MODEM_LOGI("at_cmd_send fail!,AT+CFSN\r\n");
		return BK_FAIL;
	}
}

bk_err_t bk_modem_at_get_cfun(void)
{
	if (BK_OK == bk_modem_at_cmd_send(AT_CFUN, 3, 5000))
	{
		BK_MODEM_LOGI("AT+CFUN, rsp:%s\r\n",g_modem_at_rsp_buf);
		return BK_OK;
	}
	else
	{
		BK_MODEM_LOGI("at_cmd_send fail!,AT+CFUN\r\n");
		return BK_FAIL;
	}
}

bk_err_t bk_modem_at_get_cgmr(void)
{
	char *ptr;

	if (BK_OK == bk_modem_at_cmd_send(AT_CGMR, 3, 5000))
	{
		BK_MODEM_LOGI("AT+CGMR, rsp:%s\r\n",g_modem_at_rsp_buf);
		if ((ptr = os_strstr((char *)g_modem_at_rsp_buf, "\r\n+CGMR: ")) != NULL)
		{
			ptr += 10; // offset "\r\n+CGMR: "
			os_strncpy((char *)dce_pdp_ctx.sw_ver, ptr, BK_MODEM_DCE_SVER_LEN);
		}
		return BK_OK;
	}
	else
	{
		BK_MODEM_LOGI("at_cmd_send fail!,AT+CGMR\r\n");
		return BK_FAIL;
	}
}
