#ifndef __MBOX0_DRV_H__
#define __MBOX0_DRV_H__

#include <common/bk_err.h>
#include <driver/int.h>
#include "mbox0_hal.h"

#define MBOX_CHNL_NUM		3

typedef void (*mbox0_rx_callback_t)(mbox0_message_t *);

typedef struct
{
	mbox0_hal_t            hal;
	const hal_chn_drv_t *  chn_drv[MBOX_CHNL_NUM];
	mbox0_rx_callback_t    rx_callback;
} mbox0_dev_t;

typedef struct
{
	uint32_t     start;
	uint32_t     len;
} mbox0_fifo_cfg_t;


#if CONFIG_SOC_SMP
#define SELF_CHNL     rtos_get_core_id()
#else
#define SELF_CHNL     1
#endif

int mbox0_drv_callback_register(mbox0_rx_callback_t callback);
int mbox0_drv_send_message(mbox0_message_t* message);
int mbox0_drv_get_send_stat(uint32_t dest_cpu, uint32_t *fifo_status);
int mbox0_drv_init(void);
int mbox0_drv_deinit(void);

#endif
