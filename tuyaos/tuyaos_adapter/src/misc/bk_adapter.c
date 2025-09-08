/****************************************************************************
 * @file bk_adapter.c
 * @brief this module is used to bk_adapter
 * @version 0.0.1
 * @date 2023-06-28
 *
 * @copyright Copyright(C) 2021-2022 Tuya Inc. All Rights Reserved.
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/
#include "tuya_cloud_types.h"
#include <stdint.h>
#include "sdkconfig.h"
#include "gpio_map.h"
#include "tkl_gpio.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Type Declarations
 ****************************************************************************/

/****************************************************************************
 * Private Data Declarations
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/**
 * Security Check Interface : To check the contents of hostids
 **/
#if CONFIG_SYS_CPU1
uint32_t mem_sanity_check(void *mem)
{
    return 1;
}
#endif
#if 0
void tuya_get_usb_dev(uint32_t *vid, uint32_t *pid)
{
    int cnt = 10, status = 0;
    if (vid == NULL || pid == NULL)
        return;

    TUYA_GPIO_BASE_CFG_T cfg;
    cfg.mode = TUYA_GPIO_PULLUP;
    cfg.direct = TUYA_GPIO_OUTPUT;
    cfg.level = TUYA_GPIO_LEVEL_HIGH;
    tkl_gpio_init(TUYA_GPIO_NUM_28, &cfg);

    tkl_gpio_write(TUYA_GPIO_NUM_28, TUYA_GPIO_LEVEL_HIGH);

    do {
        status = media_app_get_usb_connect_status();
        if (status) {
            bk_printf("found\r\n\r\n");
            *vid = 0x1111;
            *pid = 0x2222;
            break;
        }
        bk_printf("not found, next\r\n\r\n");
        tkl_system_sleep(50);
    } while (cnt++ < 10);

    tkl_gpio_write(TUYA_GPIO_NUM_28, TUYA_GPIO_LEVEL_LOW);
}

#if CONFIG_SYS_CPU0 && CONFIG_SOC_BK7258
#include "tuya_cloud_types.h"
#include "tkl_gpio.h"
#include "tkl_display.h"

enum {
    MUTIL_INIT = 0,
    MUTIL_ON,
    MUTIL_OFF,
};

static inline int __attribute__((always_inline)) gpio_level_check_and_set(uint32_t io, uint32_t active, int flag)
{
    int expect = 0;
    TUYA_GPIO_LEVEL_E level = TUYA_GPIO_LEVEL_LOW;

    tkl_gpio_read(io, &level);

    switch (flag) {
        case MUTIL_INIT:
            expect = (active == TUYA_GPIO_LEVEL_HIGH)? TUYA_GPIO_LEVEL_LOW: TUYA_GPIO_LEVEL_HIGH;
            break;

        case MUTIL_ON:
            expect = (active == TUYA_GPIO_LEVEL_HIGH)? TUYA_GPIO_LEVEL_HIGH: TUYA_GPIO_LEVEL_LOW;
            if (expect != level) {
                tkl_gpio_write(io, expect);
            }
            break;

        case MUTIL_OFF:
            expect = (active == TUYA_GPIO_LEVEL_HIGH)? TUYA_GPIO_LEVEL_LOW: TUYA_GPIO_LEVEL_HIGH;
            if (expect != level) {
                tkl_gpio_write(io, expect);
            }
            break;

        default:
            break;
    }

    return expect;
}

static uint32_t is_init = 0;
static void __mutil_power_init(void)
{
#if CONFIG_TUYA_LOGIC_MODIFY
    TUYA_GPIO_BASE_CFG_T cfg;
    cfg.direct = TUYA_GPIO_OUTPUT;
    cfg.level = TUYA_GPIO_LEVEL_LOW;

    uint8_t usb_ldo, lcd_ldo, lcd_bl, active_level;

    tkl_vi_get_power_info(UVC_CAMERA, &usb_ldo, &active_level);
    cfg.level = gpio_level_check_and_set(usb_ldo, active_level, MUTIL_INIT);
    tkl_gpio_init(usb_ldo, &cfg);

//     tkl_display_power_ctrl_pin(&lcd_ldo, &active_level);
//     cfg.level = gpio_level_check_and_set(lcd_ldo, active_level, MUTIL_INIT);
//     tkl_gpio_init(lcd_ldo, &cfg);

//    if (tkl_display_bl_mode() == TKL_DISP_BL_GPIO) {
//        tkl_display_bl_ctrl_io(&lcd_bl, &active_level);
//        cfg.level = gpio_level_check_and_set(lcd_bl, active_level, MUTIL_INIT);
//        tkl_gpio_init(lcd_bl, &cfg);
//    }
#ifdef MUTEX_CTRL
    tkl_gpio_init(MUTEX_CTRL, &cfg);
#endif // MUTEX_CTRL
#endif // CONFIG_TUYA_LOGIC_MODIFY
}

void tuya_multimedia_power_on(void)
{
#if CONFIG_TUYA_LOGIC_MODIFY
//    if (!is_init) {
        __mutil_power_init();
        is_init = 1;
//    }
    uint8_t usb_ldo, lcd_ldo, lcd_bl, active_level;
    // 3.3V / USB Enable
    TUYA_GPIO_LEVEL_E level = TUYA_GPIO_LEVEL_LOW;
    tkl_display_power_ctrl_pin(&lcd_ldo, &active_level);
    gpio_level_check_and_set(lcd_ldo, active_level, MUTIL_ON);

    tkl_vi_get_power_info(UVC_CAMERA, &usb_ldo, &active_level);
    gpio_level_check_and_set(usb_ldo, active_level, MUTIL_ON);

#endif // CONFIG_TUYA_LOGIC_MODIFY
}

void tuya_multimedia_power_off(void)
{
#if CONFIG_TUYA_LOGIC_MODIFY
//    if (!is_init) {
        __mutil_power_init();
        is_init = 1;
//    }

    uint8_t usb_ldo, lcd_ldo, lcd_bl, active_level;

//     if (tkl_display_bl_mode() == TKL_DISP_BL_GPIO) {
//         tkl_display_bl_ctrl_io(&lcd_bl, &active_level);
//         gpio_level_check_and_set(lcd_bl, active_level, MUTIL_OFF);
//     }

    // 3.3V / USB Enable
    TUYA_GPIO_LEVEL_E level = TUYA_GPIO_LEVEL_LOW;
    tkl_display_power_ctrl_pin(&lcd_ldo, &active_level);
    gpio_level_check_and_set(lcd_ldo, active_level, MUTIL_OFF);

    tkl_vi_get_power_info(UVC_CAMERA, &usb_ldo, &active_level);
    gpio_level_check_and_set(usb_ldo, active_level, MUTIL_OFF);

#endif // CONFIG_TUYA_LOGIC_MODIFY
}

#endif // CONFIG_SYS_CPU0 && CONFIG_SOC_BK7258

uint8_t* dhcp_lookup_mac(uint8_t *chaddr)
{
    return NULL;
}
#endif // CONFIG_SYS_CPU0 && CONFIG_SOC_BK7258

VOID tkl_data_dump(CONST int     level,
        CONST CHAR_T              *file,
        CONST INT_T               line,
        CONST CHAR_T              *title,
        UINT8_T                   width,
        UINT8_T                   *buf,
        UINT16_T                  size)
{
    int i = 0;

    if (width < 64) {
        width = 64;
    }
    bk_printf("cpu 0: %s %d <%p>", title, size, buf);
    shell_log_raw_data(buf, size);
    bk_printf("\r\n\r\n");
}

void _fini(void) {
    // 空实现
}

#include <stddef.h>
#include <string.h>
#include <time.h>

#include "FreeRTOS.h"
#include "task.h"

#define MICROSECONDS_PER_SECOND    ( 1000000LL )                                   /**< Microseconds per second. */
#define NANOSECONDS_PER_SECOND     ( 1000000000LL )                                /**< Nanoseconds per second. */
#define NANOSECONDS_PER_TICK       ( NANOSECONDS_PER_SECOND / configTICK_RATE_HZ ) /**< Nanoseconds per FreeRTOS tick. */

static void UTILS_NanosecondsToTimespec( int64_t llSource,
                                  struct timespec * const pxDestination )
{
    long lCarrySec = 0;

    /* Convert to timespec. */
    pxDestination->tv_sec = ( time_t ) ( llSource / NANOSECONDS_PER_SECOND );
    pxDestination->tv_nsec = ( long ) ( llSource % NANOSECONDS_PER_SECOND );

    /* Subtract from tv_sec if tv_nsec < 0. */
    if( pxDestination->tv_nsec < 0L )
    {
        /* Compute the number of seconds to carry. */
        lCarrySec = ( pxDestination->tv_nsec / ( long ) NANOSECONDS_PER_SECOND ) + 1L;

        pxDestination->tv_sec -= ( time_t ) ( lCarrySec );
        pxDestination->tv_nsec += lCarrySec * ( long ) NANOSECONDS_PER_SECOND;
    }
}

int clock_gettime( clockid_t clock_id,
                   struct timespec * tp )
{
    TimeOut_t xCurrentTime = { 0 };

    /* Intermediate variable used to convert TimeOut_t to struct timespec.
     * Also used to detect overflow issues. It must be unsigned because the
     * behavior of signed integer overflow is undefined. */
    uint64_t ullTickCount = 0ULL;

    /* Silence warnings about unused parameters. */
    ( void ) clock_id;

    /* Get the current tick count and overflow count. vTaskSetTimeOutState()
     * is used to get these values because they are both static in tasks.c. */
    vTaskSetTimeOutState( &xCurrentTime );

    /* Adjust the tick count for the number of times a TickType_t has overflowed.
     * portMAX_DELAY should be the maximum value of a TickType_t. */
    ullTickCount = ( uint64_t ) ( xCurrentTime.xOverflowCount ) << ( sizeof( TickType_t ) * 8 );

    /* Add the current tick count. */
    ullTickCount += xCurrentTime.xTimeOnEntering;

    /* Convert ullTickCount to timespec. */
    UTILS_NanosecondsToTimespec( ( int64_t ) ullTickCount * NANOSECONDS_PER_TICK, tp );

    return 0;
}


