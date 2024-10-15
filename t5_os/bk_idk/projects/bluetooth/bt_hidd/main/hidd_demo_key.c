#include "bk_gpio.h"
#include <driver/gpio.h>
#include <driver/hal/hal_gpio_types.h>
#include "gpio_driver.h"
#include <string.h>
#include <stdlib.h>
#include "hidd_demo.h"

#define KEY_1 GPIO_12 //KEY3 in bk7258 dev board, please join p12(p27)


static void hidd_demo_gpio_int_isr(gpio_id_t id)
{
	BK_LOGI("BT-HIDD-KEY", "gpio isr index:%d\n",id);
	bk_gpio_clear_interrupt(id);
    switch(id)
    {
        case KEY_1:
            clib_bt_hidd_send_report('0');
        break;
        default:
        break;
    }
}

void hidd_demo_key_init()
{
    gpio_dev_unmap(KEY_1);
    gpio_int_type_t int_type = GPIO_INT_TYPE_FALLING_EDGE;
    gpio_config_t cfg;
	cfg.io_mode =GPIO_INPUT_ENABLE;
    cfg.pull_mode = GPIO_PULL_UP_EN;
    bk_gpio_set_config(KEY_1, &cfg);
    bk_gpio_set_interrupt_type(KEY_1, int_type);
    bk_gpio_register_isr(KEY_1 ,hidd_demo_gpio_int_isr);
    bk_gpio_enable_interrupt(KEY_1);
}