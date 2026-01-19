#include "sdkconfig.h"
#include "cpu_id.h"
#include "mbox0_drv.h"
#include "sys_driver.h"

#if CONFIG_SOC_SMP
// cpu0 + smp(cpu1,cpu2)
#define MBOX0_CHN0_FIFO_LEN     2
#define MBOX0_CHN1_FIFO_LEN     3
#define MBOX0_CHN2_FIFO_LEN     3
#else //#if CONFIG_SOC_SMP
#define MBOX0_CHN0_FIFO_LEN     2
#define MBOX0_CHN1_FIFO_LEN     4
#define MBOX0_CHN2_FIFO_LEN     2
#endif //#if CONFIG_SOC_SMP

#define MBOX0_CHN0_FIFO_START     0
#define MBOX0_CHN1_FIFO_START     (MBOX0_CHN0_FIFO_START + MBOX0_CHN0_FIFO_LEN)
#define MBOX0_CHN2_FIFO_START     (MBOX0_CHN1_FIFO_START + MBOX0_CHN1_FIFO_LEN)

//struct mbox0_fifo_cfg_t:
#define MBOX0_FIFO_CFG_TABLE   \
{   \
	{MBOX0_CHN0_FIFO_START, MBOX0_CHN0_FIFO_LEN}, \
	{MBOX0_CHN1_FIFO_START, MBOX0_CHN1_FIFO_LEN}, \
	{MBOX0_CHN2_FIFO_START, MBOX0_CHN2_FIFO_LEN}, \
}


static mbox0_dev_t       mbox0_dev;
static u8                mbox0_init = 0;

static const mbox0_fifo_cfg_t  fifo_cfg[] = MBOX0_FIFO_CFG_TABLE;
	
static int mbox0_drv_recieve_message(mbox0_message_t* message)
{
	return mbox0_dev.chn_drv[SELF_CHNL]->chn_recv(&mbox0_dev.hal, message);
}

static void mbox0_drv_isr_handler(void)
{
	uint32_t int_status;
	uint32_t fifo_status;
	mbox0_message_t message;

	int_status = mbox0_dev.chn_drv[SELF_CHNL]->chn_get_int_status(&mbox0_dev.hal);

	if(int_status == 0)
		return;

	do
	{
		fifo_status = mbox0_dev.chn_drv[SELF_CHNL]->chn_get_rx_fifo_stat(&mbox0_dev.hal);
		
		if(fifo_status & RX_FIFO_STAT_NOT_EMPTY)
		{
			mbox0_drv_recieve_message(&message);

			if(message.data[1] != 0)  /* message data len is not 0. */
			{
				if(mbox0_dev.rx_callback != NULL)
					mbox0_dev.rx_callback(&message);
			}
			else
			{
				#if CONFIG_SOC_SMP
				extern void crosscore_smp_cmd_handler(uint8_t src_core, uint32_t cmd);
				
				crosscore_smp_cmd_handler(message.src_cpu, message.data[0]);
				#endif
			}
		}
		else
		{
			break;
		}
	} while(1);
	
}

int mbox0_drv_get_send_stat(uint32_t dest_cpu, uint32_t *fifo_status)
{
	if((dest_cpu >= MBOX_CHNL_NUM) || (fifo_status == NULL))
	{
		return MBOX0_HAL_SW_PARAM_ERR;
	}

	*fifo_status = mbox0_dev.chn_drv[dest_cpu]->chn_get_rx_fifo_stat(&mbox0_dev.hal);

	return MBOX0_HAL_OK;
}

int mbox0_drv_send_message(mbox0_message_t* message)
{
	return mbox0_dev.chn_drv[SELF_CHNL]->chn_send(&mbox0_dev.hal, message);
}

int mbox0_drv_callback_register(mbox0_rx_callback_t callback)
{
	mbox0_dev.rx_callback = callback;

	return 0;
}

int mbox0_init_on_current_core(int id)
{
	int int_src = INT_SRC_MAILBOX;

	if(CPU0_CORE_ID == id)
	{
		mbox0_hal_dev_init(&mbox0_dev.hal);  /* can only be initialized by CPU0. */
		mbox0_hal_set_reg_0x2_chn_pro_disable(&mbox0_dev.hal, 1); /* channel unprotect */

		for(int i = 0; i < ARRAY_SIZE(fifo_cfg); i++)
			mbox0_dev.chn_drv[i]->chn_cfg_fifo(&mbox0_dev.hal, fifo_cfg[i].start, fifo_cfg[i].len);
	}

	mbox0_dev.chn_drv[id]->chn_int_enable(&mbox0_dev.hal, 1);

	/* clear/empty RX FIFO. */
	while(1)
	{
		mbox0_message_t message;

		uint32_t fifo_status = mbox0_dev.chn_drv[id]->chn_get_rx_fifo_stat(&mbox0_dev.hal);
		
		if(fifo_status & RX_FIFO_STAT_NOT_EMPTY)
		{
			mbox0_drv_recieve_message(&message);
		}
		else
		{
			break;
		}
	}

#if CONFIG_SOC_SMP
	/* enable system interrupt, every channel is hard-bound to a processor */
	sys_drv_core_intr_group2_enable(id, (1 << (int_src - 32)));
#else
	sys_drv_int_group2_enable(1 << (int_src - 32));
#endif

	return 0;
}

int mbox0_drv_init(void)
{
	if(mbox0_init == 1)
		return 0;

	mbox0_dev.chn_drv[0] = &hal_chn0_drv;
	mbox0_dev.chn_drv[1] = &hal_chn1_drv;
	mbox0_dev.chn_drv[2] = &hal_chn2_drv;

	mbox0_dev.rx_callback = NULL;

	mbox0_hal_init(&mbox0_dev.hal);

	int int_src = INT_SRC_MAILBOX;

	bk_int_isr_register(int_src, mbox0_drv_isr_handler, NULL);

#if CONFIG_SOC_SMP
	mbox0_init_on_current_core(CPU1_CORE_ID);
	mbox0_init_on_current_core(CPU2_CORE_ID);
#else
	mbox0_init_on_current_core(SELF_CHNL);
#endif

	mbox0_init = 1;

	return 0;
}

int mbox0_drv_deinit(void)
{
	if(mbox0_init == 0)
		return 0;

	uint8_t int_src = INT_SRC_MAILBOX;

	// sys_drv_int_group2_disable(1 << (int_src - 32));

#if CONFIG_SOC_SMP
	/* disable system interrupt, every channel is hard-bound to a processor */
	sys_drv_core_intr_group2_disable(CPU0_CORE_ID, (1 << (int_src - 32)));
	
	/* disable system interrupt, every channel is hard-bound to a processor */
	sys_drv_core_intr_group2_disable(CPU1_CORE_ID, (1 << (int_src - 32)));

	mbox0_dev.chn_drv[CPU1_CORE_ID]->chn_int_enable(&mbox0_dev.hal, 0);
	mbox0_dev.chn_drv[CPU2_CORE_ID]->chn_int_enable(&mbox0_dev.hal, 0);
#else
	sys_drv_int_group2_disable(1 << (int_src - 32));
	mbox0_dev.chn_drv[SELF_CHNL]->chn_int_enable(&mbox0_dev.hal, 0);
#endif

	bk_int_isr_unregister(int_src);

#if CONFIG_SOC_SMP
	mbox0_hal_dev_deinit(&mbox0_dev.hal);  /* can only be de-initialized by CPU0. */
#else
	if(SELF_CHNL == 0)
		mbox0_hal_dev_deinit(&mbox0_dev.hal);  /* can only be de-initialized by CPU0. */
#endif

	mbox0_hal_deinit(&mbox0_dev.hal);
	
	mbox0_dev.rx_callback = NULL;

	mbox0_init = 0;

	return 0;
}

