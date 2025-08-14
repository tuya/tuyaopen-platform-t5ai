/**
 * @file tkl_qspi.c
 * @brief default weak implements of tuya pin
 *
 * @copyright Copyright 2020-2021 Tuya Inc. All Rights Reserved.
 *
 */

#include "driver/dma.h"
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

static dma_id_t lcd_qspi_dma_id = DMA_ID_MAX;
static TUYA_QSPI_LCD_REFRESH_METHOD_E lcd_refresh_type = TUYA_QSPI_LCD_REFRESH_BY_FRAME;
static uint32_t dma_repeat_once_len = 0;
extern media_debug_t *media_debug;
static beken_semaphore_t lcd_qspi_semaphore = NULL;
static BOOL_T is_send_use_dma = FALSE;
extern void bk_delay_us(UINT32 us);


#define TUYA_QSPI_CLK_DIV   (0x2)
#define QSPI_INIT_CLK_480M (480000000)
struct qspi_irq_config {
    uint8_t irq_enable;
    TUYA_QSPI_IRQ_CB cb;
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
static struct qspi_irq_config qspi_irq[TUYA_QSPI_NUM_MAX] = {0};
// static TUYA_QSPI_BASE_CFG_T qspi_base_config[TUYA_QSPI_NUM_MAX] = {0};

static void lcd_qspi_dma_finish_isr(void)
{
    bk_err_t ret = BK_OK;
    uint32_t value = 0;

    value = bk_dma_get_repeat_wr_pause(lcd_qspi_dma_id);
    if (value) {
        media_debug->isr_lcd++;
        bk_dma_stop(lcd_qspi_dma_id);
        //bk_lcd_qspi_quad_write_stop();

        ret = rtos_set_semaphore(&lcd_qspi_semaphore);
        if (ret != BK_OK) {
            bk_printf("lcd qspi semaphore set failed\r\n");
            return;
        }
    }
}

// rx isr callback
// rx isr callback
static void qspi_tx_callback_dispatch(TUYA_QSPI_NUM_E id, void *param)
{
    if (qspi_irq[id].cb) {
        qspi_irq[id].cb((TUYA_QSPI_NUM_E)id, TUYA_QSPI_EVENT_TX);
    }
}

// tx isr callback
static void qspi_rx_callback_dispatch(TUYA_QSPI_NUM_E id, void *param)
{
    if (qspi_irq[id].cb) {
        qspi_irq[id].cb((TUYA_QSPI_NUM_E)id, TUYA_QSPI_EVENT_RX);
    }
}


static bk_err_t qspi_pin_hardware_reset(void)
{
    gpio_dev_unmap(LCD_QSPI_RESET_PIN);
    gpio_dev_map(LCD_QSPI_RESET_PIN, 0);
    bk_gpio_enable_pull(LCD_QSPI_RESET_PIN);
    bk_gpio_pull_up(LCD_QSPI_RESET_PIN);
    rtos_delay_milliseconds(10);
    bk_gpio_pull_down(LCD_QSPI_RESET_PIN);
    rtos_delay_milliseconds(10);
    bk_gpio_pull_up(LCD_QSPI_RESET_PIN);
    rtos_delay_milliseconds(120);

    return BK_OK;
}

static bk_err_t lcd_qspi_dma_common_init(void)
{
    bk_err_t ret = BK_OK;


    ret = rtos_init_semaphore(&lcd_qspi_semaphore, 1);
    if (ret != kNoErr) {
        bk_printf("lcd qspi semaphore init failed.\r\n");
        return BK_FAIL;
    }

    ret = bk_dma_driver_init();
    if (ret != BK_OK) {
        bk_printf("dma driver init failed!\r\n");
        return BK_FAIL;
    }

    lcd_qspi_dma_id = bk_dma_alloc(DMA_DEV_DTCM);
    if ((lcd_qspi_dma_id < DMA_ID_0) || (lcd_qspi_dma_id >= DMA_ID_MAX)) {
        bk_printf("lcd qspi dma malloc failed!\r\n");
        return BK_FAIL;
    }

#if (CONFIG_SPE)
    bk_dma_set_src_sec_attr(lcd_qspi_dma_id, DMA_ATTR_SEC);
    bk_dma_set_dest_sec_attr(lcd_qspi_dma_id, DMA_ATTR_SEC);
    bk_dma_set_dest_burst_len(lcd_qspi_dma_id, BURST_LEN_INC16);
    bk_dma_set_src_burst_len(lcd_qspi_dma_id, BURST_LEN_INC16);
#endif

    return BK_OK;
}



static bk_err_t lcd_qspi_common_deinit(void)
{
    bk_err_t ret = BK_OK;

    ret = rtos_deinit_semaphore(&lcd_qspi_semaphore);
    if (ret != kNoErr) {
        bk_printf("lcd qspi semaphore deinit failed.\r\n");
        return BK_FAIL;
    }

#if (CONFIG_SOC_BK7256XX)
    bk_dma2d_driver_deinit();
#elif CONFIG_SOC_BK7236XX
    bk_dma_free(DMA_DEV_DTCM, lcd_qspi_dma_id);
    BK_LOG_ON_ERR(bk_dma_driver_deinit());
#endif

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

/**
 * @brief qspi command send
 * NOTE: 
 *
 * @param[in] port: qspi port, id index starts at 0
 * @param[in] command:  qspi command configure
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
// OPERATE_RET tkl_qspi_comand(TUYA_QSPI_NUM_E port, TUYA_QSPI_CMD_T *command)
// {
//     bk_err_t ret = BK_OK;
//     qspi_cmd_t cmd = {0};
//     if ((command == NULL) || (port > TUYA_QSPI_NUM_MAX)) {
//         return OPRT_INVALID_PARM;
//     }
//     cmd.op = command->op;
//     cmd.cmd = command->cmd;
//     cmd.addr = command->addr;
//     cmd.addr_valid_bit = command->addr_size;
//     cmd.data_len = command->data_len;
//     cmd.dummy_cycle = command->dummy_cycle;
//     cmd.wire_mode = command->data_lines;
//     cmd.work_mode = INDIRECT_MODE;
//     cmd.device = QSPI_FLASH;
//     // ret = tuya_qspi_hal_command(port, &cmd);
//     ret = bk_qspi_command(port, &cmd);
//     if (ret != BK_OK)
//         return OPRT_COM_ERROR;
//     return OPRT_OK;
// }

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
#if 1
    qspi_config_t config = {0};
    if ((port > TUYA_QSPI_NUM_MAX) || (cfg == NULL)) {
        return OPRT_INVALID_PARM;
    }

    // if(bk_qspi_driver_init() != BK_OK)
    //     return OPRT_COM_ERROR;
 
    os_memset(&config, 0, sizeof(config));
    os_memset(&s_tkl_qspi, 0, sizeof(s_tkl_qspi));
    s_tkl_qspi[port].hal.id = port;
    qspi_hal_init(&s_tkl_qspi[port].hal);
     
    switch (cfg->baudrate) {
        case 80000000:
            config.src_clk = QSPI_SCLK_480M;
            config.src_clk_div = 5;
            BK_LOG_ON_ERR(bk_qspi_init(port, &config));
            break;
        case 64000000:
            config.src_clk = QSPI_SCLK_320M;
            config.src_clk_div = 4;
            BK_LOG_ON_ERR(bk_qspi_init(port, &config));
            break;
        case 60000000:
            config.src_clk = QSPI_SCLK_480M;
            config.src_clk_div = 7;
            BK_LOG_ON_ERR(bk_qspi_init(port, &config));
            break;
        case 53000000:
            config.src_clk = QSPI_SCLK_480M;
            config.src_clk_div = 8;
            BK_LOG_ON_ERR(bk_qspi_init(port, &config));
            break;
        case 48000000:
            config.src_clk = QSPI_SCLK_480M;
            config.src_clk_div = 9;
            BK_LOG_ON_ERR(bk_qspi_init(port, &config));
            break;
        case 40000000:
            config.src_clk = QSPI_SCLK_480M;
            config.src_clk_div = 11;
            BK_LOG_ON_ERR(bk_qspi_init(port, &config));
            break;
        case 32000000:
            config.src_clk = QSPI_SCLK_320M;
            config.src_clk_div = 9;
            BK_LOG_ON_ERR(bk_qspi_init(port, &config));
            break;
        case 30000000:
            config.src_clk = QSPI_SCLK_480M;
            config.src_clk_div = 15;
            BK_LOG_ON_ERR(bk_qspi_init(port, &config));
            break;
        default:
            // for lcd config
            //  config.src_clk = QSPI_SCLK_480M;
            //  config.src_clk_div = 11;
            //  BK_LOG_ON_ERR(bk_qspi_init(qspi_id, &config));
            config.src_clk = QSPI_SCLK_480M;
            config.src_clk_div = TUYA_QSPI_CLK_DIV;
            config.clk_div = (QSPI_INIT_CLK_480M / TUYA_QSPI_CLK_DIV / cfg->baudrate);
            BK_LOG_ON_ERR(bk_qspi_init(port, &config));
            break;
    }
    // // need call in app
    // qspi_pin_hardware_reset();

    if (cfg->type == TUYA_QSPI_TYPE_LCD) {
        lcd_qspi_quad_write_enable(port);
    }

    qspi_hal_enable_soft_reset(&s_tkl_qspi[port].hal);
    // if (cfg->refresh_method == TUYA_QSPI_LCD_REFRESH_BY_LINE) {
        // lcd_qspi_refresh_by_line_lcd_head_config(port, device);
        // lcd_refresh_type = TUYA_QSPI_LCD_REFRESH_BY_LINE;
    // }
    if (cfg->is_dma) {
        lcd_qspi_dma_common_init();
        is_send_use_dma = TRUE;
        // dma config
    // delay
#if CONFIG_LCD_QSPI_SPD2010
        GPIO_UP(GPIO_5);
        rtos_delay_milliseconds(20); // 20ms
        GPIO_DOWN(GPIO_5);
        rtos_delay_milliseconds(200); // 200ms
        GPIO_UP(GPIO_5);
        rtos_delay_milliseconds(120); // 120ms
#endif
    }

    // memcpy(&qspi_base_config[port], cfg, sizeof(TUYA_QSPI_BASE_CFG_T));
#endif

    dma_repeat_once_len = 0;

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

     if (bk_qspi_driver_deinit() != BK_OK)
        return OPRT_COM_ERROR;

     if(bk_qspi_deinit(port) != BK_OK)
        return OPRT_COM_ERROR;
    
     is_send_use_dma = FALSE;
     dma_repeat_once_len = 0;
    //  memset(&qspi_base_config[port], 0, sizeof(TUYA_QSPI_BASE_CFG_T));
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

 OPERATE_RET tkl_qspi_send(TUYA_QSPI_NUM_E port, TUYA_QSPI_WIRE_MODE_E mode, VOID_T *data, UINT32_T size)
 {
    bk_err_t ret = BK_OK;

    if ((data == NULL) || (port > TUYA_QSPI_NUM_MAX)) {
        return OPRT_INVALID_PARM;
    }
 
    if (size <= 256) {
        ret = bk_qspi_write(port, data, size);
        if (ret != BK_OK)
            return OPRT_COM_ERROR;
        return  OPRT_OK;
    }


    bk_printf("tkl_qspi_send size:%d,  dma_repeat_once_len:%d, is_send_use_dma:%d \r\n", size, dma_repeat_once_len, is_send_use_dma);
    if ((size > 256) && (dma_repeat_once_len == 0) && (is_send_use_dma == TRUE)) {
        dma_repeat_once_len = lcd_qspi_get_dma_repeat_once_len(size);
        bk_printf("tkl dma_repeat_once_len = %d\r\n", dma_repeat_once_len);
        bk_dma_set_transfer_len(lcd_qspi_dma_id, dma_repeat_once_len);
        if (port == TUYA_QSPI_NUM_0) {
            dma_set_dst_pause_addr(lcd_qspi_dma_id, LCD_QSPI0_DATA_ADDR + size);
        } else if (port == TUYA_QSPI_NUM_1) {
            dma_set_dst_pause_addr(lcd_qspi_dma_id, LCD_QSPI1_DATA_ADDR + size);
        } else {
            bk_printf("unsupported lcd qspi id\r\n");
            return BK_FAIL;
        }
    }

    if ((size > 256) && (is_send_use_dma == TRUE) && (dma_repeat_once_len > 0)) {
        if (port == TUYA_QSPI_NUM_0) {
            bk_dma_stateless_judgment_configuration((void *)LCD_QSPI0_DATA_ADDR, (void *)data, size, lcd_qspi_dma_id, (void *)lcd_qspi_dma_finish_isr);
        } else if (port == TUYA_QSPI_NUM_1) {
            bk_dma_stateless_judgment_configuration((void *)LCD_QSPI1_DATA_ADDR, (void *)data, size, lcd_qspi_dma_id, (void *)lcd_qspi_dma_finish_isr);
        } else {
            bk_printf("unsupported lcd qspi id\r\n");
            return BK_FAIL;
        }

        dma_set_src_pause_addr(lcd_qspi_dma_id, (uint32_t)data + size);

        // if (device->qspi->refresh_method == LCD_QSPI_REFRESH_BY_LINE) {
        //     for (uint16_t i = 0; i < device->qspi->refresh_config.vsw; i++) {
        //         bk_lcd_qspi_send_cmd(port, device->qspi->reg_write_cmd, device->qspi->refresh_config.vsync_cmd, NULL, 0);
        //         bk_delay_us(40);
        //     }

        //     for (uint16_t i = 0; i < device->qspi->refresh_config.hfp; i++) {
        //         bk_lcd_qspi_send_cmd(port, device->qspi->reg_write_cmd, device->qspi->refresh_config.hsync_cmd, NULL, 0);
        //         bk_delay_us(40);
        //     }

        //     qspi_hal_clear_lcd_head(&s_tkl_qspi[port].hal, 1);
        //     qspi_hal_clear_lcd_head(&s_tkl_qspi[port].hal, 0);
        //     bk_lcd_qspi_quad_write_start(port, device->qspi->pixel_write_config, 0);
        //     bk_dma_start(lcd_qspi_dma_id);

        //     ret = rtos_get_semaphore(&lcd_qspi_semaphore, 3000);
        //     if (ret != kNoErr) {
        //         bk_printf("ret = %d, lcd qspi get semaphore failed!\r\n", ret);
        //         return BK_FAIL;
        //     }
        //     bk_delay_us(5);
        //     bk_lcd_qspi_quad_write_stop(port);

        //     for (uint16_t i = 0; i < device->qspi->refresh_config.hbp; i++) {
        //         bk_lcd_qspi_send_cmd(port, device->qspi->reg_write_cmd, device->qspi->refresh_config.hsync_cmd, NULL, 0);
        //         bk_delay_us(40);
        //     }
        // } else if (device->qspi->refresh_method == LCD_QSPI_REFRESH_BY_FRAME) {
            bk_lcd_qspi_quad_write_starts(port);
            bk_dma_start(lcd_qspi_dma_id);

            ret = rtos_get_semaphore(&lcd_qspi_semaphore, 3000);
            if (ret != kNoErr) {
                bk_printf("ret = %d, lcd qspi get semaphore failed!\r\n", ret);
                return BK_FAIL;
            }
            bk_delay_us(5);
            bk_lcd_qspi_quad_write_stops(port);

        // } else {
        //     bk_printf("invalid lcd qspi refresh method\r\n");
        //     return BK_FAIL;
        // }
    }
    return OPRT_OK;
 }
 
OPERATE_RET tkl_qspi_send_cmd(TUYA_QSPI_NUM_E port, uint8_t cmd)
{
    bk_qspi_write_cmd(port, cmd);

    return OPRT_OK;
}

OPERATE_RET tkl_qspi_send_data_indirect_mode(TUYA_QSPI_NUM_E port, uint8_t *data, uint32_t data_len)
{
    bk_qspi_write_data_indirect_mode(port, data, data_len);

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
 
     ret = bk_qspi_read(port, data, size);
     if (ret != BK_OK)
         return OPRT_COM_ERROR;
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
    UINT64_T data;
    CHAR_T ponit[8];
}data_union_s;

uint32_t swap_endian_24(uint32_t value) {
    return ((value & 0xFF0000) >> 16) | 
           (value & 0x00FF00)         |
           ((value & 0x0000FF) << 16);
}

uint16_t swap_endian_16(uint16_t value) {
    return ((value & 0xFF00) >> 8) | ((value & 0x00FF) << 8);
}

// 32位大小端转换
uint32_t swap_endian_32(uint32_t value) {
    return ((value & 0xFF000000) >> 24) | 
           ((value & 0x00FF0000) >> 8)  |
           ((value & 0x0000FF00) << 8)  |
           ((value & 0x000000FF) << 24);
}

 OPERATE_RET tkl_qspi_comand(TUYA_QSPI_NUM_E port, TUYA_QSPI_CMD_T *command)
 {
     bk_err_t ret = BK_OK;
     data_union_s union_data;
     qspi_cmd_t cmd = {0};
     if ((command == NULL) || (port > TUYA_QSPI_NUM_MAX)) {
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
     // ret = tuya_qspi_hal_command(port, &cmd);
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
        bk_printf("cmd size not right\r\n");
        return -1;
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

        if (command->data_size == 0) {                                             //1.cmd, 3.addr
            if ((command->cmd_size == 1) && (command->addr_size == 3)) {  //all len = 4, DE, 00 60 00
                qspi_hal_set_cmd_c_h(&s_tkl_qspi[port].hal, (swap_endian_24(uaddr) << 8 | ucmd)); // & 0xFF00FF
                qspi_hal_set_cmd_c_cfg1(&s_tkl_qspi[port].hal,  (0x3 << ((command->data_size + 4) * 2)));  // 4字节
            }else if ((command->cmd_size <= 4) && (command->addr_size == 0)){
                qspi_hal_set_cmd_c_h(&s_tkl_qspi[port].hal, ucmd);
                qspi_hal_set_cmd_c_cfg1(&s_tkl_qspi[port].hal,  0x3 << (command->cmd_size * 2));
            }else if ((command->cmd_size == 1) && ((command->addr_size == 1) || (command->addr_size == 2))) {
                qspi_hal_set_cmd_c_h(&s_tkl_qspi[port].hal, (((command->addr_size == 2) ? swap_endian_16(uaddr) : uaddr) << 8) | ucmd);
                qspi_hal_set_cmd_c_cfg1(&s_tkl_qspi[port].hal,  0x3 << ((command->cmd_size + command->addr_size) * 2));
            }else {
                bk_printf("data 0 not adapt\r\n");
            }
        }else if (command->data_size > 0 && command->data_size <= 4) {
            uint32_t value = 0;
            for (uint8_t i = 0; i < command->data_size; i++) {
                value = value | (command->data[i] << (i * 8));
            }
            qspi_hal_set_cmd_c_l(&s_tkl_qspi[port].hal, value);
            qspi_hal_set_cmd_c_h(&s_tkl_qspi[port].hal, (uaddr << 8 | ucmd));  // & 0xFF00FF
            qspi_hal_set_cmd_c_cfg1(&s_tkl_qspi[port].hal, 0x3 << ((command->data_size + 4) * 2));
            if (command->data_lines == TUYA_QSPI_1WIRE) {
                qspi_ll_set_cmd_c_data_line(&s_tkl_qspi[port].hal, QSPI_1WIRE);
            }else if (command->data_lines == TUYA_QSPI_2WIRE) {
                qspi_ll_set_cmd_c_data_line(&s_tkl_qspi[port].hal, QSPI_2WIRE);
            }else {
                qspi_ll_set_cmd_c_data_line(&s_tkl_qspi[port].hal, QSPI_4WIRE);
            }

        }else if (command->data_size > 4 && command->data_size <= 0xFF) {
            qspi_hal_set_cmd_c_h(&s_tkl_qspi[port].hal, (uaddr << 8 | ucmd));   //  & 0xFF00FF
            qspi_hal_set_cmd_c_cfg1(&s_tkl_qspi[port].hal, 0x300);
            qspi_hal_set_cmd_c_cfg2(&s_tkl_qspi[port].hal, command->data_size << 2);
            bk_qspi_write(port, command->data, command->data_size);
            if (command->data_lines == TUYA_QSPI_1WIRE) {
                qspi_ll_set_cmd_c_data_line(&s_tkl_qspi[port].hal, QSPI_1WIRE);
            }else if (command->data_lines == TUYA_QSPI_2WIRE) {
                qspi_ll_set_cmd_c_data_line(&s_tkl_qspi[port].hal, QSPI_2WIRE);
            }else {
                qspi_ll_set_cmd_c_data_line(&s_tkl_qspi[port].hal, QSPI_4WIRE);
            }
            qspi_hal_cmd_c_start(&s_tkl_qspi[port].hal);
            qspi_hal_wait_cmd_done(&s_tkl_qspi[port].hal);
            qspi_hal_set_cmd_c_cfg1(&s_tkl_qspi[port].hal, 0);
            qspi_hal_set_cmd_c_cfg2(&s_tkl_qspi[port].hal, 0);
            return BK_OK;
        }
        // down
        // hw->cmd_c_cfg2.data_line = cmd->wire_mode;
        if (command->dummy_cycle) {
            qspi_hal_set_cmd_c_dummy_clock(&s_tkl_qspi[port].hal, command->dummy_cycle);
            qspi_hal_set_cmd_c_dummy_mode(&s_tkl_qspi[port].hal, 4);
        } else {
            qspi_hal_set_cmd_c_dummy_mode(&s_tkl_qspi[port].hal, 0);
        }
        //up
        qspi_hal_cmd_c_start(&s_tkl_qspi[port].hal);
        qspi_hal_wait_cmd_done(&s_tkl_qspi[port].hal);
    }else {  // read
        if ((command->addr_size == 0) && (command->cmd_size == 1)) {
            qspi_hal_set_cmd_d_l(&s_tkl_qspi[port].hal, 0);
            qspi_hal_set_cmd_d_h(&s_tkl_qspi[port].hal, ucmd);
            qspi_hal_set_cmd_d_cfg1(&s_tkl_qspi[port].hal, 0xc);
            if (command->dummy_cycle) {
                qspi_hal_set_cmd_d_dummy_mode(&s_tkl_qspi[port].hal, 4);
            } else {
                qspi_hal_set_cmd_d_dummy_mode(&s_tkl_qspi[port].hal, 0);
            }
        }else if ((command->addr_size == 1) && (command->cmd_size == 1)) {
            qspi_hal_set_cmd_d_h(&s_tkl_qspi[port].hal, ucmd | (uaddr << 8));
            qspi_hal_set_cmd_d_cfg1(&s_tkl_qspi[port].hal,  0x30);
            if (command->dummy_cycle) {
                qspi_hal_set_cmd_d_dummy_mode(&s_tkl_qspi[port].hal, 4);
            } else {
                qspi_hal_set_cmd_d_dummy_mode(&s_tkl_qspi[port].hal, 0);
            }
        }else if ((command->addr_size == 2) && (command->cmd_size == 1)) {
            qspi_hal_set_cmd_d_h(&s_tkl_qspi[port].hal, (swap_endian_16(uaddr) << 8 | ucmd));
            if(TUYA_QSPI_4WIRE == command->addr_lines) {
                qspi_hal_set_cmd_d_cfg1(&s_tkl_qspi[port].hal, 0xe8);
            } else {
                qspi_hal_set_cmd_d_cfg1(&s_tkl_qspi[port].hal, 0xc0);
            }
            if (command->dummy_cycle) {
                qspi_hal_set_cmd_d_dummy_mode(&s_tkl_qspi[port].hal, 3);
            } else {
                qspi_hal_set_cmd_d_dummy_mode(&s_tkl_qspi[port].hal, 0);
            }
        }else if ((command->addr_size == 3) && (command->cmd_size == 1)) {
            qspi_hal_set_cmd_d_h(&s_tkl_qspi[port].hal, (swap_endian_24(uaddr) << 8 | ucmd));
            // qspi_hal_set_cmd_d_cfg1(&s_tkl_qspi[port].hal,  (0x3 << ((command->data_size + 4) * 2)));  // 4字节
            if(TUYA_QSPI_4WIRE == command->addr_lines) {
                qspi_hal_set_cmd_d_cfg1(&s_tkl_qspi[port].hal, 0x300);
            } else {
                qspi_hal_set_cmd_d_cfg1(&s_tkl_qspi[port].hal, 0x300);
            }

            if (command->dummy_cycle) {
                qspi_hal_set_cmd_d_dummy_mode(&s_tkl_qspi[port].hal, 4);
            } else {
                qspi_hal_set_cmd_d_dummy_mode(&s_tkl_qspi[port].hal, 0);
            }
        }else {
            bk_printf("not support read:%x ,%x\r\n", command->addr_size, command->cmd_size);
            return OPRT_COM_ERROR;
        }
        // data len
        qspi_ll_set_cmd_d_data_length(&s_tkl_qspi[port].hal, command->data_size);
        // data line
        if (command->data_lines == TUYA_QSPI_1WIRE) {
            qspi_ll_set_cmd_d_data_line(&s_tkl_qspi[port].hal, QSPI_1WIRE);
        }else if (command->data_lines == TUYA_QSPI_2WIRE) {
            qspi_ll_set_cmd_d_data_line(&s_tkl_qspi[port].hal, QSPI_2WIRE);
        }else {
            qspi_ll_set_cmd_d_data_line(&s_tkl_qspi[port].hal, QSPI_4WIRE);
        }
        // dummy cycle
        qspi_hal_set_cmd_d_dummy_clock(&s_tkl_qspi[port].hal, command->dummy_cycle);

        qspi_hal_cmd_d_start(&s_tkl_qspi[port].hal);
        qspi_hal_wait_cmd_done(&s_tkl_qspi[port].hal);
        ret = bk_qspi_read(port, command->data, command->data_size);
        if (ret != BK_OK)
            return OPRT_COM_ERROR;
    }
    return OPRT_OK;
 }
 
 OPERATE_RET tkl_qspi_abort_transfer(TUYA_QSPI_NUM_E port)
 {
     if (port > TUYA_QSPI_NUM_MAX) {
         return OPRT_INVALID_PARM;
     }
 
     if (bk_qspi_deinit((spi_id_t)port) != BK_OK)
         return OPRT_COM_ERROR;
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
 
     qspi_irq[port].cb = cb;
     qspi_irq[port].irq_enable = 0;
 
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
     bk_qspi_register_tx_isr(qspi_tx_callback_dispatch, NULL);
     bk_qspi_register_rx_isr(qspi_rx_callback_dispatch, NULL);
 
     qspi_irq[port].irq_enable = 1;
 
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
     bk_qspi_register_tx_isr(NULL, NULL);
     bk_qspi_register_rx_isr(NULL, NULL);
 
     qspi_irq[port].irq_enable = 0;
 
     return OPRT_OK;
 }


OPERATE_RET tkl_qspi_force_lcd_cs_pin(TUYA_QSPI_NUM_E port, BOOL_T enable)
{
    if (enable)
        qspi_hal_force_spi_cs_low_enable(&s_tkl_qspi[port].hal);
    else 
        qspi_hal_force_spi_cs_low_disable(&s_tkl_qspi[port].hal);
    return OPRT_OK;
}



