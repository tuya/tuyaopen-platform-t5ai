#include <common/bk_include.h>
#include "bk_arm_arch.h"
#include <common/sys_config.h>
#include "flash_bypass.h"
#include "sys_driver.h"
#include "gpio_driver.h"
#include "spi_hal.h"
#include "driver/flash.h"
#include <os/mem.h>
#include <soc/soc.h>
#include "flash_driver.h"
#include "flash_hal.h"

#if CONFIG_SOC_BK7236XX
#define SPI_R_0X2(_id)  (SPI_R_BASE(_id) + 2 * 0x04)
__attribute__((section(".iram"))) int flash_bypass_op_write(uint8_t *op_code, uint8_t *tx_buf, uint32_t tx_len)
{
	uint32_t reg;
	uint32_t reg_0x2, reg_ctrl, reg_dat;
	uint32_t reg_stat, reg_cfg;
	uint32_t int_status = 0;
	int exceptional_flag = 0;
	uint32_t reg_sys_clk_en_0xc, reg_sys_clk_sel_0xa;

	int_status =  rtos_disable_int();

	/*step 1, save spi register configuration*/
	reg_0x2 = REG_READ(SPI_R_0X2(0));
	reg_ctrl = REG_READ(SPI_R_CTRL(0));
	reg_stat = REG_READ(SPI_R_INT_STATUS(0));
	reg_dat  = REG_READ(SPI_R_DATA(0));
	reg_cfg  = REG_READ(SPI_R_CFG(0));

	/*step 2, en software reset bit, bk7236/58 should enable this bit*/
	reg = REG_READ(SPI_R_0X2(0));
	reg |= (1 << 0);
	REG_WRITE(SPI_R_0X2(0), reg);

	/*step 3, config spi master*/
	/*     3.1 clear spi fifo content*/
	reg = REG_READ(SPI_R_INT_STATUS(0));
	while (reg & SPI_STATUS_RXFIFO_RD_READY) {
		REG_READ(SPI_R_DATA(0));
		reg = REG_READ(SPI_R_INT_STATUS(0));
	}
	/*     3.2 disable spi block, and backup */
	reg_sys_clk_en_0xc = reg = REG_READ(SYS_R_ADD_X(0xc));
	reg &= ~(1 << 1);
	REG_WRITE(SYS_R_ADD_X(0xc), reg);

	/*     3.3 clear spi status*/
	REG_WRITE(SPI_R_CTRL(0), 0);
	reg = REG_READ(SPI_R_INT_STATUS(0));
	REG_WRITE(SPI_R_INT_STATUS(0), reg);
	REG_WRITE(SPI_R_CFG(0), 0);

	/*     3.4 save the previous setting status*/
	/* gpioi of SPI0 are set as high-impedance state or input state ,
	          for spi mux with them*/
	//gpio_dev_unmap(SPI0_LL_CSN_PIN);
	//gpio_dev_unmap(SPI0_LL_SCK_PIN);
	//gpio_dev_unmap(SPI0_LL_MOSI_PIN);
	//gpio_dev_unmap(SPI0_LL_MISO_PIN);


	//gpio_dev_map(SPI0_LL_SCK_PIN, GPIO_DEV_SPI0_SCK);
	//gpio_dev_map(SPI0_LL_CSN_PIN, GPIO_DEV_SPI0_CSN);
	//gpio_dev_map(SPI0_LL_MOSI_PIN, GPIO_DEV_SPI0_MOSI);
	//gpio_dev_map(SPI0_LL_MISO_PIN, GPIO_DEV_SPI0_MISO);

	/*     3.5 set the spi master mode*/
	// open clock, and backup
	reg = REG_READ(SYS_R_ADD_X(0xc));
	reg |= (1 << 1);
	REG_WRITE(SYS_R_ADD_X(0xc), reg);

	// select 26M
	reg_sys_clk_sel_0xa = reg = REG_READ(SYS_R_ADD_X(0xa));
	reg &= ~(1 << 4);
	REG_WRITE(SYS_R_ADD_X(0xa), reg);

	// set to spi config directly
	reg = 0xC00100; // spien  msten  spi_clk=1---13M
	REG_WRITE(SPI_R_CTRL(0), reg);

	/*step 4, gpio(14/15/16/17) are set as high-impedance state or input state */

	/*step 5, switch flash interface to spi
	 *        Pay attention to prefetch instruction destination, the text can not
	 *        fetch from flash space after this timepoint.
	 */
	//sys_drv_set_cpu_storage_connect_op_select_flash_sel(1);
	reg = REG_READ(SYS_R_ADD_X(0x2));
	reg |= (1 << 9);
	REG_WRITE(SYS_R_ADD_X(0x2), reg);

	if(op_code != NULL)
	{
		/*step 6, write enable for volatile status register: 50H*/
		/*      6.1:take cs*/
		reg = REG_READ(SPI_R_CFG(0));
		reg &= ~(SPI_CFG_TRX_LEN_MASK << SPI_CFG_TX_TRAHS_LEN_POSI);
		reg |= (1 << SPI_CFG_TX_TRAHS_LEN_POSI);
		reg |= (SPI_CFG_TX_EN | SPI_CFG_TX_FIN_INT_EN);
		REG_WRITE(SPI_R_CFG(0), reg);

		/*      6.2:write tx fifo*/
		reg = REG_READ(SPI_R_INT_STATUS(0));
		if (reg & SPI_STATUS_TXFIFO_WR_READY)
			REG_WRITE(SPI_R_DATA(0), *op_code);
		else {
			exceptional_flag = -1;
			goto wr_exceptional;
		}

		/*      6.3:waiting for TXFIFO_EMPTY interrupt*/
		while (1) {
			reg = REG_READ(SPI_R_INT_STATUS(0));
			if (reg & SPI_STATUS_TX_FINISH_INT)
				break;
		}

		/*      6.4:release cs*/
		reg = REG_READ(SPI_R_CFG(0));
		reg &= ~(SPI_CFG_TRX_LEN_MASK << SPI_CFG_TX_TRAHS_LEN_POSI);
		reg &= ~(SPI_CFG_TX_EN | SPI_CFG_TX_FIN_INT_EN);
		REG_WRITE(SPI_R_CFG(0), reg);

		// cler stat and fifo
		reg = REG_READ(SPI_R_INT_STATUS(0));
		while (reg & SPI_STATUS_RXFIFO_RD_READY) {
			REG_READ(SPI_R_DATA(0));
			reg = REG_READ(SPI_R_INT_STATUS(0));
		}
		reg = REG_READ(SPI_R_INT_STATUS(0));
		REG_WRITE(SPI_R_INT_STATUS(0), reg);
	}

	if((tx_len == 0) || (tx_buf == NULL))
	{
		exceptional_flag = 0;
		goto wr_exceptional;
	}

	/*step 7, for tx_buf */
	/*      7.1:take cs*/
	reg = REG_READ(SPI_R_CFG(0));
	reg &= ~(SPI_CFG_TRX_LEN_MASK << SPI_CFG_TX_TRAHS_LEN_POSI);
	reg &= ~(SPI_CFG_TRX_LEN_MASK << SPI_CFG_RX_TRAHS_LEN_POSI);
	reg |= ((tx_len & SPI_CFG_TRX_LEN_MASK) << SPI_CFG_TX_TRAHS_LEN_POSI);
	reg |= (SPI_CFG_TX_EN | SPI_CFG_TX_FIN_INT_EN);
	REG_WRITE(SPI_R_CFG(0), reg);

	/*      7.2:write tx fifo*/
	// write tx first
	for (int i = 0, wait = 0; i < tx_len; ) {
		reg = REG_READ(SPI_R_INT_STATUS(0));
		if ((reg & SPI_STATUS_TXFIFO_WR_READY) == 0) {
			for(volatile int j=0; j<500; j++);
			wait++;
			if(wait > 100) {
				exceptional_flag = -2;
				goto wr_exceptional;
			}
		} else {
			wait = 0;
			REG_WRITE(SPI_R_DATA(0), tx_buf[i]);
			i++;
		}
	}

	/*      7.3:waiting for TXFIFO_EMPTY interrupt*/
	while (1) {
		reg = REG_READ(SPI_R_INT_STATUS(0));
		if (reg & SPI_STATUS_TX_FINISH_INT)
			break;
	}

	/*      7.4:release cs*/
	reg = REG_READ(SPI_R_CFG(0));
	reg &= ~(SPI_CFG_TRX_LEN_MASK << SPI_CFG_TX_TRAHS_LEN_POSI);
	reg &= ~(SPI_CFG_TRX_LEN_MASK << SPI_CFG_RX_TRAHS_LEN_POSI);
	reg &= (SPI_CFG_RX_EN | SPI_CFG_TX_EN | SPI_CFG_TX_FIN_INT_EN);
	REG_WRITE(SPI_R_CFG(0), reg);

	// cler stat and fifo
	reg = REG_READ(SPI_R_INT_STATUS(0));
	while (reg & SPI_STATUS_RXFIFO_RD_READY) {
		REG_READ(SPI_R_DATA(0));
		reg = REG_READ(SPI_R_INT_STATUS(0));
	}
	reg = REG_READ(SPI_R_INT_STATUS(0));
	REG_WRITE(SPI_R_INT_STATUS(0), reg);
	exceptional_flag = 0;

wr_exceptional:
	/*step 8, switch flash interface to flash controller */
	//sys_drv_set_cpu_storage_connect_op_select_flash_sel(0);
	reg = REG_READ(SYS_R_ADD_X(0x2));
	reg &= ~(1 << 9);
	REG_WRITE(SYS_R_ADD_X(0x2), reg);

	// recover icu and powerdown bits for spi
	REG_WRITE(SYS_R_ADD_X(0xc), reg_sys_clk_en_0xc);
	REG_WRITE(SYS_R_ADD_X(0xa), reg_sys_clk_sel_0xa);

	/*step 9, gpio(14/15/16/17) second function*/

	/*step 10, restore spi register configuration*/
	REG_WRITE(SPI_R_CTRL(0), reg_ctrl);
	REG_WRITE(SPI_R_INT_STATUS(0), reg_stat);
	REG_WRITE(SPI_R_DATA(0), reg_dat);
	REG_WRITE(SPI_R_CFG(0), reg_cfg);
	REG_WRITE(SPI_R_0X2(0), reg_0x2);

	/*step 11, enable interrupt*/
	rtos_enable_int(int_status);

	return exceptional_flag;
}

#if CONFIG_FLASH_BYPASS_OTP_OPERATION
__attribute__((section(".iram"))) int flash_bypass_op_read(uint8_t *tx_buf, uint32_t tx_len, uint8_t *rx_buf, uint32_t rx_len)
{
	uint32_t reg;
	uint32_t reg_0x2, reg_ctrl, reg_dat;
	uint32_t reg_stat, reg_cfg;
	uint32_t int_status = 0;
	int exceptional_flag = 0;
	uint32_t reg_sys_clk_en_0xc, reg_sys_clk_sel_0xa;

	int_status =  rtos_disable_int();

	/*step 1, save spi register configuration*/
	reg_0x2 = REG_READ(SPI_R_0X2(0));
	reg_ctrl = REG_READ(SPI_R_CTRL(0));
	reg_stat = REG_READ(SPI_R_INT_STATUS(0));
	reg_dat  = REG_READ(SPI_R_DATA(0));
	reg_cfg  = REG_READ(SPI_R_CFG(0));

	/*step 2, en software reset bit, bk7236/58 should enable this bit*/
	reg = REG_READ(SPI_R_0X2(0));
	reg |= (1 << 0);
	REG_WRITE(SPI_R_0X2(0), reg);

	/*step 3, config spi master*/
	/*     3.1 clear spi fifo content*/
	reg = REG_READ(SPI_R_INT_STATUS(0));
	while (reg & SPI_STATUS_RXFIFO_RD_READY) {
		REG_READ(SPI_R_DATA(0));
		reg = REG_READ(SPI_R_INT_STATUS(0));
	}
	/*     3.2 disable spi block, and backup */
	reg_sys_clk_en_0xc = reg = REG_READ(SYS_R_ADD_X(0xc));
	reg &= ~(1 << 1);
	REG_WRITE(SYS_R_ADD_X(0xc), reg);

	/*     3.3 clear spi status*/
	REG_WRITE(SPI_R_CTRL(0), 0);
	reg = REG_READ(SPI_R_INT_STATUS(0));
	REG_WRITE(SPI_R_INT_STATUS(0), reg);
	REG_WRITE(SPI_R_CFG(0), 0);

	/*     3.4 save the previous setting status*/
	/* gpioi of SPI0 are set as high-impedance state or input state ,
	          for spi mux with them*/
	//gpio_dev_unmap(SPI0_LL_CSN_PIN);
	//gpio_dev_unmap(SPI0_LL_SCK_PIN);
	//gpio_dev_unmap(SPI0_LL_MOSI_PIN);
	//gpio_dev_unmap(SPI0_LL_MISO_PIN);


	//gpio_dev_map(SPI0_LL_SCK_PIN, GPIO_DEV_SPI0_SCK);
	//gpio_dev_map(SPI0_LL_CSN_PIN, GPIO_DEV_SPI0_CSN);
	//gpio_dev_map(SPI0_LL_MOSI_PIN, GPIO_DEV_SPI0_MOSI);
	//gpio_dev_map(SPI0_LL_MISO_PIN, GPIO_DEV_SPI0_MISO);

	/*     3.5 set the spi master mode*/
	// open clock, and backup
	reg = REG_READ(SYS_R_ADD_X(0xc));
	reg |= (1 << 1);
	REG_WRITE(SYS_R_ADD_X(0xc), reg);

	// select 26M
	reg_sys_clk_sel_0xa = reg = REG_READ(SYS_R_ADD_X(0xa));
	reg &= ~(1 << 4);
	REG_WRITE(SYS_R_ADD_X(0xa), reg);

	// set to spi config directly
	reg = 0xC00100; // spien  msten  spi_clk=1---13M
	REG_WRITE(SPI_R_CTRL(0), reg);

	/*step 4, gpio(14/15/16/17) are set as high-impedance state or input state */

	/*step 5, switch flash interface to spi
	 *        Pay attention to prefetch instruction destination, the text can not
	 *        fetch from flash space after this timepoint.
	 */
	//sys_drv_set_cpu_storage_connect_op_select_flash_sel(1);
	reg = REG_READ(SYS_R_ADD_X(0x2));
	reg |= (1 << 9);
	REG_WRITE(SYS_R_ADD_X(0x2), reg);


	if((tx_len == 0) || (tx_buf == NULL))
	{
		exceptional_flag = 0;
		goto wr_exceptional;
	}

	/*step 7, for tx_buf */
	/*      7.1:take cs*/
	reg = REG_READ(SPI_R_CFG(0));
	reg &= ~(SPI_CFG_TRX_LEN_MASK << SPI_CFG_TX_TRAHS_LEN_POSI);
	reg &= ~(SPI_CFG_TRX_LEN_MASK << SPI_CFG_RX_TRAHS_LEN_POSI);
	reg |= (((tx_len + rx_len) & SPI_CFG_TRX_LEN_MASK) << SPI_CFG_TX_TRAHS_LEN_POSI);
	reg |= (((tx_len + rx_len) & SPI_CFG_TRX_LEN_MASK) << SPI_CFG_RX_TRAHS_LEN_POSI);
	reg |= (SPI_CFG_TX_EN | SPI_CFG_RX_EN | SPI_CFG_TX_FIN_INT_EN);
	REG_WRITE(SPI_R_CFG(0), reg);

	/*      7.2:write tx fifo*/
	// write tx first
	int tx_cnt = 0;
	int rx_cnt = 0;
	int wait = 0;
	for (int i = 0; i < tx_len; ) {
		reg = REG_READ(SPI_R_INT_STATUS(0));
		if ((reg & SPI_STATUS_TXFIFO_WR_READY) == 0) {
			for(int j=0; j<500; j++);
			wait++;
			if(wait > 100) {
				exceptional_flag = -1;
				goto wr_exceptional;
			}
		} else {
			wait = 0;
			REG_WRITE(SPI_R_DATA(0), tx_buf[i]);
			i++;
		}

		reg = REG_READ(SPI_R_INT_STATUS(0));
		if ((reg & SPI_STATUS_RXFIFO_RD_READY)) {
			if (tx_cnt < tx_len) {
				REG_READ(SPI_R_DATA(0));
				tx_cnt++;
			} else if (rx_cnt < rx_len) {
				rx_buf[rx_cnt] = REG_READ(SPI_R_DATA(0));
				rx_cnt++;
			} else {
				REG_READ(SPI_R_DATA(0));
			}
		}
	}

	// read rx, send 0xff
	wait = 0;
	for (int i = 0; i < rx_len; ) {
		reg = REG_READ(SPI_R_INT_STATUS(0));
		if ((reg & SPI_STATUS_TXFIFO_WR_READY) == 0) {
			for(int j=0; j<500; j++);
			wait++;
			if(wait > 100) {
				exceptional_flag = -2;
				goto wr_exceptional;
			}
		} else {
			wait = 0;
			REG_WRITE(SPI_R_DATA(0), 0xff);
			i++;
		}

		reg = REG_READ(SPI_R_INT_STATUS(0));
		if ((reg & SPI_STATUS_RXFIFO_RD_READY)) {
			if(tx_cnt < tx_len) {
				REG_READ(SPI_R_DATA(0));
				tx_cnt++;
			} else if(rx_cnt < rx_len) {
				rx_buf[rx_cnt] = REG_READ(SPI_R_DATA(0));
				rx_cnt++;
			} else {
				REG_READ(SPI_R_DATA(0));
			}
		}
	}

	/*      7.3:waiting for TXFIFO_EMPTY interrupt*/
	while (1) {
		reg = REG_READ(SPI_R_INT_STATUS(0));
		if (reg & SPI_STATUS_TX_FINISH_INT)
			break;
		else if ((reg & SPI_STATUS_RXFIFO_RD_READY)) {
			if(tx_cnt < tx_len) {
				REG_READ(SPI_R_DATA(0));
				tx_cnt++;
			} else if(rx_cnt < rx_len) {
				rx_buf[rx_cnt] = REG_READ(SPI_R_DATA(0));
				rx_cnt++;
			}
		}
	}

	wait = 0;
	while(rx_cnt < rx_len) {
		reg = REG_READ(SPI_R_INT_STATUS(0));
		if ((reg & SPI_STATUS_RXFIFO_RD_READY)) {
			if(tx_cnt < tx_len) {
				REG_READ(SPI_R_DATA(0));
				tx_cnt++;
			}
			rx_buf[rx_cnt] = REG_READ(SPI_R_DATA(0));
			rx_cnt++;

			wait = 0;
		} else {
			for(int j=0; j<500; j++);
			wait++;
			if(wait > 100) {
				break;
			}
		}
	}
	exceptional_flag = rx_cnt;

	/*      7.4:release cs*/
	reg = REG_READ(SPI_R_CFG(0));
	reg &= ~(SPI_CFG_TRX_LEN_MASK << SPI_CFG_TX_TRAHS_LEN_POSI);
	// reg &= ~(SPI_CFG_TRX_LEN_MASK << SPI_CFG_RX_TRAHS_LEN_POSI);
	reg &= (SPI_CFG_TX_EN | SPI_CFG_TX_FIN_INT_EN);
	REG_WRITE(SPI_R_CFG(0), reg);

wr_exceptional:
	reg = 0;
	REG_WRITE(SPI_R_CFG(0), reg);

	// cler stat and fifo
	reg = REG_READ(SPI_R_INT_STATUS(0));
	while (reg & SPI_STATUS_RXFIFO_RD_READY) {
		REG_READ(SPI_R_DATA(0));
		reg = REG_READ(SPI_R_INT_STATUS(0));
	}
	reg = REG_READ(SPI_R_INT_STATUS(0));
	REG_WRITE(SPI_R_INT_STATUS(0), reg);

	/*step 8, switch flash interface to flash controller */
	//sys_drv_set_cpu_storage_connect_op_select_flash_sel(0);
	reg = REG_READ(SYS_R_ADD_X(0x2));
	reg &= ~(1 << 9);
	REG_WRITE(SYS_R_ADD_X(0x2), reg);

	// recover icu and powerdown bits for spi
	REG_WRITE(SYS_R_ADD_X(0xc), reg_sys_clk_en_0xc);
	REG_WRITE(SYS_R_ADD_X(0xa), reg_sys_clk_sel_0xa);

	/*step 9, gpio(14/15/16/17) second function*/

	/*step 10, restore spi register configuration*/
	REG_WRITE(SPI_R_CTRL(0), reg_ctrl);
	REG_WRITE(SPI_R_INT_STATUS(0), reg_stat);
	REG_WRITE(SPI_R_DATA(0), reg_dat);
	REG_WRITE(SPI_R_CFG(0), reg_cfg);
	REG_WRITE(SPI_R_0X2(0), reg_0x2);

	/*step 11, enable interrupt*/
	rtos_enable_int(int_status);

	return exceptional_flag;
}

extern void bk_delay_us(uint32 num);
bool flash_bypass_op_read_buf_can_be_zero_flag = 0;
bool flash_bypass_otp_test_flag = 0;
static bool flash_bypass_otp_test_is_flag(void)
{
	if (flash_bypass_otp_test_flag) {
		FLASH_BYPASS_LOGW("flash_bypass_otp is working, maybe wait it finish.\r\n");
	} else {
		// BK_LOGD(NULL,"flash_bypass_otp is free now, you can use it.\r\n");
	}
	return flash_bypass_otp_test_flag;
}

static bk_err_t flash_bypass_op_read_and_check(uint8_t *tx_buf, uint32_t tx_len, uint8_t *rx_buf, uint32_t rx_len, bool null_flag)
{
	int ret  = BK_OK, ret1 = BK_OK, ret2 = BK_OK;
	flash_bypass_op_read_buf_can_be_zero_flag = null_flag;

	uint8_t *rx_buf1 = (uint8_t *)os_malloc(rx_len);
	uint8_t *rx_buf2 = (uint8_t *)os_malloc(rx_len);
	uint8_t *err_buf = (uint8_t *)os_malloc(rx_len);
	if (!rx_buf1 || !rx_buf2 || !err_buf) {
		FLASH_BYPASS_LOGW("%s malloc err\r\n", __func__);
		ret = BK_FAIL;
		goto read_check_exit;
	}

	os_memset(err_buf, 0x00, rx_len);

	// read retry 3 times(default), until the read result consecutively twice are the same
	for (uint8_t i = 0; i < FLASH_BYPASS_OTP_READ_RETRY_MAX; i++) {
		ret1 = flash_bypass_op_read(tx_buf, tx_len, rx_buf1, rx_len);
		bk_delay_us(1000);
		ret2 = flash_bypass_op_read(tx_buf, tx_len, rx_buf2, rx_len);
		bk_delay_us(1000);

		// if flash_bypass_op_read ok, it will return ret >= 0
		if ((ret1 < 0) || (ret2 < 0)) {
			continue;
		}

		if ((!flash_bypass_op_read_buf_can_be_zero_flag) &&
		((os_memcmp(rx_buf1, err_buf, rx_len) == 0) || (os_memcmp(rx_buf2, err_buf, rx_len) == 0))) {
			continue;
		}

		if (os_memcmp(rx_buf1, rx_buf2, rx_len) == 0) {
			break;
		}
	}

	if ((!flash_bypass_op_read_buf_can_be_zero_flag) &&
		((os_memcmp(rx_buf1, err_buf, rx_len) == 0) || (os_memcmp(rx_buf2, err_buf, rx_len) == 0))) {
		FLASH_BYPASS_LOGW("%s fail\r\n", __func__);
		ret = BK_FAIL;
		goto read_check_exit;
	}

	if (os_memcmp(rx_buf1, rx_buf2, rx_len) != 0) {
		FLASH_BYPASS_LOGW("%s fail\r\n", __func__);
		ret = BK_FAIL;
		goto read_check_exit;
	}

	ret = BK_OK;

	os_memcpy(rx_buf, rx_buf2, rx_len);

read_check_exit:
	if (rx_buf1) {
		os_free(rx_buf1);
	}
	if (rx_buf2) {
		os_free(rx_buf2);
	}
	if (err_buf) {
		os_free(err_buf);
	}
	return ret;
}

static bk_err_t flash_bypass_status_read(uint32_t *sta_val)
{
	int ret;

	uint8_t sta_reg_read_cmd[FLASH_STA_REG_MAX] = {
		CMD_FLASH_STA_REG_1_READ,
		CMD_FLASH_STA_REG_2_READ,
		CMD_FLASH_STA_REG_3_READ
	};
	uint8_t tx_buf[CMD_FLASH_STA_REG_READ_TASK_LEN] = {0};
	uint8_t tx_len = CMD_FLASH_STA_REG_READ_TASK_LEN;
	uint8_t rx_buf = 0;
	uint8_t rx_len = CMD_FLASH_STA_REG_READ_TASK_LEN;

	uint32_t sta_reg_val = 0;
	for (uint8_t i = 0; i < FLASH_STA_REG_MAX; i++) {
		tx_buf[0] = sta_reg_read_cmd[i];
		// otp_read_result maybe 0x00
		ret = flash_bypass_op_read_and_check(tx_buf, tx_len, &rx_buf, rx_len, 1);
		if (ret < 0) {
			FLASH_BYPASS_LOGW("%s fail\r\n", __func__);
			return BK_FAIL;
		}
		sta_reg_val |= (rx_buf << (i * 8));
	}

	os_memcpy(sta_val, &sta_reg_val, sizeof(sta_reg_val));
	return BK_OK;
}

static void flash_bypass_wait_work_in_progress_end(void)
{
	uint32_t flash_bypass_sta_reg_val = 0;

	do {
		flash_bypass_status_read(&flash_bypass_sta_reg_val);
		bk_delay_us(5000);

		FLASH_BYPASS_LOGV("flash_bypass_sta_reg_val = 0x%x\n", flash_bypass_sta_reg_val);

		if (flash_bypass_sta_reg_val & FLASH_STA_REG_WIP_BIT) {
			bk_delay_us(5000);
		} else {
			break;
		}
	} while (1);
}

static bk_err_t flash_bypass_status_write(uint32_t *sta_val)
{
	int ret;

	if(flash_bypass_otp_test_is_flag()) {
		return BK_FAIL;
	}
	flash_bypass_otp_test_flag = 1;

	uint8_t sta_reg_write_cmd[FLASH_STA_REG_MAX] = {
		CMD_FLASH_STA_REG_1_WRITE,
		CMD_FLASH_STA_REG_2_WRITE,
		CMD_FLASH_STA_REG_3_WRITE
	};
	uint8_t tx_buf[CMD_FLASH_STA_REG_WRITE_TASK_LEN] = {0};
	uint8_t tx_len = CMD_FLASH_STA_REG_WRITE_TASK_LEN;

	uint8_t  op_code = CMD_FLASH_BYPASS_WR_EN;
	uint32_t sta_reg_val = *sta_val;
	for (uint8_t i = 0; i < FLASH_STA_REG_MAX; i++) {
		tx_buf[0] = sta_reg_write_cmd[i];
		tx_buf[1] = (sta_reg_val >> (i * 8));
		flash_bypass_wait_work_in_progress_end();
		ret = flash_bypass_op_write(&op_code, tx_buf, tx_len);
		if (ret != 0) {
			FLASH_BYPASS_LOGW("%s fail\r\n", __func__);
			flash_bypass_otp_test_flag = 0;
			return BK_FAIL;
		}
		flash_bypass_wait_work_in_progress_end();
	}

	flash_bypass_otp_test_flag = 0;
	return BK_OK;
}

static bk_err_t flash_bypass_otp_lock(flash_bypass_otp_ctrl_t *otp_cfg)
{
	bk_err_t ret = BK_OK;
	uint32_t flash_sta_reg_val = 0;
	flash_bypass_otp_ctrl_t otp_lock_cfg = {0};
	otp_lock_cfg.otp_idx = otp_cfg->otp_idx;

	ret = flash_bypass_status_read(&flash_sta_reg_val);
	if (ret != BK_OK) {
		FLASH_BYPASS_LOGW("%s fail\r\n", __func__);
		return BK_FAIL;
	}

	flash_sta_reg_val |= (1 << (FLASH_STA_REG_LOCK_OTP_OFFSET + otp_lock_cfg.otp_idx));

	int ret1 = BK_OK, ret2 = BK_OK;
	for (uint8_t i = 0; i < FLASH_BYPASS_OTP_LOCK_RETRY_MAX; i++) {
		ret1 = flash_bypass_status_write(&flash_sta_reg_val);
		ret2 = flash_bypass_status_read(&flash_sta_reg_val);
		if ((ret1 != BK_OK) || (ret2 != BK_OK)) {
			continue;
		}
		if (flash_sta_reg_val & (1 << (FLASH_STA_REG_LOCK_OTP_OFFSET + otp_lock_cfg.otp_idx))) {
			break;
		}
	}
	if (flash_sta_reg_val & (1 << (FLASH_STA_REG_LOCK_OTP_OFFSET + otp_lock_cfg.otp_idx))) {
		ret = BK_OK;
	} else {
		FLASH_BYPASS_LOGW("%s fail\r\n", __func__);
		ret = BK_FAIL;
	}

	return BK_OK;
}

static bk_err_t flash_bypass_otp_read(flash_bypass_otp_ctrl_t *otp_cfg, bool printf_flag)
{
	int ret = BK_OK;

	if(flash_bypass_otp_test_is_flag()) {
		return BK_FAIL;
	}
	flash_bypass_otp_test_flag = 1;

	uint8_t  tx_buf[CMD_FLASH_BYPASS_OTP_READ_TASK_LEN] = {0};
	uint8_t  tx_len   = CMD_FLASH_BYPASS_OTP_READ_TASK_LEN;
	uint32_t otp_addr = FLASH_BYPASS_OTP_IDX_X_ADDR(otp_cfg->otp_idx) + otp_cfg->addr_offset;
	tx_buf[0] = CMD_FLASH_BYPASS_OTP_READ;
	tx_buf[1] = (otp_addr >> 16) & 0xff;
	tx_buf[2] = (otp_addr >>  8) & 0xff;
	tx_buf[3] = (otp_addr >>  0) & 0xff;

	uint32_t rx_len   = otp_cfg->read_len;
	uint8_t  *rx_buf  = (uint8_t *)os_malloc(otp_cfg->read_len);
	if (rx_buf == NULL) {
		FLASH_BYPASS_LOGW("%s malloc err\r\n", __func__);
		ret = BK_FAIL;
		goto otp_read_exit;
	}

	// otp_read_result cannot be 0x00
	ret = flash_bypass_op_read_and_check(tx_buf, tx_len, rx_buf, rx_len, 0);
	if (ret != BK_OK) {
		FLASH_BYPASS_LOGW("%s fail\r\n", __func__);
		goto otp_read_exit;
	}

	if (printf_flag) {
		uint8_t printf_div = 16;
		for (uint32_t i = 0; i < rx_len; i++) {
			FLASH_BYPASS_RAWI("%02x ", rx_buf[i]);
			if ((i + 1) % printf_div == 0) {
				FLASH_BYPASS_RAWI("\n");
			}
		}
		if (rx_len % printf_div != 0) {
			FLASH_BYPASS_RAWI("\n");
		}
	}

	os_memcpy(otp_cfg->read_buf, rx_buf, rx_len);

otp_read_exit:
	if (rx_buf != NULL) {
		os_free(rx_buf);
	}

	flash_bypass_otp_test_flag = 0;
	return ret;
}

static bk_err_t flash_bypass_otp_earse(flash_bypass_otp_ctrl_t *otp_cfg)
{
	int ret = BK_OK;

	if(flash_bypass_otp_test_is_flag()) {
		return BK_FAIL;
	}
	flash_bypass_otp_test_flag = 1;

	uint8_t  op_code = CMD_FLASH_BYPASS_WR_EN;
	uint8_t  tx_buf[CMD_FLASH_BYPASS_OTP_EARSE_TASK_LEN] = {0};
	uint8_t  tx_len   = CMD_FLASH_BYPASS_OTP_EARSE_TASK_LEN;
	uint32_t otp_addr = FLASH_BYPASS_OTP_IDX_X_ADDR(otp_cfg->otp_idx);
	tx_buf[0] = CMD_FLASH_BYPASS_OTP_EARSE;
	tx_buf[1] = (otp_addr >> 16) & 0xff;
	tx_buf[2] = (otp_addr >>  8) & 0xff;
	tx_buf[3] = (otp_addr >>  0) & 0xff;

	flash_bypass_wait_work_in_progress_end();
	ret = flash_bypass_op_write(&op_code, tx_buf, tx_len);
	if (ret != 0) {
		FLASH_BYPASS_LOGW("%s fail\r\n", __func__);
		flash_bypass_otp_test_flag = 0;
		return BK_FAIL;
	}
	flash_bypass_wait_work_in_progress_end();

	flash_bypass_otp_test_flag = 0;
	return ret;
}

static bk_err_t flash_bypass_otp_earse_and_check(flash_bypass_otp_ctrl_t *otp_cfg)
{
	int ret = BK_OK;
	flash_bypass_otp_ctrl_t otp_op = {0};

	otp_op.otp_idx   = otp_cfg->otp_idx;
	otp_op.read_len  = FLASH_BYPASS_OTP_BLOCK_LENGTH;
	otp_op.read_buf  = (uint8_t *)os_malloc(otp_op.read_len);
	uint8_t *dft_buf = (uint8_t *)os_malloc(otp_op.read_len);
	if (!otp_op.read_buf || !dft_buf) {
		FLASH_BYPASS_LOGW("%s malloc err\r\n", __func__);
		ret = BK_FAIL;
		goto earse_and_read_check;
	}

	os_memset(dft_buf, 0xff, otp_op.read_len);

	int ret1 = BK_OK, ret2 = BK_OK;

	for (uint8_t i = 0; i < FLASH_BYPASS_OTP_EARSE_RETRY_MAX; i++) {
		ret1 = flash_bypass_otp_earse(&otp_op);
		ret2 = flash_bypass_otp_read(&otp_op , 0);
		if ((ret1 != BK_OK) || (ret2 != BK_OK)
		|| (os_memcmp(otp_op.read_buf, dft_buf, otp_op.read_len) != 0)) {
			continue;
		} else {
			break;
		}
	}

	if ((ret1 != BK_OK) || (ret2 != BK_OK)
	|| (os_memcmp(otp_op.read_buf, dft_buf, otp_op.read_len) != 0)) {
		ret = BK_FAIL;
	}

earse_and_read_check:
	if (otp_op.read_buf) {
		os_free(otp_op.read_buf);
	}
	if (dft_buf) {
		os_free(dft_buf);
	}
	return ret;
}

static bk_err_t flash_bypass_otp_write_without_read(flash_bypass_otp_ctrl_t *otp_cfg)
{
	int ret = BK_OK;

	if(flash_bypass_otp_test_is_flag()) {
		return BK_FAIL;
	}
	flash_bypass_otp_test_flag = 1;

	uint8_t  *tx_buf = (uint8_t *)os_malloc(CMD_FLASH_BYPASS_OTP_WRITE_TASK_LEN);
	if(!tx_buf) {
		FLASH_BYPASS_LOGW("%s malloc err\r\n", __func__);
		ret = BK_FAIL;
		goto write_exit;
	}

	uint8_t  op_code = CMD_FLASH_BYPASS_WR_EN;
	uint32_t tx_len  = CMD_FLASH_BYPASS_OTP_WRITE_HEAD_LEN + otp_cfg->write_len;
	uint32_t otp_addr = FLASH_BYPASS_OTP_IDX_X_ADDR(otp_cfg->otp_idx) + otp_cfg->addr_offset;
	tx_buf[0] = CMD_FLASH_BYPASS_OTP_WRITE;
	tx_buf[1] = (otp_addr >> 16) & 0xff;
	tx_buf[2] = (otp_addr >>  8) & 0xff;
	tx_buf[3] = (otp_addr >>  0) & 0xff;
	os_memcpy(tx_buf + CMD_FLASH_BYPASS_OTP_WRITE_HEAD_LEN, otp_cfg->write_buf, otp_cfg->write_len);

	flash_bypass_wait_work_in_progress_end();
	ret = flash_bypass_op_write(&op_code, tx_buf, tx_len);
	if (ret != 0) {
		FLASH_BYPASS_LOGW("%s fail\r\n", __func__);
		ret = BK_FAIL;
		goto write_exit;
	}
	flash_bypass_wait_work_in_progress_end();

write_exit:
	if (tx_buf) {
		os_free(tx_buf);
	}
	flash_bypass_otp_test_flag = 0;
	return ret;
}

static bk_err_t flash_bypass_otp_write(flash_bypass_otp_ctrl_t *otp_cfg)
{
	bk_err_t ret = BK_OK;
	flash_bypass_otp_ctrl_t otp_op_cfg = {0};
	flash_bypass_otp_ctrl_t otp_write_cfg = {0};

	// STEP1 read otp, save whole block data(1024-Byte)
	flash_bypass_otp_ctrl_t *otp_read_cfg = &otp_op_cfg;
	otp_read_cfg->otp_idx     = otp_cfg->otp_idx;
	otp_read_cfg->addr_offset = FLASH_BYPASS_OTP_BLOCK_START_OFFSET;
	otp_read_cfg->read_len    = FLASH_BYPASS_OTP_BLOCK_LENGTH;
	otp_read_cfg->read_buf    = (uint8_t *)os_malloc(otp_read_cfg->read_len);
	if(!otp_read_cfg->read_buf) {
		FLASH_BYPASS_LOGW("%s malloc err\r\n", __func__);
		return BK_FAIL;
	}
	ret = flash_bypass_otp_read(otp_read_cfg, 0);
	if (ret != BK_OK) {
		FLASH_BYPASS_LOGW("%s fail\r\n", __func__);
		ret = BK_FAIL;
		goto otp_write_exit;
	}

	// STEP2 earse otp, prepare for write
	flash_bypass_otp_ctrl_t *otp_earse_cfg = &otp_op_cfg;
	ret = flash_bypass_otp_earse_and_check(otp_earse_cfg);
	if (ret != BK_OK) {
		FLASH_BYPASS_LOGW("%s fail\r\n", __func__);
		ret = BK_FAIL;
		goto otp_write_exit;
	}

	// STEP3 combine write buf with otp_read_buf, prepare for write
	flash_bypass_otp_ctrl_t *otp_temp_cfg = otp_read_cfg;
	otp_temp_cfg->write_len    = FLASH_BYPASS_OTP_BLOCK_LENGTH;
	otp_temp_cfg->write_buf    = (uint8_t *)os_malloc(otp_temp_cfg->write_len);
	if(!otp_temp_cfg->write_buf) {
		FLASH_BYPASS_LOGW("%s malloc err\r\n", __func__);
		ret = BK_FAIL;
		goto otp_write_exit;
	}

	os_memcpy(otp_temp_cfg->write_buf, otp_read_cfg->read_buf, otp_read_cfg->read_len);
	os_memcpy(otp_temp_cfg->write_buf + otp_cfg->addr_offset, otp_cfg->write_buf, otp_cfg->write_len);

	// STEP4 write buf to otp
	otp_write_cfg.otp_idx   = otp_temp_cfg->otp_idx;
	otp_write_cfg.write_len = FLASH_BYPASS_OTP_PAGE_LENGTH;
	otp_write_cfg.write_buf = (uint8_t *)os_malloc(otp_write_cfg.write_len);
	if(!otp_write_cfg.write_buf) {
		FLASH_BYPASS_LOGW("%s malloc err\r\n", __func__);
		ret = BK_FAIL;
		goto otp_write_exit;
	}

	for (uint8_t i = 0; i < (FLASH_BYPASS_OTP_BLOCK_LENGTH / FLASH_BYPASS_OTP_PAGE_LENGTH); i++) {
		otp_write_cfg.addr_offset = i * FLASH_BYPASS_OTP_PAGE_LENGTH;
		os_memcpy(otp_write_cfg.write_buf, otp_temp_cfg->write_buf + i * FLASH_BYPASS_OTP_PAGE_LENGTH, FLASH_BYPASS_OTP_PAGE_LENGTH);
		ret = flash_bypass_otp_write_without_read(&otp_write_cfg);
		if (ret != BK_OK) {
			FLASH_BYPASS_LOGW("%s fail\r\n", __func__);
			ret = BK_FAIL;
			goto otp_write_exit;
		}
	}

otp_write_exit:
	if (otp_read_cfg->read_buf) {
		os_free(otp_read_cfg->read_buf);
	}
	if (otp_temp_cfg->write_buf) {
		os_free(otp_temp_cfg->write_buf);
	}
	if (otp_write_cfg.write_buf) {
		os_free(otp_write_cfg.write_buf);
	}
	return ret;
}

typedef struct {
	flash_hal_t            hal;
	uint32_t               flash_id;
	uint32_t               flash_status_reg_val;
	uint32_t               flash_line_mode;
} flash_driver_t;

static flash_driver_t s_flash = {0};
static bool s_flash_is_init = false;

static flash_line_mode_t flash_bypass_set_line_mode(flash_line_mode_t line_mode)
{
	if (!s_flash_is_init) {
		os_memset(&s_flash, 0, sizeof(s_flash));
		flash_hal_init(&s_flash.hal);
		s_flash_is_init = true;
	}

	flash_line_mode_t  new_line_mode;
	flash_line_mode_t  old_line_mode = s_flash.flash_line_mode;

#if CONFIG_FLASH_QUAD_ENABLE
	if ( (FLASH_LINE_MODE_FOUR == line_mode)
		&& (FLASH_LINE_MODE_FOUR == bk_flash_get_line_mode()) )
	{
		new_line_mode = FLASH_LINE_MODE_FOUR;
	}
	else
#endif
	{
		new_line_mode = FLASH_LINE_MODE_TWO;
	}

	if(new_line_mode == old_line_mode)
	{
		return old_line_mode;
	}

	flash_hal_clear_qwfr(&s_flash.hal);   // cmd CRMR (coutinuous_read_mode reset), quit QPI mode.

#if CONFIG_SOC_BK7236XX
	sys_drv_set_sys2flsh_2wire(0);
#endif

	if (FLASH_LINE_MODE_FOUR == new_line_mode)
	{
		flash_hal_set_quad_m_value(&s_flash.hal, bk_flash_get_coutinuous_read_mode());
		flash_hal_set_mode(&s_flash.hal, FLASH_MODE_QUAD);  // enter QPI mode.
	}
	else
	{
		flash_hal_set_mode(&s_flash.hal, FLASH_MODE_DUAL);
	}

	s_flash.flash_line_mode = new_line_mode;

#if CONFIG_SOC_BK7236XX
	sys_drv_set_sys2flsh_2wire(1);
#endif

	return old_line_mode;
}

bk_err_t flash_bypass_otp_operation(flash_bypass_otp_cmd_t cmd, flash_bypass_otp_ctrl_t *param)
{
	bk_err_t ret = BK_OK;
	flash_bypass_otp_ctrl_t *otp_cfg = (flash_bypass_otp_ctrl_t *)param;

	if ((otp_cfg->otp_idx >= FLASH_BYPASS_OTP_IDX_MAX)
	|| (otp_cfg->addr_offset + otp_cfg->read_len > FLASH_BYPASS_OTP_BLOCK_LENGTH)
	|| (otp_cfg->addr_offset + otp_cfg->write_len > FLASH_BYPASS_OTP_BLOCK_LENGTH)) {
		FLASH_BYPASS_LOGW("%s out of range\r\n", __func__);
		return BK_FAIL;
	}

	// flash need to change 2 line when do flash operate except read
	// need to recover 4 line, please do it manually
	if (FLASH_LINE_MODE_FOUR == bk_flash_get_line_mode())
		flash_bypass_set_line_mode(FLASH_LINE_MODE_TWO);

	switch(cmd) {
		case FLASH_BYPASS_OTP_READ:
			ret = flash_bypass_otp_read(otp_cfg, 1);
			break;

		case FLASH_BYPASS_OTP_EARSE:
			ret = flash_bypass_otp_earse_and_check(otp_cfg);
			break;

		case FLASH_BYPASS_OTP_WRITE:
			ret = flash_bypass_otp_write(otp_cfg);
			break;

		case FLASH_BYPASS_OTP_LOCK:
			ret = flash_bypass_otp_lock(otp_cfg);
			break;

	}
	if (ret != BK_OK) {
		FLASH_BYPASS_LOGW("%s fail\r\n", __func__);
	}

	if (FLASH_LINE_MODE_FOUR == bk_flash_get_line_mode())
		flash_bypass_set_line_mode(FLASH_LINE_MODE_FOUR);

	return ret;
}
#endif
#else /*CONFIG_SOC_BK7236XX*/

void flash_bypass_init(void) {
	char *text_ptr, temp_buf = 0;
	uint32_t reg;
	
	/*step 2, resident cache*/
	REG_WRITE(SPI_R_CTRL(0), 0);
	do {
		text_ptr = (char *)flash_bypass_quad_enable;
		for (uint32_t i = 0; i < CURRENT_ROUTINE_TEXT_SIZE; i ++)
			temp_buf += text_ptr[i];

		REG_WRITE(SPI_R_INT_STATUS(0), temp_buf);
	} while (0);

	/*step 3, config spi master*/
	/*  clear spi status*/
	REG_WRITE(SPI_R_CTRL(0), 0);
	reg = REG_READ(SPI_R_INT_STATUS(0));
	REG_WRITE(SPI_R_INT_STATUS(0), reg);
	REG_WRITE(SPI_R_CFG(0), 0);

	spi_config_t config = {0};
	config.role = SPI_ROLE_MASTER;
	config.bit_width = SPI_BIT_WIDTH_8BITS;
	config.polarity = 1;
	config.phase = 1;
	config.wire_mode = SPI_4WIRE_MODE;
	config.baud_rate = 1000000;
	config.bit_order = SPI_MSB_FIRST;
#if (CONFIG_SPI_BYTE_INTERVAL)
	config.byte_interval = 1;
#endif
	bk_spi_driver_init();
	bk_spi_init(0, &config);

	/*step 4, gpioi of SPI0 are set as high-impedance state or input state ,
	          for spi mux with them*/
	gpio_dev_unmap(SPI0_LL_CSN_PIN);
	gpio_dev_unmap(SPI0_LL_SCK_PIN);
	gpio_dev_unmap(SPI0_LL_MOSI_PIN);
	gpio_dev_unmap(SPI0_LL_MISO_PIN);
}

__attribute__((section(".itcm_sec_code"))) void flash_bypass_quad_enable(void)
{
	uint32_t reg;
	uint32_t reg_ctrl, reg_dat;
	uint32_t reg_stat, reg_cfg;
	uint32_t int_status = 0;
	uint32_t spi_status = 0;
	volatile uint32_t i, j, delay_count;

	int_status =  rtos_disable_int();

	/*step 1, save spi register configuration*/
	reg_ctrl = REG_READ(SPI_R_CTRL(0));
	reg_stat = REG_READ(SPI_R_INT_STATUS(0));
	reg_dat  = REG_READ(SPI_R_DATA(0));
	reg_cfg  = REG_READ(SPI_R_CFG(0));

	flash_bypass_init();

	/*step 5, switch flash interface to spi
	 *        Pay attention to prefetch instruction destination, the text can not
	 *        fetch from flash space after this timepoint.
	 */
	sys_drv_set_cpu_storage_connect_op_select_flash_sel(1);

	/*step 6, write enable for status register: 06H*/
	reg = REG_READ(SPI_R_INT_STATUS(0));
	REG_WRITE(SPI_R_INT_STATUS(0), reg);
	REG_WRITE(SPI_R_CFG(0), SPI_CFG_TX_EN_ONE_BYTE);
	REG_WRITE(SPI_R_DATA(0), FLASH_CMD_WR_EN_SR);

	/*step 7, write cmd 31H, data 0x02*/
	for(i = 0; i < 500; i++) {
		spi_status = REG_READ(SPI_R_INT_STATUS(0));
		if(0 != (spi_status & SPI_STATUS_TX_FINISH_INT)) {
			break;
		}
	}
	for(delay_count = 0; delay_count < 20000; delay_count ++)
	{
		for(j = 0; j < 8; j ++)
		;
	}

	REG_WRITE(SPI_R_CFG(0), 0);
	reg = REG_READ(SPI_R_INT_STATUS(0));
	REG_WRITE(SPI_R_INT_STATUS(0), reg);

	REG_WRITE(SPI_R_CFG(0), SPI_CFG_TX_EN_TWO_BYTE);
	REG_WRITE(SPI_R_DATA(0), FLASH_CMD_WR_SR);
	REG_WRITE(SPI_R_DATA(0), FLASH_GD25Q32C_SR_QUAD_EN);

	for(i = 0; i < 500; i++) {
		spi_status = REG_READ(SPI_R_INT_STATUS(0));
		if(0 != (spi_status & SPI_STATUS_TX_FINISH_INT)) {
			break;
		}
	}
	for(delay_count = 0; delay_count < 20000; delay_count ++)
	{
		for(j = 0; j < 8; j ++)
		;
	}
	REG_WRITE(SPI_R_CFG(0), 0);
	reg = REG_READ(SPI_R_INT_STATUS(0));
	REG_WRITE(SPI_R_INT_STATUS(0), reg);

	/*step 8, switch flash interface to flash controller */
	sys_drv_set_cpu_storage_connect_op_select_flash_sel(0);

	/*step 9, restore spi register configuration*/
	REG_WRITE(SPI_R_CTRL(0), reg_ctrl);
	REG_WRITE(SPI_R_INT_STATUS(0), reg_stat);
	REG_WRITE(SPI_R_DATA(0), reg_dat);
	REG_WRITE(SPI_R_CFG(0), reg_cfg);
	rtos_enable_int(int_status);
}


__attribute__((section(".itcm_sec_code"))) void flash_bypass_quad_test(uint32_t quad_enable, uint32_t delay_cycle1, uint32_t delay_cycle2)
{
	uint32_t reg;
	uint32_t reg_ctrl, reg_dat;
	uint32_t reg_stat, reg_cfg;
	uint32_t int_status = 0;
	uint32_t spi_status = 0;
	volatile uint32_t i, j, delay_count;

	int_status =  rtos_disable_int();

	/*step 1, save spi register configuration*/
	reg_ctrl = REG_READ(SPI_R_CTRL(0));
	reg_stat = REG_READ(SPI_R_INT_STATUS(0));
	reg_dat  = REG_READ(SPI_R_DATA(0));
	reg_cfg  = REG_READ(SPI_R_CFG(0));

	flash_bypass_init();

	/*step 5, switch flash interface to spi
	 *        Pay attention to prefetch instruction destination, the text can not
	 *        fetch from flash space after this timepoint.
	 */
	sys_drv_set_cpu_storage_connect_op_select_flash_sel(1);

	/*step 6, write enable for status register: 06H*/
	reg = REG_READ(SPI_R_INT_STATUS(0));
	REG_WRITE(SPI_R_INT_STATUS(0), reg);
	REG_WRITE(SPI_R_CFG(0), SPI_CFG_TX_EN_ONE_BYTE);
	REG_WRITE(SPI_R_DATA(0), FLASH_CMD_WR_EN_SR);

	/*step 7, write cmd 31H, data 0x02*/
	for(i = 0; i < delay_cycle1; i++) {
		spi_status = REG_READ(SPI_R_INT_STATUS(0));
		if(0 != (spi_status & SPI_STATUS_TX_FINISH_INT)) {
			break;
		}
	}
	for(delay_count = 0; delay_count < delay_cycle2; delay_count ++)
	{
		for(j = 0; j < 8; j ++)
		;
	}

	REG_WRITE(SPI_R_CFG(0), 0);
	reg = REG_READ(SPI_R_INT_STATUS(0));
	REG_WRITE(SPI_R_INT_STATUS(0), reg);

	REG_WRITE(SPI_R_CFG(0), SPI_CFG_TX_EN_TWO_BYTE);
	REG_WRITE(SPI_R_DATA(0), FLASH_CMD_WR_SR);
	if (quad_enable) {
		REG_WRITE(SPI_R_DATA(0), FLASH_GD25Q32C_SR_QUAD_EN);
	} else {
		REG_WRITE(SPI_R_DATA(0), 0);
	}

	for(i = 0; i < delay_cycle1; i++) {
		spi_status = REG_READ(SPI_R_INT_STATUS(0));
		if(0 != (spi_status & SPI_STATUS_TX_FINISH_INT)) {
			break;
		}
	}
	for(delay_count = 0; delay_count < delay_cycle2; delay_count ++)
	{
		for(j = 0; j < 8; j ++)
		;
	}
	REG_WRITE(SPI_R_CFG(0), 0);
	reg = REG_READ(SPI_R_INT_STATUS(0));
	REG_WRITE(SPI_R_INT_STATUS(0), reg);

	/*step 8, switch flash interface to flash controller */
	sys_drv_set_cpu_storage_connect_op_select_flash_sel(0);

	/*step 9, restore spi register configuration*/
	REG_WRITE(SPI_R_CTRL(0), reg_ctrl);
	REG_WRITE(SPI_R_INT_STATUS(0), reg_stat);
	REG_WRITE(SPI_R_DATA(0), reg_dat);
	REG_WRITE(SPI_R_CFG(0), reg_cfg);
	rtos_enable_int(int_status);
}

#endif
