/**
 * @file tkl_qspi.c
 * @brief default weak implements of tuya pin
 *
 * @copyright Copyright 2020-2021 Tuya Inc. All Rights Reserved.
 *
 */

#include "tuya_dma.h"
#include "tkl_qspi.h"
#include <driver/qspi.h>
#include <sdkconfig.h>
#include "qspi_hal.h"
#include <driver/qspi_types.h>
#include <driver/lcd_types.h>
#include <driver/lcd_qspi.h>
#include "gpio_driver.h"
#include <driver/gpio.h>

#if CONFIG_SOC_BK7236XX
#define LCD_QSPI0_DATA_ADDR     0x64000000
#define LCD_QSPI1_DATA_ADDR     0x68000000
#define LCD_QSPI_RESET_PIN      GPIO_40
#endif

extern media_debug_t *media_debug;
// up report

bk_err_t bk_lcd_qspi_quad_write_stops(qspi_id_t qspi_id);
extern void bk_delay_us(UINT32 us);


#define TUYA_QSPI_CLK_DIV   (0x2)
#define QSPI_INIT_CLK_480M (480000000)
struct qspi_init_config {
    TUYA_QSPI_IRQ_CB cb;
    uint8_t irq_enable;
    dma_id_t lcd_qspi_dma_id;
    uint32_t dma_repeat_once_len;
    BOOL_T is_send_use_dma;
    TUYA_QSPI_WIRE_MODE_E dma_data_lines;
    uint8_t qspi_enable;
};


static qspi_driver_t s_tkl_qspi[SOC_QSPI_UNIT_NUM] = {
	{
		.hal.hw = (qspi_hw_t *)(SOC_QSPI0_REG_BASE),
	},
#if (SOC_QSPI_UNIT_NUM > 1)
	{
		.hal.hw = (qspi_hw_t *)(SOC_QSPI1_REG_BASE),
	}
#endif
};
static struct qspi_init_config qspi_infos[TUYA_QSPI_NUM_MAX] = {0};

#define QSPI_SET_PIN(id) do {\
	gpio_dev_unmap(QSPI##id##_LL_CSN_PIN);\
	gpio_dev_unmap(QSPI##id##_LL_CLK_PIN);\
	gpio_dev_unmap(QSPI##id##_LL_IO0_PIN);\
	gpio_dev_unmap(QSPI##id##_LL_IO1_PIN);\
	gpio_dev_unmap(QSPI##id##_LL_IO2_PIN);\
	gpio_dev_unmap(QSPI##id##_LL_IO3_PIN);\
	gpio_dev_map(QSPI##id##_LL_CSN_PIN, GPIO_DEV_QSPI##id##_CSN);\
	gpio_dev_map(QSPI##id##_LL_CLK_PIN, GPIO_DEV_QSPI##id##_CLK);\
	gpio_dev_map(QSPI##id##_LL_IO0_PIN, GPIO_DEV_QSPI##id##_IO0);\
	gpio_dev_map(QSPI##id##_LL_IO1_PIN, GPIO_DEV_QSPI##id##_IO1);\
	gpio_dev_map(QSPI##id##_LL_IO2_PIN, GPIO_DEV_QSPI##id##_IO2);\
	gpio_dev_map(QSPI##id##_LL_IO3_PIN, GPIO_DEV_QSPI##id##_IO3);\
	bk_gpio_pull_up(QSPI##id##_LL_CSN_PIN);\
	bk_gpio_pull_up(QSPI##id##_LL_IO0_PIN);\
	bk_gpio_pull_up(QSPI##id##_LL_IO1_PIN);\
	bk_gpio_pull_up(QSPI##id##_LL_IO2_PIN);\
	bk_gpio_pull_up(QSPI##id##_LL_IO3_PIN);\
	bk_gpio_set_capacity(QSPI##id##_LL_CSN_PIN, 3);\
	bk_gpio_set_capacity(QSPI##id##_LL_CLK_PIN, 3);\
	bk_gpio_set_capacity(QSPI##id##_LL_IO0_PIN, 3);\
	bk_gpio_set_capacity(QSPI##id##_LL_IO1_PIN, 3);\
	bk_gpio_set_capacity(QSPI##id##_LL_IO2_PIN, 3);\
	bk_gpio_set_capacity(QSPI##id##_LL_IO3_PIN, 3);\
} while(0)

static uint8_t dma_id_to_port(dma_id_t dma_id)
{
    int i = 0;
    for (i = 0; i < TUYA_QSPI_NUM_MAX; i ++) {
        if(dma_id == qspi_infos[i].lcd_qspi_dma_id) {
            if(qspi_infos[i].qspi_enable == 1) {
                return i;
            }
        }
    }
    return 0xFF;
}

static void lcd_qspi_dma_finish_isr(dma_id_t dma_id)
{
    uint8_t port = dma_id_to_port(dma_id);
    if(port == 0xFF) {
        bk_printf("qspi irq port not right\r\n");
        return;
    }
    // up report
    if (qspi_infos[port].cb) {
        qspi_infos[port].cb(port, TUYA_QSPI_EVENT_TX);
    }
    bk_lcd_qspi_quad_write_stops(port);
}

// rx isr callback
// rx isr callback
static void qspi_tx_callback_dispatch(TUYA_QSPI_NUM_E id, void *param)
{
    if (qspi_infos[id].cb) {
        qspi_infos[id].cb((TUYA_QSPI_NUM_E)id, TUYA_QSPI_EVENT_TX);
    }
}

// tx isr callback
static void qspi_rx_callback_dispatch(TUYA_QSPI_NUM_E id, void *param)
{
    if (qspi_infos[id].cb) {
        qspi_infos[id].cb((TUYA_QSPI_NUM_E)id, TUYA_QSPI_EVENT_RX);
    }
}


static bk_err_t lcd_qspi_dma_common_init(uint8_t port)
{
    bk_err_t ret = BK_OK;
	TKL_DMA_CONFIG_T dma_config = {0};
	dma_config.mode = DMA_WORK_MODE_REPEAT;
	dma_config.src.dev = DMA_DEV_DTCM;
	dma_config.src.width = TKL_DMA_DATA_WIDTH_32BITS;
	dma_config.dst.width = TKL_DMA_DATA_WIDTH_32BITS;
	dma_config.src.addr_inc_en = TKL_DMA_ADDR_INC_ENABLE;
	dma_config.src.addr_loop_en = TKL_DMA_ADDR_LOOP_ENABLE;
    dma_config.src.brust_len = BURST_LEN_INC16;
    dma_config.dst.addr_inc_en = TKL_DMA_ADDR_INC_ENABLE;
    dma_config.dst.addr_loop_en = TKL_DMA_ADDR_LOOP_ENABLE;
    dma_config.dst.brust_len = BURST_LEN_INC16;
    if (port == TUYA_QSPI_NUM_0) {
	    dma_config.dst.start_addr = LCD_QSPI0_DATA_ADDR;
    } else if (port == TUYA_QSPI_NUM_1) {
	    dma_config.dst.start_addr = LCD_QSPI1_DATA_ADDR;
    } else {
        bk_printf("unsupported lcd qspi id\r\n");
        return BK_FAIL;
    }
	dma_config.dst.dev = DMA_DEV_DTCM;
    dma_config.dev_id = DMA_DEV_DTCM;
    ret = tkl_dma_init(&qspi_infos[port].lcd_qspi_dma_id, &dma_config);
    if (ret != BK_OK) {
        bk_printf("dma driver init failed!\r\n");
        return BK_FAIL;
    }

    if ((qspi_infos[port].lcd_qspi_dma_id < DMA_ID_0) || (qspi_infos[port].lcd_qspi_dma_id >= DMA_ID_MAX)) {
        bk_printf("lcd qspi dma malloc failed!\r\n");
        return BK_FAIL;
    }
    tkl_dma_register_isr(qspi_infos[port].lcd_qspi_dma_id, lcd_qspi_dma_finish_isr);
#if (CONFIG_SPE)
    bk_dma_set_src_sec_attr(qspi_infos[port].lcd_qspi_dma_id, DMA_ATTR_SEC);
    bk_dma_set_dest_sec_attr(qspi_infos[port].lcd_qspi_dma_id, DMA_ATTR_SEC);
    bk_dma_set_dest_burst_len(qspi_infos[port].lcd_qspi_dma_id, BURST_LEN_INC16);
    bk_dma_set_src_burst_len(qspi_infos[port].lcd_qspi_dma_id, BURST_LEN_INC16);
#endif

    return BK_OK;
}



static bk_err_t lcd_qspi_common_deinit(uint8_t port)
{
    BK_LOG_ON_ERR(tkl_dma_deinit(qspi_infos[port].lcd_qspi_dma_id));
    return BK_OK;
}

static bk_err_t lcd_qspi_quad_write_enable(qspi_id_t qspi_id)
{
    qspi_hal_set_cmd_a_l(&s_tkl_qspi[qspi_id].hal, 0x00002C00);
    qspi_hal_set_cmd_a_h(&s_tkl_qspi[qspi_id].hal, 0x10000100);
    qspi_hal_set_cmd_a_cfg1(&s_tkl_qspi[qspi_id].hal, 0xEAAA);
    qspi_hal_set_cmd_a_cfg2(&s_tkl_qspi[qspi_id].hal, 0x80008000);
    return BK_OK;
}

#if CONFIG_SOC_BK7236XX
static bk_err_t lcd_qspi_refresh_by_line_lcd_head_config(qspi_id_t qspi_id, const lcd_device_t *device)
{
    uint8_t *cmd = NULL;
    uint32_t head_cmd[4];
    uint8_t i;

    cmd = device->qspi->pixel_write_config.cmd;
    for (i = 0; i < device->qspi->pixel_write_config.cmd_len; i++) {
        if (0 == cmd[i]) {
            head_cmd[i] = 0x0;
            continue;
        } else {
            uint8_t cmd_temp = cmd[i];
            cmd_temp = ((cmd_temp >> 4) & 0x0F) | ((cmd_temp << 4) & 0xF0);
            cmd_temp = ((cmd_temp >> 2) & 0x33) | ((cmd_temp << 2) & 0xCC);
            cmd_temp = ((cmd_temp >> 1) & 0x55) | ((cmd_temp << 1) & 0xAA);

            head_cmd[i] = ((cmd_temp << 21) & 0x10000000) | ((cmd_temp << 18) & 0x01000000) |
                          ((cmd_temp << 15) & 0x00100000) | ((cmd_temp << 12) & 0x00010000) |
                          ((cmd_temp << 9) & 0x00001000) | ((cmd_temp << 6) & 0x00000100) |
                          ((cmd_temp << 3) & 0x00000010) | (cmd_temp & 0x00000001);
        }
    }

    qspi_hal_enable_soft_reset(&s_tkl_qspi[qspi_id].hal);
    qspi_hal_set_lcd_head_cmd0(&s_tkl_qspi[qspi_id].hal, head_cmd[0]);
    qspi_hal_set_lcd_head_cmd1(&s_tkl_qspi[qspi_id].hal, head_cmd[1]);
    qspi_hal_set_lcd_head_cmd2(&s_tkl_qspi[qspi_id].hal, head_cmd[2]);
    qspi_hal_set_lcd_head_cmd3(&s_tkl_qspi[qspi_id].hal, head_cmd[3]);
    qspi_hal_set_lcd_head_resolution(&s_tkl_qspi[qspi_id].hal, device->qspi->refresh_config.line_len * 2, device->ppi & 0xFFFF);

    qspi_hal_enable_lcd_head_selection_without_ram(&s_tkl_qspi[qspi_id].hal);
    qspi_hal_set_lcd_head_len(&s_tkl_qspi[qspi_id].hal, 0x20);
    qspi_hal_set_lcd_head_dly(&s_tkl_qspi[qspi_id].hal, (device->qspi->clk >> 1) + 6);

    return BK_OK;
}

static bk_err_t lcd_qspi_get_dma_repeat_once_len(uint32_t frame_len)
{
    uint32_t len = 0;
    uint32_t value = 0;
    uint8_t i = 0;

    for (i = 4; i < 13; i++) {
        len = frame_len / i;
        if (len <= 0x10000) {
            value = frame_len % i;
            if (!value) {
                return len;
            }
        }
    }

    bk_printf("%s Error dma length, please check the resolution of qspi lcd\r\n", __func__);

    return len;
}

#endif


static void qspi_interrupt_enable(qspi_id_t id)
{
	switch(id)
	{
		case QSPI_ID_0:
			sys_drv_int_enable(QSPI0_INTERRUPT_CTRL_BIT);
			break;
#if (SOC_QSPI_UNIT_NUM > 1)
		case QSPI_ID_1:
			sys_drv_int_group2_enable(QSPI1_INTERRUPT_CTRL_BIT);
			break;
#endif
		default:
			break;
	}
}

static void qspi_interrupt_disable(qspi_id_t id)
{
	switch(id)
	{
		case QSPI_ID_0:
			sys_drv_int_disable(QSPI0_INTERRUPT_CTRL_BIT);
			break;
#if (SOC_QSPI_UNIT_NUM > 1)
		case QSPI_ID_1:
			sys_drv_int_group2_disable(QSPI1_INTERRUPT_CTRL_BIT);
			break;
#endif
		default:
			break;
	}
}

static void qspi_clock_disable(qspi_id_t id)
{
	switch(id)
	{
		case QSPI_ID_0:
			sys_drv_dev_clk_pwr_up(CLK_PWR_ID_QSPI_1, CLK_PWR_CTRL_PWR_DOWN);
			break;
#if (SOC_QSPI_UNIT_NUM > 1)
		case QSPI_ID_1:
			sys_drv_dev_clk_pwr_up(CLK_PWR_ID_QSPI_2, CLK_PWR_CTRL_PWR_DOWN);
			break;
#endif
		default:
			break;
	}
}

static void qspi_init_gpio(qspi_id_t id)
{
	switch (id) {
	case QSPI_ID_0:
		QSPI_SET_PIN(0);
		break;
#if (SOC_QSPI_UNIT_NUM > 1)
	case QSPI_ID_1:
		QSPI_SET_PIN(1);
		break;
#endif
#if (SOC_QSPI_UNIT_NUM > 2)
	case QSPI_ID_2:
		QSPI_SET_PIN(2);
		break;
#endif
	default:
		break;
	}
}

static void qspi_clock_enable(qspi_id_t id)
{
	switch(id)
	{
		case QSPI_ID_0:
			sys_drv_dev_clk_pwr_up(CLK_PWR_ID_QSPI_1, CLK_PWR_CTRL_PWR_UP);
			break;
#if (SOC_QSPI_UNIT_NUM > 1)
		case QSPI_ID_1:
			sys_drv_dev_clk_pwr_up(CLK_PWR_ID_QSPI_2, CLK_PWR_CTRL_PWR_UP);
			break;
#endif
		default:
			break;
	}
}

 /**
  * @brief Init the QSPI
  * NOTE: 
  *
  * @param[in] port: qspi port, id index starts at 0
  * @param[in] cfg:  QSPI parameter settings
  *
  * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
  */
 OPERATE_RET tkl_qspi_init(TUYA_QSPI_NUM_E port, const TUYA_QSPI_BASE_CFG_T *cfg)
 {
    qspi_config_t config = {0};
    if ((port > TUYA_QSPI_NUM_MAX) || (cfg == NULL)) {
        return OPRT_INVALID_PARM;
    }

    if (qspi_infos[port].qspi_enable == 1) {
        bk_printf("qspi :%d already init \r\n", port);
        return OPRT_INVALID_PARM;
    }
    // if(bk_qspi_driver_init() != BK_OK)
    //     return OPRT_COM_ERROR;
 
    os_memset(&config, 0, sizeof(config));
    os_memset(&s_tkl_qspi[port], 0, sizeof(s_tkl_qspi));
    s_tkl_qspi[port].hal.id = port;
    qspi_hal_init(&s_tkl_qspi[port].hal);
    
    switch (cfg->freq_hz) {
        case 80000000:
            config.src_clk = QSPI_SCLK_480M;
            config.src_clk_div = 5;
            break;
        case 64000000:
            config.src_clk = QSPI_SCLK_320M;
            config.src_clk_div = 4;
            break;
        case 60000000:
            config.src_clk = QSPI_SCLK_480M;
            config.src_clk_div = 7;
            break;
        case 53000000:
            config.src_clk = QSPI_SCLK_480M;
            config.src_clk_div = 8;
            break;
        case 48000000:
            config.src_clk = QSPI_SCLK_480M;
            config.src_clk_div = 9;
            break;
        case 40000000:
            config.src_clk = QSPI_SCLK_480M;
            config.src_clk_div = 11;
            break;
        case 32000000:
            config.src_clk = QSPI_SCLK_320M;
            config.src_clk_div = 9;
            break;
        case 30000000:
            config.src_clk = QSPI_SCLK_480M;
            config.src_clk_div = 15;
            break;
        default:
            // for lcd config
            //  config.src_clk = QSPI_SCLK_480M;
            //  config.src_clk_div = 11;
            config.src_clk = QSPI_SCLK_480M;
            config.src_clk_div = TUYA_QSPI_CLK_DIV;
            config.clk_div = (QSPI_INIT_CLK_480M / TUYA_QSPI_CLK_DIV / cfg->freq_hz);
            break;
    }

    if (port == QSPI_ID_0) {
		bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_AHBP_QSPI, PM_POWER_MODULE_STATE_ON);
#if (SOC_QSPI_UNIT_NUM > 1)
	} else if (port == QSPI_ID_1) {
		bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_AHBP_QSPI1, PM_POWER_MODULE_STATE_ON);
#endif
	}

    qspi_clock_enable(port);
    qspi_init_gpio(port);
    qspi_interrupt_enable(port);
    qspi_hal_init_common(&s_tkl_qspi[port].hal);

    switch (config.src_clk) {
    case QSPI_SCLK_320M:
        sys_drv_qspi_clk_sel(port, QSPI_CLK_320M);
        break;
    case QSPI_SCLK_480M:
        sys_drv_qspi_clk_sel(port, QSPI_CLK_480M);
        break;
    default:
        sys_drv_qspi_clk_sel(port, QSPI_CLK_480M);
        break;
    }
    sys_drv_qspi_set_src_clk_div(port, config.src_clk_div);
    qspi_hal_set_clk_div(&s_tkl_qspi[port].hal, config.clk_div);

    memset(&qspi_infos[port], 0, sizeof(struct qspi_init_config));
    qspi_infos[port].qspi_enable = 1;
    qspi_infos[port].lcd_qspi_dma_id = DMA_ID_MAX;

    qspi_hal_set_cmd_a_l(&s_tkl_qspi[port].hal, 0);
    qspi_hal_set_cmd_a_h(&s_tkl_qspi[port].hal, 0);
    qspi_hal_set_cmd_a_cfg1(&s_tkl_qspi[port].hal, 0x3);
    qspi_infos[port].dma_data_lines = cfg->dma_data_lines;
    if (cfg->dma_data_lines == TUYA_QSPI_4WIRE) {
        // lcd_qspi_quad_write_enable(port);
        qspi_hal_set_cmd_a_cfg2(&s_tkl_qspi[port].hal, 0x8000);
    }else if(cfg->dma_data_lines == TUYA_QSPI_2WIRE) {
        qspi_hal_set_cmd_a_cfg2(&s_tkl_qspi[port].hal, 0x4000);
    }else {
        qspi_hal_set_cmd_a_cfg2(&s_tkl_qspi[port].hal, 0x0);
    }
    qspi_hal_enable_soft_reset(&s_tkl_qspi[port].hal);
    // if (cfg->refresh_method == TUYA_QSPI_LCD_REFRESH_BY_LINE) {
        // lcd_qspi_refresh_by_line_lcd_head_config(port, device);
        // lcd_refresh_type = TUYA_QSPI_LCD_REFRESH_BY_LINE;
    // }
    if (cfg->use_dma) {
        lcd_qspi_dma_common_init(port);
        qspi_infos[port].is_send_use_dma = TRUE;
        // dma config
    // delay
#if CONFIG_LCD_QSPI_SPD2010
        // GPIO_UP(GPIO_5);
        // rtos_delay_milliseconds(20); // 20ms
        // GPIO_DOWN(GPIO_5);
        // rtos_delay_milliseconds(200); // 200ms
        // GPIO_UP(GPIO_5);
        // rtos_delay_milliseconds(120); // 120ms
#endif
    }
    //config cpol cpha
    qspi_hal_set_cpol_and_cpha(&s_tkl_qspi[port].hal, cfg->mode);

    return OPRT_OK;
 }
 
 /**
  * @brief Deinit the QSPI driver
  * NOTE: 
  *
  * @param[in] port: qspi port, id index starts at 0
  *
  * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
  */
 OPERATE_RET tkl_qspi_deinit(TUYA_QSPI_NUM_E port)
 {
     if (port > TUYA_QSPI_NUM_MAX) {
         return OPRT_INVALID_PARM;
     }

    //  if (bk_qspi_driver_deinit() != BK_OK)
    //     return OPRT_COM_ERROR;
    if (qspi_infos[port].is_send_use_dma == TRUE) {
        lcd_qspi_common_deinit(port);
    }
    //  if(bk_qspi_deinit(port) != BK_OK)
    //     return OPRT_COM_ERROR;

    qspi_hal_deinit_common(&s_tkl_qspi[port].hal);
    qspi_interrupt_disable(port);
    qspi_clock_disable(port);

    if (port == QSPI_ID_0) {
        bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_AHBP_QSPI, PM_POWER_MODULE_STATE_OFF);
#if (SOC_QSPI_UNIT_NUM > 1)
    } else if (port == QSPI_ID_1) {
        bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_AHBP_QSPI1, PM_POWER_MODULE_STATE_OFF);
#endif
    }
    
    memset(&qspi_infos[port], 0, sizeof(struct qspi_init_config));
    qspi_infos[port].qspi_enable = 0;

     return OPRT_OK;
 }
 
 /**
  * @brief qspi write data, dma send or not
  * NOTE: 
  *
  * @param[in] port: qspi port, id index starts at 0
  * @param[in] data:  address of buffer
  * @param[in] size:  size of read
  *
  * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
  */

static bk_err_t bk_lcd_qspi_quad_write_starts(qspi_id_t qspi_id)
{
#if 0
    uint32_t cmd_c_h = 0;

    // qspi_hal_force_spi_cs_low_enable(&s_tkl_qspi[qspi_id].hal);
    qspi_hal_set_cmd_c_h(&s_tkl_qspi[qspi_id].hal, 0);

    for (uint8_t i = 0; i < 4; i++) {
        cmd_c_h = qspi_hal_get_cmd_c_h(&s_tkl_qspi[qspi_id].hal);
        cmd_c_h |= ((cmd[i]) << i * 8);
        qspi_hal_set_cmd_c_h(&s_tkl_qspi[qspi_id].hal, cmd_c_h);
    }

    if (addr_is_4wire) {
        qspi_hal_set_cmd_c_cfg1(&s_tkl_qspi[qspi_id].hal, 0x3A8); //cmd0: 1, cmd1: 4，cmd2: 4, cmd3:4
    } else {
        qspi_hal_set_cmd_c_cfg1(&s_tkl_qspi[qspi_id].hal, 0x300);
    }
    qspi_hal_cmd_c_start(&s_tkl_qspi[qspi_id].hal);
    qspi_hal_wait_cmd_done(&s_tkl_qspi[qspi_id].hal);
#endif


#if (CONFIG_SOC_BK7236XX)
    qspi_hal_io_cpu_mem_select(&s_tkl_qspi[qspi_id].hal, 1);
#endif

    qspi_hal_disable_cmd_sck_enable(&s_tkl_qspi[qspi_id].hal);

    return BK_OK;
}

bk_err_t bk_lcd_qspi_quad_write_stops(qspi_id_t qspi_id)
{
    qspi_hal_disable_cmd_sck_disable(&s_tkl_qspi[qspi_id].hal);
    // qspi_hal_force_spi_cs_low_disable(&s_tkl_qspi[qspi_id].hal);

#if (CONFIG_SOC_BK7236XX)
    qspi_hal_io_cpu_mem_select(&s_tkl_qspi[qspi_id].hal, 0);
#endif

    return BK_OK;
}

 OPERATE_RET tkl_qspi_send(TUYA_QSPI_NUM_E port, VOID_T *data, UINT32_T size)
 {
    bk_err_t ret = BK_OK;
    TUYA_QSPI_CMD_T sd_command;

    if ((data == NULL) || (port > TUYA_QSPI_NUM_MAX)) {
        return OPRT_INVALID_PARM;
    }
    if (qspi_infos[port].qspi_enable != 1) {
        bk_printf("qspi :%d not init \r\n", port);
        return OPRT_COM_ERROR;
    }

    if (size <= 256) {
        memset(&sd_command, 0 ,sizeof(TUYA_QSPI_CMD_T));
        sd_command.op = TUYA_QSPI_WRITE;
        sd_command.addr_size = 0;
        // bk_printf("(uint8_t *)data[0]:%x,%x \r\n", (uint8_t *)data, *(uint8_t *)data);
        memcpy(&sd_command.cmd[0], data, 1);
        sd_command.cmd_lines = qspi_infos[port].dma_data_lines;
        sd_command.cmd_size = 1;
        sd_command.addr_lines = qspi_infos[port].dma_data_lines;
        sd_command.data_size = size - 1;
        sd_command.data = (UINT8_T *)(data + 1);
        sd_command.data_lines = qspi_infos[port].dma_data_lines;
        ret = tkl_qspi_comand(TUYA_QSPI_NUM_0,  &sd_command);
        if ((qspi_infos[port].cb) && (ret == OPRT_OK)) {
            qspi_infos[port].cb(port, TUYA_QSPI_EVENT_TX);
        }
        return ret;
    } else {
        if ((qspi_infos[port].is_send_use_dma == TRUE)) {
            bk_lcd_qspi_quad_write_starts(port);
            tkl_dma_write(qspi_infos[port].lcd_qspi_dma_id, (void *)data, size);
            bk_delay_us(5);
        }
    }
    return OPRT_OK;
 }
 
 /**
  * @brief qspi read from addr by mapping mode
  * NOTE: 
  *
  * @param[in] port: qspi port, id index starts at 0
  * @param[out] data:  address of buffer
  * @param[in] size:  size of read
  *
  * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
  */
 OPERATE_RET tkl_qspi_recv(TUYA_QSPI_NUM_E port, VOID_T *data, UINT32_T size)
 {
     bk_err_t ret = BK_OK;

     if ((data == NULL) || (port > TUYA_QSPI_NUM_MAX) || (size > MAX_QSPI_FIFO_SIZE)) {
         return OPRT_INVALID_PARM;
     }
     if (qspi_infos[port].qspi_enable != 1) {
        bk_printf("qspi :%d not init \r\n", port);
        return OPRT_COM_ERROR;
    }

     if (size <= 256) {
        ret = qspi_hal_io_read(&s_tkl_qspi[port].hal, data, size);
        if (ret != BK_OK)
            return OPRT_COM_ERROR;
     }else {
        return OPRT_NOT_SUPPORTED;
     }

     return OPRT_OK;
 }
 
 /**
  * @brief qspi command send
  * NOTE: 
  *
  * @param[in] port: qspi port, id index starts at 0
  * @param[in] command:  qspi command configure
  *
  * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
  */
typedef union {
    UINT32_T data;
    CHAR_T ponit[4];
}data_union_s;

static uint32_t line_data_get(TUYA_QSPI_WIRE_MODE_E cmdlines, uint8_t cmd_len, TUYA_QSPI_WIRE_MODE_E addrlines, uint8_t addr_len)
{
    int i = 0;
    uint32_t clines = 0;
    uint32_t alines = 0;
    uint32_t data_get = 0x3;

    if (addrlines == TUYA_QSPI_4WIRE) {
        alines = 0x2;
    }else if(addrlines == TUYA_QSPI_2WIRE) {
        alines = 0x1;
    }
    
    for (i = 0; i < addr_len; i ++) {
        data_get = data_get << 2;
        data_get |= alines;
    }

    if (cmdlines == TUYA_QSPI_4WIRE) {
        clines = 0x2;
    }else if(cmdlines == TUYA_QSPI_2WIRE) {
        clines = 0x1;
    }
    for (i = 0; i < cmd_len; i ++) {
        data_get = data_get << 2;
        data_get |= clines;
    }

    return data_get;
}


 OPERATE_RET tkl_qspi_comand(TUYA_QSPI_NUM_E port, TUYA_QSPI_CMD_T *command)
 {
    bk_err_t ret = BK_OK;
    data_union_s union_data;
    UINT32_T c_l = 0;
    UINT32_T c_h = 0;

    qspi_cmd_t cmd = {0};
    if ((command == NULL) || (port > TUYA_QSPI_NUM_MAX)) {
        return OPRT_INVALID_PARM;
    }
    if (qspi_infos[port].qspi_enable != 1) {
        bk_printf("qspi :%d not init \r\n", port);
        return OPRT_COM_ERROR;
    }
    if((command->data_size > 0x100) || (command->cmd_size > 4) || (command->addr_size > 4)){
        return OPRT_INVALID_PARM;
    }

#if 0
     cmd.op = command->op;
     cmd.cmd = command->cmd;
     cmd.addr = command->addr;
     cmd.addr_valid_bit = command->addr_size;
     cmd.data_len = command->data_len;
     cmd.dummy_cycle = command->dummy_cycle;
     cmd.wire_mode = command->data_lines;
     cmd.work_mode = INDIRECT_MODE;
     cmd.device = QSPI_FLASH;

     ret = bk_qspi_command(port, &cmd);
     if (ret != BK_OK)
         return OPRT_COM_ERROR;
     return OPRT_OK;
#endif
    UINT32_T  ucmd = 0;
    UINT32_T  uaddr = 0;
    // get cmd
    union_data.data = 0;
    if (command->cmd_size != 0) {
        memcpy(union_data.ponit, command->cmd, command->cmd_size);
        ucmd = (UINT32_T)union_data.data;
    }else {
        bk_printf("cmd size is 0\r\n");
        // return -1;
    }
    // get addr
    union_data.data = 0;
    if (command->addr_size != 0) {
        memcpy(union_data.ponit, command->addr, command->addr_size);
        uaddr = (UINT32_T)union_data.data;
    }

    if (command->op == TUYA_QSPI_WRITE) {
        qspi_hal_set_cmd_c_l(&s_tkl_qspi[port].hal, 0);
        qspi_hal_set_cmd_c_h(&s_tkl_qspi[port].hal, 0);
        qspi_hal_set_cmd_c_cfg1(&s_tkl_qspi[port].hal, 0);
        qspi_hal_set_cmd_c_cfg2(&s_tkl_qspi[port].hal, 0);
        // bk_printf("write addr:%x, ucmd:%x, addr_size:%d, cmd_size:%d, data_size:%d \r\n", uaddr, ucmd, command->addr_size, command->cmd_size, command->data_size);

        c_h = ucmd;
        INT8_T off = 4 - command->cmd_size;  //cmd : 0 1 2
        if(off > 0) {
            c_h = (UINT32_T) (ucmd | (uaddr << command->cmd_size * 8));
        }else {//command->cmd_size >= 4
            bk_printf("cmd size is out of 4 \r\n");
            return OPRT_INVALID_PARM;
        }
        qspi_hal_set_cmd_c_h(&s_tkl_qspi[port].hal, c_h);
        if(command->cmd_size + command->addr_size > 4) {
            off = command->cmd_size + command->addr_size - 4;
            c_l = uaddr >> ((4 - off) * 8);
            qspi_hal_set_cmd_c_l(&s_tkl_qspi[port].hal, c_l);
        }
        uint32_t date_lines = line_data_get(command->cmd_lines, command->cmd_size, command->addr_lines, command->addr_size);
        qspi_hal_set_cmd_c_cfg1(&s_tkl_qspi[port].hal, date_lines); // all 0 line

        if (command->data_size > 0 && command->data_size <= 0x100) {
            qspi_hal_set_cmd_c_cfg2(&s_tkl_qspi[port].hal, command->data_size << 2);
            qspi_hal_io_write(&s_tkl_qspi[port].hal, command->data, command->data_size);
            if (command->data_lines == TUYA_QSPI_1WIRE) {
                qspi_hal_set_cmd_c_data_line(&s_tkl_qspi[port].hal, QSPI_1WIRE);
            }else if (command->data_lines == TUYA_QSPI_2WIRE) {
                qspi_hal_set_cmd_c_data_line(&s_tkl_qspi[port].hal, QSPI_2WIRE);
            }else {
                qspi_hal_set_cmd_c_data_line(&s_tkl_qspi[port].hal, QSPI_4WIRE);
            }
        } 
        // down
        if (command->dummy_cycle) {
            qspi_hal_set_cmd_c_dummy_clock(&s_tkl_qspi[port].hal, command->dummy_cycle);
            qspi_hal_set_cmd_c_dummy_mode(&s_tkl_qspi[port].hal, 4);
        } else {
            qspi_hal_set_cmd_c_dummy_mode(&s_tkl_qspi[port].hal, 0);
        }
        // bk_printf("tuya write c_l:%x, c_h:%x, cfg1:%x, cfg2:%x \r\n", qspi_hal_get_cmd_c_l(&s_tkl_qspi[port].hal), qspi_hal_get_cmd_c_h(&s_tkl_qspi[port].hal), qspi_hal_get_cmd_c_cfg1(&s_tkl_qspi[port].hal), qspi_hal_get_cmd_c_cfg2(&s_tkl_qspi[port].hal));
        //up
        qspi_hal_cmd_c_start(&s_tkl_qspi[port].hal);
        qspi_hal_wait_cmd_done(&s_tkl_qspi[port].hal);
        qspi_hal_set_cmd_c_cfg1(&s_tkl_qspi[port].hal, 0);
        qspi_hal_set_cmd_c_cfg2(&s_tkl_qspi[port].hal, 0);
    }else {  // read
        qspi_hal_set_cmd_d_l(&s_tkl_qspi[port].hal, 0);
        qspi_hal_set_cmd_d_h(&s_tkl_qspi[port].hal, 0);
        qspi_hal_set_cmd_d_cfg1(&s_tkl_qspi[port].hal, 0);
        qspi_hal_set_cmd_d_cfg2(&s_tkl_qspi[port].hal, 0);

        // bk_printf("read addr:%x, ucmd:%x, addr_size:%d, cmd_size:%d, data_size:%d \r\n", uaddr, ucmd, command->addr_size, command->cmd_size, command->data_size);
        c_h = ucmd;
        INT8_T off = 4 - command->cmd_size;  //cmd : 0 1 2
        if(off > 0) {
            c_h = (UINT32_T) (ucmd | (uaddr << command->cmd_size * 8));
        }else {//command->cmd_size >= 4
            bk_printf("cmd size is out of 4 \r\n");
            return OPRT_INVALID_PARM;
        }
        qspi_hal_set_cmd_d_h(&s_tkl_qspi[port].hal, c_h);
        if(command->cmd_size + command->addr_size > 4) {
            off = command->cmd_size + command->addr_size - 4;
            c_l = uaddr >> ((4 - off) * 8);
            qspi_hal_set_cmd_d_l(&s_tkl_qspi[port].hal, c_l);
        }
        uint32_t date_lines = line_data_get(command->cmd_lines, command->cmd_size, command->addr_lines, command->addr_size);
        qspi_hal_set_cmd_d_cfg1(&s_tkl_qspi[port].hal, date_lines); // all 0 line
        // data len
        qspi_hal_set_cmd_d_data_length(&s_tkl_qspi[port].hal, command->data_size);

        if (command->data_lines == TUYA_QSPI_1WIRE) {
            qspi_hal_set_cmd_d_data_line(&s_tkl_qspi[port].hal, QSPI_1WIRE);
        }else if (command->data_lines == TUYA_QSPI_2WIRE) {
            qspi_hal_set_cmd_d_data_line(&s_tkl_qspi[port].hal, QSPI_2WIRE);
        }else {
            qspi_hal_set_cmd_d_data_line(&s_tkl_qspi[port].hal, QSPI_4WIRE);
        }
        // dummy cycle
        qspi_hal_set_cmd_d_dummy_clock(&s_tkl_qspi[port].hal, command->dummy_cycle);
        if (command->dummy_cycle) {
            qspi_hal_set_cmd_d_dummy_mode(&s_tkl_qspi[port].hal, command->cmd_size + command->addr_size);
        }
        // bk_printf("tuya read d_l:%x, d_h:%x, cfg1:%x, cfg2:%x \r\n", qspi_hal_get_cmd_d_l(&s_tkl_qspi[port].hal), qspi_hal_get_cmd_d_h(&s_tkl_qspi[port].hal), qspi_hal_get_cmd_d_cfg1(&s_tkl_qspi[port].hal), qspi_hal_get_cmd_d_cfg2(&s_tkl_qspi[port].hal));

        qspi_hal_cmd_d_start(&s_tkl_qspi[port].hal);
        qspi_hal_wait_cmd_done(&s_tkl_qspi[port].hal);

        ret = qspi_hal_io_read(&s_tkl_qspi[port].hal, command->data, command->data_size);
        if (ret != BK_OK)
            return OPRT_COM_ERROR;
    }
    return OPRT_OK;
 }
 
 /**
  * @brief qspi irq init
  * NOTE: call this API will not enable interrupt
  *
  * @param[in] port: qspi port, id index starts at 0
  * @param[in] cb:  qspi irq cb
  *
  * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
  */
 OPERATE_RET tkl_qspi_irq_init(TUYA_QSPI_NUM_E port, TUYA_QSPI_IRQ_CB cb)
 {
     if (port > TUYA_QSPI_NUM_MAX) {
         return OPRT_INVALID_PARM;
     }
     if (qspi_infos[port].is_send_use_dma != TRUE){
        return OPRT_NOT_SUPPORTED;
     }
     qspi_infos[port].cb = cb;
     qspi_infos[port].irq_enable = 0;
 
     return OPRT_OK;
 }
 
 /**
  * @brief qspi irq enable
  *
  * @param[in] port: qspi port id, id index starts at 0
  *
  * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
  */
 OPERATE_RET tkl_qspi_irq_enable(TUYA_QSPI_NUM_E port)
 {
     if (port > TUYA_QSPI_NUM_MAX) {
         return OPRT_INVALID_PARM;
     }
     if (qspi_infos[port].is_send_use_dma != TRUE){
        return OPRT_NOT_SUPPORTED;
     }
     bk_qspi_register_tx_isr(qspi_tx_callback_dispatch, NULL);
     bk_qspi_register_rx_isr(qspi_rx_callback_dispatch, NULL);
 
     qspi_infos[port].irq_enable = 1;
 
     return OPRT_OK;
 }
 
 /**
  * @brief qspi irq disable
  *
  * @param[in] port: qspi port id, id index starts at 0
  *k
  * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
  */
 OPERATE_RET tkl_qspi_irq_disable(TUYA_QSPI_NUM_E port)
 {
     if (port > TUYA_QSPI_NUM_MAX) {
         return OPRT_INVALID_PARM;
     }
     if (qspi_infos[port].is_send_use_dma != TRUE){
        return OPRT_NOT_SUPPORTED;
     }
     bk_qspi_register_tx_isr(NULL, NULL);
     bk_qspi_register_rx_isr(NULL, NULL);
 
     qspi_infos[port].irq_enable = 0;
 
     return OPRT_OK;
 }


OPERATE_RET tkl_qspi_force_cs_pin(TUYA_QSPI_NUM_E port, TUYA_GPIO_LEVEL_E level)
{
    if (qspi_infos[port].qspi_enable != 1) {
        bk_printf("qspi :%d not init \r\n", port);
        return OPRT_COM_ERROR;
    }

    if (level == TUYA_GPIO_LEVEL_LOW)
        qspi_hal_force_spi_cs_low_enable(&s_tkl_qspi[port].hal);
    else 
        qspi_hal_force_spi_cs_low_disable(&s_tkl_qspi[port].hal);
    return OPRT_OK;
}



