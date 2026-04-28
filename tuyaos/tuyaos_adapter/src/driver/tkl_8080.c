#include "tkl_8080.h"
#include <os/os.h>
#include <os/mem.h>
#include <os/str.h>
#include <driver/lcd.h>
#include <driver/flash.h>
#include "lcd_disp_hal.h"
#include "gpio_map.h"

#define IO_FUNCTION_ENABLE_8080(pin, func)   \
	do {                                \
		gpio_dev_unmap(pin);            \
		gpio_dev_map(pin, func);        \
		bk_gpio_enable_output(pin);     \
		bk_gpio_set_capacity(pin,GPIO_DRIVER_CAPACITY_3);    \
	} while (0)

#define clk_m(a) (a * 1000 * 1000)

TUYA_MCU8080_ISR_CB ty_8080_cb = NULL;
static lcd_rgb_t *g_mcu = NULL;
static void __lcd_8080_isr_cb(void)
{
    flash_op_status_t flash_status = FLASH_OP_IDLE;
	flash_status = bk_flash_get_operate_status();

    if((flash_status == FLASH_OP_IDLE) && ty_8080_cb) {
        ty_8080_cb(TUYA_MCU8080_OUTPUT_FINISH);
    }
}

static OPERATE_RET __rgb_ty_clk_to_bk_clk(uint32_t clk, lcd_clk_t *outclk)
{
    lcd_clk_t bk_clk = LCD_30M;
    switch(clk) {
        case clk_m(80):
            bk_clk = LCD_80M;
            break;
        case clk_m(64):
            bk_clk = LCD_64M;
            break;
        case clk_m(60):
            bk_clk = LCD_60M;
            break;
        case clk_m(54):
            bk_clk = LCD_54M;
            break;
        case (457*100000):
            bk_clk = LCD_45M;
            break;
        case clk_m(40):
            bk_clk = LCD_40M;
            break;
        case (355*100000):
            bk_clk = LCD_35M;
            break;
        case clk_m(32):
            bk_clk = LCD_32M;
            break;
        case clk_m(30):
            bk_clk = LCD_30M;
            break;
        case (266*100000):
            bk_clk = LCD_26M;
            break;
        case (246*100000):
            bk_clk = LCD_24M;
            break;
        case (2285*10000):
            bk_clk = LCD_22M;
            break;
        case clk_m(20):
            bk_clk = LCD_20M;
            break;
        case (171*100000):
            bk_clk = LCD_17M;
            break;
        case clk_m(15):
            bk_clk = LCD_15M;
            break;
        case clk_m(12):
            bk_clk = LCD_12M;
            break;
           case clk_m(10):
            bk_clk = LCD_10M;
            break;
        case (92*100000):
            bk_clk = LCD_9M;
            break;
        case clk_m(8):
            bk_clk = LCD_8M;
            break;
        case (75*100000):
            bk_clk = LCD_7M;
            break;
        default:
            return -1;
    }

    *outclk = bk_clk;
    return 0;
}

bk_err_t tkl_lcd_mcu_gpio_init(uint8_t data_bits)
{
    bk_printf("%s data_bits:%d\r\n", __func__, data_bits);

    switch (data_bits)
    {
    case 18:
	    IO_FUNCTION_ENABLE_8080(LCD_MCU_D16_PIN, LCD_MCU_D16_FUNC);
	    IO_FUNCTION_ENABLE_8080(LCD_MCU_D17_PIN, LCD_MCU_D17_FUNC);
    case 16:
	    IO_FUNCTION_ENABLE_8080(LCD_MCU_D9_PIN , LCD_MCU_D9_FUNC );
	    IO_FUNCTION_ENABLE_8080(LCD_MCU_D10_PIN, LCD_MCU_D10_FUNC);
	    IO_FUNCTION_ENABLE_8080(LCD_MCU_D11_PIN, LCD_MCU_D11_FUNC);
	    IO_FUNCTION_ENABLE_8080(LCD_MCU_D12_PIN, LCD_MCU_D12_FUNC);
	    IO_FUNCTION_ENABLE_8080(LCD_MCU_D13_PIN, LCD_MCU_D13_FUNC);
	    IO_FUNCTION_ENABLE_8080(LCD_MCU_D14_PIN, LCD_MCU_D14_FUNC);
	    IO_FUNCTION_ENABLE_8080(LCD_MCU_D15_PIN, LCD_MCU_D15_FUNC);
    case 9:
	    IO_FUNCTION_ENABLE_8080(LCD_MCU_D8_PIN , LCD_MCU_D8_FUNC );
    case 8:
	    IO_FUNCTION_ENABLE_8080(LCD_MCU_D0_PIN, LCD_MCU_D0_FUNC);
	    IO_FUNCTION_ENABLE_8080(LCD_MCU_D1_PIN, LCD_MCU_D1_FUNC);
	    IO_FUNCTION_ENABLE_8080(LCD_MCU_D2_PIN, LCD_MCU_D2_FUNC);
	    IO_FUNCTION_ENABLE_8080(LCD_MCU_D3_PIN, LCD_MCU_D3_FUNC);
	    IO_FUNCTION_ENABLE_8080(LCD_MCU_D4_PIN, LCD_MCU_D4_FUNC);
	    IO_FUNCTION_ENABLE_8080(LCD_MCU_D5_PIN, LCD_MCU_D5_FUNC);
	    IO_FUNCTION_ENABLE_8080(LCD_MCU_D6_PIN, LCD_MCU_D6_FUNC);
	    IO_FUNCTION_ENABLE_8080(LCD_MCU_D7_PIN, LCD_MCU_D7_FUNC);
        break;
    default:
        break;
    }

	IO_FUNCTION_ENABLE_8080(LCD_MCU_RDX_PIN, LCD_MCU_RDX_FUNC);
	IO_FUNCTION_ENABLE_8080(LCD_MCU_WRX_PIN, LCD_MCU_WRX_FUNC);
	IO_FUNCTION_ENABLE_8080(LCD_MCU_RSX_PIN, LCD_MCU_RSX_FUNC);
	IO_FUNCTION_ENABLE_8080(LCD_MCU_RESET_PIN, LCD_MCU_RESET_FUNC);
	IO_FUNCTION_ENABLE_8080(LCD_MCU_CSX_PIN, LCD_MCU_CSX_FUNC);

    bk_gpio_set_capacity(LCD_MCU_RDX_PIN,GPIO_DRIVER_CAPACITY_3);
    bk_gpio_set_capacity(LCD_MCU_WRX_PIN,GPIO_DRIVER_CAPACITY_3);
    bk_gpio_set_capacity(LCD_MCU_RESET_PIN,GPIO_DRIVER_CAPACITY_3);
    bk_gpio_set_capacity(LCD_MCU_RSX_PIN,GPIO_DRIVER_CAPACITY_3);
    bk_gpio_set_capacity(LCD_MCU_CSX_PIN,GPIO_DRIVER_CAPACITY_3);
	return BK_OK;
}

/**
 * @brief 8080 init
 * 
 * @param[in] cfg: 8080 config
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_8080_init(TUYA_8080_BASE_CFG_T *cfg)
{
    bk_printf("%s...\r\n",__func__);
    lcd_device_t bk_device = {0};

    memset(&bk_device, 0, sizeof(bk_device));

    bk_device.type = LCD_TYPE_MCU8080;
    bk_device.ppi = (cfg->width << 16) | cfg->height;
    if(cfg->pixel_fmt == TUYA_PIXEL_FMT_RGB565) 
        bk_device.out_fmt = PIXEL_FMT_RGB565;
    if(cfg->pixel_fmt == TUYA_PIXEL_FMT_RGB666) 
        bk_device.out_fmt = PIXEL_FMT_RGB666;
    if(cfg->pixel_fmt == TUYA_PIXEL_FMT_RGB888) 
        bk_device.out_fmt = PIXEL_FMT_RGB888;

    bk_device.init = NULL;
    bk_device.lcd_off = NULL;

    g_mcu = tkl_system_psram_malloc(sizeof(lcd_mcu_t));
    memset(g_mcu, 0, sizeof(lcd_mcu_t));
    if(__rgb_ty_clk_to_bk_clk(cfg->clk, &g_mcu->clk) != 0) {
        bk_printf("clk error %d\r\n",cfg->clk );
        return -2;
    }

    bk_device.mcu= g_mcu;
    tkl_lcd_mcu_gpio_init(cfg->data_bits);
    bk_err_t ret = lcd_driver_init(&bk_device);
    lcd_hal_8080_start_transfer(0);

    return ret;
}

/**
 * @brief 8080 deinit
 * 
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_8080_deinit(void)
{
    bk_err_t ret = lcd_driver_deinit();
    
    tkl_system_psram_free(g_mcu);
    g_mcu = NULL;
    return ret;
}


/**
 * @brief register rgb cb
 * 
 * @param[in] cb: callback
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_8080_irq_cb_register(TUYA_MCU8080_ISR_CB cb)
{
    ty_8080_cb = cb;
    bk_err_t ret = bk_lcd_isr_register(I8080_OUTPUT_EOF, __lcd_8080_isr_cb);
    
    return ret;
}

/**
 * @brief ppi set
 * 
 * @param[in] width: ppi : width
 * @param[in] height: ppi : height
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_8080_ppi_set(uint16_t width, uint16_t height)
{
    lcd_driver_ppi_set(width, height);
    bk_printf("%s : width %d, height  %d\r\n",__func__, width, height);
    return 0;
}

/**
 * @brief pixel mode set
 * 
 * @param[in] mode: mode, such as 565 or 888
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_8080_pixel_mode_set(TUYA_DISPLAY_PIXEL_FMT_E mode)
{
    pixel_format_t bk_mode = 0;
    if(mode == TUYA_PIXEL_FMT_RGB565)
        bk_mode = PIXEL_FMT_RGB565;
    else if(mode == TUYA_PIXEL_FMT_RGB666)
        bk_mode = PIXEL_FMT_RGB666;
    else if(mode == TUYA_PIXEL_FMT_RGB888)
        bk_mode = PIXEL_FMT_RGB888;
    else
        return -2;

    bk_printf("%s : mode %d, bk_mode  %d\r\n",__func__, mode, bk_mode);
    bk_err_t ret = bk_lcd_set_yuv_mode(bk_mode);//mode做一层转换

    return ret;
}

/**
 * @brief 8080 base addr set
 * 
 * @param[in] addr : base addr
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_8080_base_addr_set(uint32_t addr)
{
    bk_err_t ret = lcd_driver_set_display_base_addr(addr);

    return ret;
}

/**
 * @brief 8080 transfer start
 * 
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_8080_transfer_start(void)
{
    lcd_hal_8080_cmd_param_count(1);
    lcd_hal_8080_start_transfer(1);

    return 0;
}

/**
 * @brief 8080 transfer stop
 * 
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_8080_transfer_stop(void)
{
    lcd_hal_8080_start_transfer(0);

    return 0;
}

/**
 * @brief 8080 cmd send
 * 
 *@param[in] cmd : cmd
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_8080_cmd_send(uint32_t cmd)
{
    lcd_hal_8080_cmd_param_count(1);
    lcd_hal_8080_write_cmd(cmd);

    return 0;
}

/**
 * @brief 8080 cmd send(with param)
 * 
 *@param[in] cmd : cmd
 *@param[in] param : param data buf
 *@param[in] param_cnt : param cnt
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_8080_cmd_send_with_param(uint32_t cmd, uint32_t *param, uint8_t param_cnt)
{
    lcd_hal_8080_cmd_send(param_cnt, cmd, param);

    return 0;
}