/*
 * test_timer.c
 * Copyright (C) 2024 cc <cc@tuya>
 *
 * Distributed under terms of the MIT license.
 */

#include "tkl_timer.h"
#include "tuya_cloud_types.h"
#include "tuya_error_code.h"

#define DELAY_TIME    (2 * 1000 * 1000)    //us
static uint32_t timer_id = 1;

STATIC CHAR_T sg_count = 0;

STATIC VOID_T __timer_callback(VOID *args)
{
    uint32_t t = *(uint32_t *)args;

    sg_count++;
    bk_printf("\r\n------------- Timer%d Callback %d --------------\r\n", t, sg_count);

    if(sg_count >= 5) {
        sg_count=0;
        tkl_timer_stop(timer_id);
        tkl_timer_deinit(timer_id);
        bk_printf("\r\ntimer %d is stop\r\n", timer_id);
    }

    return;
}

void cli_tkl_timer_test(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    bk_printf("argc: %d\r\n cmd: ", argc);
    for (int i = 0; i < argc; i++) {
        bk_printf("%s ", argv[i]);
    }
    bk_printf("\r\n");

    if (sg_count != 0) {
        bk_printf("wait...\r\n");
        return;
    }

    timer_id = os_strtoul(argv[1], NULL, 10);

    OPERATE_RET ret = OPRT_OK;
    TUYA_TIMER_BASE_CFG_T sg_timer_cfg = {
        .mode = TUYA_TIMER_MODE_PERIOD,
        .args = &timer_id,
        .cb = __timer_callback
    };

    bk_printf("init timer %d\r\n", timer_id);
    ret = tkl_timer_init(timer_id, &sg_timer_cfg);
    if (OPRT_OK != ret) {
        bk_printf("init timer %d error\r\n", timer_id);
        goto __EXIT;
    }

    /*start timer*/
    bk_printf("start timer %d\r\n", timer_id);
    ret = tkl_timer_start(timer_id, DELAY_TIME);
    if (OPRT_OK != ret) {
        bk_printf("start timer %d error\r\n", timer_id);
        goto __EXIT;
    }

    bk_printf("11111111111111111111\r\n");
    return;

__EXIT:
    bk_printf("22222222222222222222\r\n");
    return;
}

