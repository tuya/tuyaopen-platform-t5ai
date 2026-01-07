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
#include "posix/time.h"

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

#if (CONFIG_CPU_INDEX != 0)
void tuya_get_usb_dev(uint32_t *vid, uint32_t *pid)
{
    int cnt = 10, status = 0;
    if (vid == NULL || pid == NULL)
        return;

#if 0   // TODO
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
#endif
}
#endif  // CONFIG_CPU_INDEX != 0

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


