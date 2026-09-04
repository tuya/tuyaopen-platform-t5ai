#include "tkl_uart.h"

#include "drv_model_pub.h"
#include "uart_pub.h"
#include <driver/uart.h>
#include "uart_statis.h"
#include "bk_uart.h"
#include <components/log.h>
#include "uart_hal.h"

#define CLI_GETCHAR_TIMEOUT           (120000)

extern void bk_printf(const char *fmt, ...);

static gpio_id_t __tkl_uart2_rx_pin = GPIO_30;
static gpio_id_t __tkl_uart2_tx_pin = GPIO_31;

static inline int __tkl_uart2_pin_check(void)
{
    if (((__tkl_uart2_rx_pin == GPIO_30) && (__tkl_uart2_tx_pin == GPIO_31)) ||
        ((__tkl_uart2_rx_pin == GPIO_40) && (__tkl_uart2_tx_pin == GPIO_41))) {
        return 0;
    } else {
        return -1;
    }
}
gpio_id_t __tkl_uart2_get_rx_pin(void)
{
    if (__tkl_uart2_pin_check() != 0) {
        return GPIO_30;
    }
    return __tkl_uart2_rx_pin;
}
gpio_id_t __tkl_uart2_get_tx_pin(void)
{
    if (__tkl_uart2_pin_check() != 0) {
        return GPIO_31;
    }
    return __tkl_uart2_tx_pin;
}

/**
 * @brief which UART port/role a pin can serve, for tkl_io_pin_to_func()
 *
 * @param[in] pin: gpio number
 * @return (port << 8) | role   with role 0 = TX, 1 = RX, 2 = RTS, 3 = CTS
 *         OPRT_NOT_SUPPORTED if the pin has no UART function
 *
 * Only UART2 has selectable pins on this chip; UART0/1 are fixed (and one of
 * them is the log port) so they are not reported here.
 */
int32_t __tkl_uart_pin_to_func(TUYA_PIN_NAME_E pin)
{
    if ((pin == GPIO_31) || (pin == GPIO_41)) {
        return (2 << 8) | 0;
    }
    if ((pin == GPIO_30) || (pin == GPIO_40)) {
        return (2 << 8) | 1;
    }
    return OPRT_NOT_SUPPORTED;
}

// These used to be void: they validated the pin and then could only print, so a
// rejected pin left the old one in place with the caller told nothing. The
// port parameter was also typed TUYA_I2C_NUM_E, copy-pasted from the I2C
// setters.
OPERATE_RET __tkl_uart2_set_rx_pin(TUYA_UART_NUM_E port, const TUYA_PIN_NAME_E pin)
{
    // only support UART2
    if (pin != GPIO_30 && pin != GPIO_40) {
        bk_printf("tkl_uart2_set_rx_pin unsupport pin: %d\r\n", pin);
        return OPRT_NOT_SUPPORTED;
    }
    __tkl_uart2_rx_pin = (gpio_id_t)pin;
    return OPRT_OK;
}

OPERATE_RET __tkl_uart2_set_tx_pin(TUYA_UART_NUM_E port, const TUYA_PIN_NAME_E pin)
{
    // only support UART2
    if (pin != GPIO_31 && pin != GPIO_41) {
        bk_printf("tkl_uart2_set_tx_pin unsupport pin: %d\r\n", pin);
        return OPRT_NOT_SUPPORTED;
    }
    __tkl_uart2_tx_pin = (gpio_id_t)pin;
    return OPRT_OK;
}

/**
 * @brief uart init
 *
 * @param[in] port_id: uart port id,
 *                     the high 16bit - uart type
 *                                      it's value must be one of the TKL_UART_TYPE_E type
 *                     the low 16bit - uart port number
 *                     you can input like this TKL_UART_PORT_ID(TKL_UART_SYS, 2)
 * @param[in] cfg: uart config
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_uart_init(TUYA_UART_NUM_E port_id, TUYA_UART_BASE_CFG_T *cfg)
{
    int port_num = TUYA_UART_GET_PORT_NUMBER(port_id);
    uart_id_t port;
    uart_config_t bkcfg;

    memset(&bkcfg, 0, sizeof(uart_config_t));

    if(CONFIG_UART_PRINT_PORT == port_num) {
        // bk_printf("tkl_uart_init: print port already init.\n");
        return OPRT_OK;
    } else if (0 == port_num) {
        port = UART_ID_0;
    } else if (1 == port_num) {
        port = UART_ID_1;
    } else if (2 == port_num) {
        port = UART_ID_2;
    } else {
        bk_printf("tkl_uart_init unsupport port: %d\r\n", port_num);
        return OPRT_INVALID_PARM;
    }

    //! data bits
    if (TUYA_UART_DATA_LEN_5BIT == cfg->databits) {
        bkcfg.data_bits = UART_DATA_5_BITS;
    } else if (TUYA_UART_DATA_LEN_6BIT == cfg->databits) {
        bkcfg.data_bits = UART_DATA_6_BITS;
    } else if (TUYA_UART_DATA_LEN_7BIT ==  cfg->databits) {
        bkcfg.data_bits = UART_DATA_7_BITS;
    } else if (TUYA_UART_DATA_LEN_8BIT == cfg->databits) {
        bkcfg.data_bits = UART_DATA_8_BITS;
    } else {
        bk_printf("tkl_uart_init unsupport databits: %d\r\n", cfg->databits);
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }

    //! stop bits
    if (TUYA_UART_STOP_LEN_1BIT == cfg->stopbits) {
        bkcfg.stop_bits = UART_STOP_BITS_1;
    } else if (TUYA_UART_STOP_LEN_2BIT == cfg->stopbits) {
        bkcfg.stop_bits = UART_STOP_BITS_2;
    } else {
        bk_printf("tkl_uart_init unsupport stopbits: %d\r\n", cfg->stopbits);
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }

    //!  parity bits
    if (TUYA_UART_PARITY_TYPE_NONE == cfg->parity) {
        bkcfg.parity = UART_PARITY_NONE;
    } else if (TUYA_UART_PARITY_TYPE_EVEN == cfg->parity) {
        bkcfg.parity = UART_PARITY_EVEN;
    } else if (TUYA_UART_PARITY_TYPE_ODD == cfg->parity) {
        bkcfg.parity = UART_PARITY_ODD;
    } else {
        bk_printf("tkl_uart_init unsupport parity: %d\r\n", cfg->parity);
        return OPRT_OS_ADAPTER_INVALID_PARM;
    }

    bkcfg.baud_rate = cfg->baudrate;
    bkcfg.flow_ctrl = UART_FLOWCTRL_DISABLE;
    bkcfg.src_clk   = UART_SCLK_XTAL_26M;

    //bk_printf("tkl_uart_init, port: %d, baudrate %d\r\n", port_num, cfg->baudrate);

    bk_uart_init(port, &bkcfg);

    return OPRT_OK;
}

/**
 * @brief uart deinit
 *
 * @param[in] port_id: uart port id,
 *                     the high 16bit - uart type
 *                                      it's value must be one of the TKL_UART_TYPE_E type
 *                     the low 16bit - uart port number
 *                     you can input like this TKL_UART_PORT_ID(TKL_UART_SYS, 2)
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_uart_deinit(TUYA_UART_NUM_E port_id)
{
    int port_num = TUYA_UART_GET_PORT_NUMBER(port_id);
    uart_id_t port;

    if( CONFIG_UART_PRINT_PORT == port_num) {
        // bk_printf("tkl_uart_init: print port already inuse.\n");
        return OPRT_INVALID_PARM;
    } else if ( 0 == port_num) {
        port = UART_ID_0;
    } else if ( 1 == port_num) {
        port = UART_ID_1;
    } else if ( 2 == port_num) {
        port = UART_ID_2;
    }  else {
        return OPRT_INVALID_PARM;
    }
    bk_uart_deinit(port);

    tkl_gpio_deinit(uart_hal_get_tx_pin(port));
    tkl_gpio_deinit(uart_hal_get_rx_pin(port));

    TUYA_GPIO_BASE_CFG_T gpio_cfg;
    gpio_cfg.direct = TUYA_GPIO_INPUT;
    gpio_cfg.level = TUYA_GPIO_LEVEL_HIGH;
    gpio_cfg.mode = TUYA_GPIO_PULLDOWN;
    tkl_gpio_init(uart_hal_get_tx_pin(port), &gpio_cfg);
    gpio_cfg.mode = TUYA_GPIO_FLOATING;
    tkl_gpio_init(uart_hal_get_rx_pin(port), &gpio_cfg);

    return OPRT_OK;
}

/**
 * @brief uart write data
 *
 * @param[in] port_id: uart port id,
 *                     the high 16bit - uart type
 *                                      it's value must be one of the TKL_UART_TYPE_E type
 *                     the low 16bit - uart port number
 *                     you can input like this TKL_UART_PORT_ID(TKL_UART_SYS, 2)
 * @param[in] data: write buff
 * @param[in] len:  buff len
 *
 * @return return > 0: number of data written; return <= 0: write errror
 */

int32_t tkl_uart_write(TUYA_UART_NUM_E port_id, void *buff, uint16_t len)
{
    uart_id_t port;

    if (0 == TUYA_UART_GET_PORT_NUMBER(port_id)) {
        port = UART_ID_0;
    } else if ( 1 == TUYA_UART_GET_PORT_NUMBER(port_id)) {
        port = UART_ID_1;
    } else if ( 2 == TUYA_UART_GET_PORT_NUMBER(port_id)) {
        port = UART_ID_2;
    } else {
        return OPRT_INVALID_PARM;
    }

    bk_uart_write_bytes(port, buff, len);
    return len;
}

/**
 * @brief uart read data
 *
 * @param[in] port_id: uart port id,
 *                     the high 16bit - uart type
 *                                      it's value must be one of the TKL_UART_TYPE_E type
 *                     the low 16bit - uart port number
 *                     you can input like this TKL_UART_PORT_ID(TKL_UART_SYS, 2)
 * @param[out] data: read data
 * @param[in] len:  buff len
 *
 * @return return >= 0: number of data read; return < 0: read errror
 */
int32_t tkl_uart_read(TUYA_UART_NUM_E port_id, void *buff, uint16_t len)
{
    uart_id_t port;

    if ( 0 == TUYA_UART_GET_PORT_NUMBER(port_id)) {
        port = UART_ID_0;
    } else if ( 1 == TUYA_UART_GET_PORT_NUMBER(port_id)) {
        port = UART_ID_1;
    } else if ( 2 == TUYA_UART_GET_PORT_NUMBER(port_id)) {
        port = UART_ID_2;
    }  else {
        return OPRT_INVALID_PARM;
    }

    uint32_t length = bk_uart_read_bytes(port, buff, len, CLI_GETCHAR_TIMEOUT);

    return length;
}

/**
 * @brief set uart transmit interrupt status
 *
 * @param[in] port_id: uart port id,
 *                     the high 16bit - uart type
 *                                      it's value must be one of the TKL_UART_TYPE_E type
 *                     the low 16bit - uart port number
 *                     you can input like this TKL_UART_PORT_ID(TKL_UART_SYS, 2)
 * @param[in] enable: TRUE-enalbe tx int, FALSE-disable tx int
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_uart_set_tx_int(TUYA_UART_NUM_E port_id, BOOL_T enable)
{
    return OPRT_OK;
}

/**
 * @brief set uart receive flowcontrol
 *
 * @param[in] port_id: uart port id,
 *                     the high 16bit - uart type
 *                                      it's value must be one of the TKL_UART_TYPE_E type
 *                     the low 16bit - uart port number
 *                     you can input like this TKL_UART_PORT_ID(TKL_UART_SYS, 2)
 * @param[in] enable: TRUE-enalbe rx flowcontrol, FALSE-disable rx flowcontrol
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_uart_set_rx_flowctrl(TUYA_UART_NUM_E port_id, BOOL_T enable)
{
    return OPRT_OK;
}

TUYA_UART_IRQ_CB tkl_rx_cb = NULL;
static void uart_isr_t_cb(uart_id_t id, void  *param) {
    if (NULL != tkl_rx_cb) {
        tkl_rx_cb((uint32_t)id);
    }
}
/**
 * @brief enable uart rx interrupt and regist interrupt callback
 *
 * @param[in] port_id: uart port id,
 *                     the high 16bit - uart type
 *                                      it's value must be one of the TKL_UART_TYPE_E type
 *                     the low 16bit - uart port number
 *                     you can input like this TKL_UART_PORT_ID(TKL_UART_SYS, 2)
 * @param[in] rx_cb: receive callback
 *
 * @return none
 */
void tkl_uart_rx_irq_cb_reg(TUYA_UART_NUM_E port_id, TUYA_UART_IRQ_CB rx_cb)
{
    int port_num = TUYA_UART_GET_PORT_NUMBER(port_id);
    uart_id_t port;

    // bk_printf("tkl_uart_rx_irq_cb_reg: port_num(%d).rx_cb(%p).\n", port_num, rx_cb);

    if( CONFIG_UART_PRINT_PORT == port_num) {
        // bk_printf("tkl_uart_rx_irq_cb_reg: print port already inuse.\n");
        return;
    } else if ( 0 == port_num) {
        port = UART_ID_0;
    } else if ( 1 == port_num) {
        port = UART_ID_1;
    } else if ( 2 == port_num) {
        port = UART_ID_2;
    }  else {
        return;
    }
    tkl_rx_cb = rx_cb;

    bk_uart_disable_sw_fifo(port);
	bk_uart_register_rx_isr(port, uart_isr_t_cb, NULL);
    bk_uart_enable_rx_interrupt(port);
}

/**
 * @brief regist uart tx interrupt callback
 * If this function is called, it indicates that the data is sent asynchronously through interrupt,
 * and then write is invoked to initiate asynchronous transmission.
 *
 * @param[in] port_id: uart port id,
 *                     the high 16bit - uart type
 *                                      it's value must be one of the TKL_UART_TYPE_E type
 *                     the low 16bit - uart port number
 *                     you can input like this TKL_UART_PORT_ID(TKL_UART_SYS, 2)
 * @param[in] rx_cb: receive callback
 *
 * @return none
 */
void tkl_uart_tx_irq_cb_reg(TUYA_UART_NUM_E port_id, TUYA_UART_IRQ_CB tx_cb)
{

}
