#include "tuya_dma.h"
#include "tkl_memory.h"
#include "sys_driver.h"

#include "driver/dma.h"
#include "bk_misc.h"
#ifdef CONFIG_FREERTOS_SMP
#include "spinlock.h"
static SPINLOCK_SECTION volatile  spinlock_t dma_spin_lock = SPIN_LOCK_INIT;
#endif // CONFIG_FREERTOS_SMP

#define DMA_TRANS_LEN_MAX (0x10000)

void tkl_dma_isr_common(dma_unit_t dma_unit_id, uint32_t id);
typedef struct {
    dma_hal_t hal;
    uint32_t id_init_bits;
} DMA_DRIVER_T;

typedef struct
{
	uint32_t		chnl_bitmap;
} DMA_CHNL_POOL_T;

static dma_config_t g_dma_config[SOC_DMA_UNIT_NUM][SOC_DMA_CHAN_NUM_PER_UNIT] = {0};
static DMA_DRIVER_T g_dma[SOC_DMA_UNIT_NUM] = {0};
static DMA_ISR_T g_dma_finish_isr[SOC_DMA_UNIT_NUM][SOC_DMA_CHAN_NUM_PER_UNIT] = {NULL};
static uint32_t g_dma_devid[DMA_ID_MAX] = {0};

static bool g_dma_driver_is_init = false;
static DMA_CHNL_POOL_T g_dma_chnl_pool = {0};

#define DMA_RETURN_NOT_INIT() do {\
        if (!g_dma_driver_is_init) {\
            return OPRT_NOT_FOUND;\
        }\
    } while(0)

#define DMA_RETURN_INVALID_ID(channel) do {\
        if (((channel) < CONFIG_DMA_LOGIC_CHAN_ID_MIN) || ((channel) >= CONFIG_DMA_LOGIC_CHAN_ID_MIN + CONFIG_DMA_LOGIC_CHAN_CNT)) {\
            return OPRT_INVALID_PARM;\
        }\
    } while(0)

#define DMA_RETURN_ID_NOT_INIT(dma_num,id) do {\
        if (!(g_dma[dma_num].id_init_bits & BIT((id)))) {\
            return OPRT_NOT_FOUND;\
        }\
    } while(0)

#define DMA_LOG_ID_IS_STARTED(dma_num,channel) do {\
        if (dma_hal_is_id_started(&g_dma[dma_num].hal, (channel))) {\
        }\
    } while(0)

#define DMA_RETURN_INVALID_ADDR(start_addr, end_addr) do {\
        if ((0 < (end_addr)) && ((end_addr) < (start_addr))) {\
            return OPRT_INVALID_PARM;\
        }\
    } while(0)

static inline uint32_t tkl_dma_enter_critical()
{
	uint32_t flags = rtos_disable_int();

#ifdef CONFIG_FREERTOS_SMP
	spin_lock(&dma_spin_lock);
#endif // CONFIG_FREERTOS_SMP

	return flags;
}

static inline void tkl_dma_exit_critical(uint32_t flags)
{
#ifdef CONFIG_FREERTOS_SMP
	spin_unlock(&dma_spin_lock);
#endif // CONFIG_FREERTOS_SMP

	rtos_enable_int(flags);
}

static void dma_id_to_id_ch(dma_id_t id,uint8_t *dma_num,uint8_t *channel)
{
    if(dma_num){
        *dma_num = id / SOC_DMA_CHAN_NUM_PER_UNIT;
    }
    if(channel){
        *channel = id % SOC_DMA_CHAN_NUM_PER_UNIT;
    }
}

static void dma_id_init(dma_id_t id)
{
    uint8_t dma_num;
    uint8_t channel;
    dma_id_to_id_ch(id, &dma_num, &channel);
    g_dma[dma_num].id_init_bits |= BIT(channel);
}

static void dma_id_deinit(dma_id_t id)
{
	uint8_t dma_num;
	uint8_t dma_channel;
	dma_id_to_id_ch(id, &dma_num, &dma_channel);
    g_dma[dma_num].id_init_bits &= ~BIT(dma_channel);
}

static void dma_id_enable_interrupt(dma_id_t id)
{
	#if CONFIG_SPE
	sys_drv_int_enable(GDMA_INTERRUPT_CTRL_BIT);
	sys_drv_int_group2_enable(DMA1_SEC_INTERRUPT_CTRL_BIT);
	#else
	sys_drv_int_enable(DMA0_NSEC_INTERRUPT_CTRL_BIT);
	sys_drv_int_enable(DMA1_NSEC_INTERRUPT_CTRL_BIT);
	#endif
}

static OPERATE_RET tkl_dma_driver_init(void)
{
    if (g_dma_driver_is_init) {
        return OPRT_OK;
    }

	g_dma_chnl_pool.chnl_bitmap = 0;
    // workaround: must uncomment it after mailbox problem fixed
    // bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_BAKP_DMA0, PM_POWER_MODULE_STATE_ON);
    // bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_BAKP_DMA1, PM_POWER_MODULE_STATE_ON);

    /* 1)intc_service_register
     * 2)init dma_finish_int handler, dma_half_finish_int handler
     * 3)disable dma_en (0~6), clear int status
     * 4)init dma_config
     */
    os_memset(&g_dma, 0, sizeof(g_dma));
    os_memset(&g_dma_finish_isr, 0, sizeof(g_dma_finish_isr));
    bk_dma_driver_init();
    g_dma_driver_is_init = true;

    return OPRT_OK;
}

static OPERATE_RET tkl_dma_driver_deinit(void)
{
    if (!g_dma_driver_is_init) {
        return OPRT_OK;
    }
    //free only no used dma channel
    if (g_dma_chnl_pool.chnl_bitmap == 0) {
        for (int id = 0; id < (SOC_DMA_CHAN_NUM_PER_UNIT*SOC_DMA_UNIT_NUM); id++) {
            dma_id_deinit(id);
        }

        // bk_dma_driver_deinit();
        g_dma_driver_is_init = false;
    }

    return OPRT_OK;
}

OPERATE_RET tkl_dma_init(dma_id_t *channel, const TKL_DMA_CONFIG_T *config)
{
    OPERATE_RET ret = 0;
    uint8_t dma_channel,dma_num;
	dma_config_t dma_config = {0};
    dma_id_t id = 0;
    if (config == NULL) {
        return OPRT_INVALID_PARM;
    }
    ret = tkl_dma_driver_init();
    if (ret != OPRT_OK) {
        return OPRT_COM_ERROR;
    }
    *channel = bk_dma_alloc(config->dev_id);
    g_dma_devid[*channel] = config->dev_id;
    id = *channel;
    DMA_RETURN_NOT_INIT();
    dma_id_to_id_ch(id,&dma_num,&dma_channel);
    
    dma_hal_init_without_channels(&g_dma[dma_num].hal);	//TODO:special codes for DMA init after enter low voltage
#if CONFIG_SPE
    dma_hal_set_dest_sec_attr(&g_dma[dma_num].hal, dma_channel, DMA_ATTR_SEC);
    dma_hal_set_src_sec_attr(&g_dma[dma_num].hal, dma_channel, DMA_ATTR_SEC);
    dma_hal_set_dest_burst_len(&g_dma[dma_num].hal, dma_channel,config->dst.brust_len);
    dma_hal_set_src_burst_len(&g_dma[dma_num].hal, dma_channel,config->src.brust_len);
#endif
    DMA_RETURN_INVALID_ID(id);
    DMA_RETURN_INVALID_ADDR(config->src.start_addr, config->src.end_addr);
    DMA_RETURN_INVALID_ADDR(config->dst.start_addr, config->dst.end_addr);
    DMA_LOG_ID_IS_STARTED(dma_num,dma_channel);

#if CONFIG_CACHE_ENABLE
    flush_dcache((void *)config->src.start_addr, config->src.end_addr - config->src.start_addr);
    flush_dcache((void *)config->dst.start_addr, config->dst.end_addr - config->dst.start_addr);
#endif

    dma_id_init(id);
    if ((config->src.dev == DMA_DEV_DTCM) && (config->src.width != TKL_DMA_DATA_WIDTH_32BITS)) {
        bk_printf("Must be 32bits width when device is DTCM\r\n");
        return OPRT_COM_ERROR;
    }
    
    if ((config->dst.dev == DMA_DEV_DTCM) && (config->dst.width != TKL_DMA_DATA_WIDTH_32BITS)) {
        bk_printf("Must be 32bits width when device is DTCM\r\n");
        return OPRT_COM_ERROR;
    }

	dma_config.mode = config->mode;
    dma_config.trans_type = DMA_TRANS_DEFAULT;
	dma_config.chan_prio = 0;
	dma_config.src.dev = config->src.dev;
	dma_config.src.width = config->src.width;
	dma_config.src.addr_inc_en = config->src.addr_inc_en;
	dma_config.src.addr_loop_en = config->src.addr_loop_en;
	dma_config.src.start_addr = config->src.start_addr;
    if (dma_config.src.addr_loop_en) {
        dma_config.src.end_addr = config->src.end_addr;
    }
	dma_config.dst.dev = config->dst.dev;
	dma_config.dst.width = config->dst.width;
    dma_config.dst.addr_inc_en = config->dst.addr_inc_en;
    dma_config.dst.addr_loop_en = config->dst.addr_loop_en;
	dma_config.dst.start_addr = config->dst.start_addr;
    if (dma_config.dst.addr_loop_en) {
        dma_config.dst.end_addr = config->dst.end_addr;
    }
    memcpy(&g_dma_config[dma_num][dma_channel], &dma_config, sizeof(dma_config_t));
    return dma_hal_init_dma(&g_dma[dma_num].hal, dma_channel, &dma_config);
}

OPERATE_RET tkl_dma_deinit(dma_id_t id)
{
	uint8_t dma_channel,dma_num;
	dma_id_to_id_ch(id,&dma_num,&dma_channel);
    DMA_RETURN_INVALID_ID(id);
    bk_dma_stop(id);
    tkl_dma_register_isr(id, NULL);
    tkl_dma_driver_deinit();
    bk_dma_free(g_dma_devid[id],id);
    memset(&g_dma_config[dma_num][dma_channel], 0, sizeof(dma_config_t));
    return OPRT_OK;
}

OPERATE_RET tkl_dma_start(dma_id_t id)
{
	uint8_t dma_channel,dma_num;
	dma_id_to_id_ch(id,&dma_num,&dma_channel);

    DMA_RETURN_NOT_INIT();
    DMA_RETURN_ID_NOT_INIT(dma_num,dma_channel);
    dma_hal_start_common(&g_dma[dma_num].hal, dma_channel);
    return OPRT_OK;
}

OPERATE_RET tkl_dma_stop(dma_id_t id)
{
	uint8_t dma_channel,dma_num;
	dma_id_to_id_ch(id,&dma_num,&dma_channel);

    DMA_RETURN_NOT_INIT();
	DMA_RETURN_ID_NOT_INIT(dma_num, dma_channel);
    dma_hal_stop_common(&g_dma[dma_num].hal, dma_channel);
    return OPRT_OK;
}

uint32_t tkl_dma_get_enable_status(dma_id_t id)
{
	uint8_t dma_channel,dma_num;
	dma_id_to_id_ch(id,&dma_num,&dma_channel);

    DMA_RETURN_NOT_INIT();
    DMA_RETURN_ID_NOT_INIT(dma_num,dma_channel);
    uint32_t ret;
    ret = dma_hal_get_enable_status(&g_dma[dma_num].hal, dma_channel);
    return ret;
}

#define DMA_MAX_BUSY_TIME (10000)  //us
static uint32_t tkl_dma_wait_to_idle(dma_id_t id)
{
	uint8_t dma_channel,dma_num;
	dma_id_to_id_ch(id,&dma_num,&dma_channel);

	if(dma_hal_get_work_mode(&g_dma[dma_num].hal, dma_channel) == DMA_WORK_MODE_SINGLE) {
		uint32_t i = 0;
		while(dma_hal_get_enable_status(&g_dma[dma_num].hal, dma_channel)) {
			bk_delay_us(1);

			i++;
			if(i > DMA_MAX_BUSY_TIME) {
				bk_printf("ch%d busy,remain len=%d,dst_addr=%x\r\n", dma_channel,
							dma_hal_get_remain_len(&g_dma[dma_num].hal, dma_channel),
							dma_hal_get_dest_write_addr(&g_dma[dma_num].hal, dma_channel));
				break;
			}
		}

		return i;
	} else {
		//TODO:
	}

	return 0;
}

static OPERATE_RET get_dma_repeat_once_len(uint32_t frame_len, uint32_t* outlen)
{
    uint32_t len = 0;
    uint32_t value = 0;
    uint8_t i = 0;

    for (i = 2; i < 1024; i++) {          
        len = frame_len / i;
        if (len <= DMA_TRANS_LEN_MAX  && (frame_len % i == 0)) {
            value = frame_len % i;
            if (!value) {
                *outlen = len;
                return OPRT_OK;
            }
        }
    }

    bk_printf("%s Error dma length, please check the resolution of qspi lcd\r\n", __func__);

    return OPRT_INVALID_PARM;
}

/* DTCM->peripheral
 */
OPERATE_RET tkl_dma_write(dma_id_t id, const uint8_t *data, uint32_t size)
{
	uint8_t dma_channel,dma_num;
	uint32_t len = 0;
	dma_id_to_id_ch(id,&dma_num,&dma_channel);
    DMA_RETURN_NOT_INIT();
    DMA_RETURN_ID_NOT_INIT(dma_num,dma_channel);
	tkl_dma_wait_to_idle(id);

    dma_hal_stop_common(&g_dma[dma_num].hal, dma_channel);
#if CONFIG_CACHE_ENABLE
    flush_dcache((void *)g_dma_config[dma_num][dma_channel].src.start_addr, g_dma_config[dma_num][dma_channel].src.end_addr - g_dma_config[dma_num][dma_channel].src.start_addr);
    flush_dcache((void *)g_dma_config[dma_num][dma_channel].dst.start_addr, g_dma_config[dma_num][dma_channel].dst.end_addr - g_dma_config[dma_num][dma_channel].dst.start_addr);
#endif
	if (size > DMA_TRANS_LEN_MAX) {
        uint32_t remain_len = size % 4;
        size &= ~(4 - 1);
		if(!get_dma_repeat_once_len(size, &len)) {
            //避免频繁初始化
            if (dma_hal_get_work_mode(&g_dma[dma_num].hal, dma_channel) != DMA_WORK_MODE_REPEAT) {
                dma_ll_set_work_mode(g_dma[dma_num].hal.hw, dma_channel, DMA_WORK_MODE_REPEAT);
            }
            dma_hal_set_src_pause_addr(&g_dma[dma_num].hal, dma_channel, (((uint32_t)data + size)));
            if ((g_dma_config[dma_num][dma_channel].src.dev == DMA_DEV_DTCM) && (g_dma_config[dma_num][dma_channel].src.addr_inc_en == DMA_ADDR_INC_ENABLE)) {
                dma_hal_set_src_loop_addr(&g_dma[dma_num].hal, dma_channel, (uint32_t)data, (uint32_t)(data + size + remain_len + 4));
            }
            if ((g_dma_config[dma_num][dma_channel].dst.dev == DMA_DEV_DTCM) && (g_dma_config[dma_num][dma_channel].dst.addr_inc_en == DMA_ADDR_INC_ENABLE)) {
                dma_hal_set_dest_loop_addr(&g_dma[dma_num].hal, dma_channel, (uint32_t)g_dma_config[dma_num][dma_channel].dst.start_addr, (uint32_t)(g_dma_config[dma_num][dma_channel].dst.start_addr + size + remain_len + 4));
            }
        } else {
            return OPRT_INVALID_PARM;
        }
	} else {
		len = size;
        if (dma_hal_get_work_mode(&g_dma[dma_num].hal, dma_channel) != DMA_WORK_MODE_SINGLE) {
            dma_ll_set_work_mode(g_dma[dma_num].hal.hw, dma_channel, DMA_WORK_MODE_SINGLE);
        }
        if ((g_dma_config[dma_num][dma_channel].src.dev == DMA_DEV_DTCM) && (g_dma_config[dma_num][dma_channel].src.addr_inc_en == DMA_ADDR_INC_ENABLE)) {
            dma_hal_set_src_loop_addr(&g_dma[dma_num].hal, dma_channel, (uint32_t)data, (uint32_t)(data + size + 4));
        }
        if ((g_dma_config[dma_num][dma_channel].dst.dev == DMA_DEV_DTCM) && (g_dma_config[dma_num][dma_channel].dst.addr_inc_en == DMA_ADDR_INC_ENABLE)) {
            dma_hal_set_dest_loop_addr(&g_dma[dma_num].hal, dma_channel, (uint32_t)g_dma_config[dma_num][dma_channel].dst.start_addr, (uint32_t)(g_dma_config[dma_num][dma_channel].dst.start_addr + size + 4));
        }
	}
    dma_hal_set_src_start_addr(&g_dma[dma_num].hal, dma_channel, (uint32_t)data);
    dma_hal_set_transfer_len(&g_dma[dma_num].hal, dma_channel, len);

    dma_hal_start_common(&g_dma[dma_num].hal, dma_channel);

    return OPRT_OK;
}

/* peripheral->DTCM
 */
OPERATE_RET tkl_dma_read(dma_id_t id, uint8_t *data, uint32_t size)
{
	uint8_t dma_channel,dma_num;
	uint32_t len = 0;
	dma_id_to_id_ch(id,&dma_num,&dma_channel);

    DMA_RETURN_NOT_INIT();
    DMA_RETURN_ID_NOT_INIT(dma_num,dma_channel);
	tkl_dma_wait_to_idle(id);
    
    dma_hal_stop_common(&g_dma[dma_num].hal, dma_channel);
	if (size > DMA_TRANS_LEN_MAX) {
        if (size % 4) {
            bk_printf("The receive length must be 4-byte aligned.\n");
            return OPRT_COM_ERROR;
        }
        
        uint32_t remain_len = size % 4;
        size &= ~(4 - 1);
		if(!get_dma_repeat_once_len(size, &len)) {
            if (dma_hal_get_work_mode(&g_dma[dma_num].hal, dma_channel) != DMA_WORK_MODE_REPEAT) {
                dma_ll_set_work_mode(g_dma[dma_num].hal.hw, dma_channel, DMA_WORK_MODE_REPEAT);
            }
            dma_hal_set_dest_pause_addr(&g_dma[dma_num].hal, dma_channel, (((uint32_t)data + size)));
            if ((g_dma_config[dma_num][dma_channel].dst.dev == DMA_DEV_DTCM) && (g_dma_config[dma_num][dma_channel].dst.addr_inc_en == DMA_ADDR_INC_ENABLE)) {
                dma_hal_set_dest_loop_addr(&g_dma[dma_num].hal, dma_channel, (uint32_t)data, (uint32_t)(data + size + remain_len + 4));
            }
            if ((g_dma_config[dma_num][dma_channel].dst.dev == DMA_DEV_DTCM) && (g_dma_config[dma_num][dma_channel].dst.addr_inc_en == DMA_ADDR_INC_ENABLE)) {
                dma_hal_set_src_loop_addr(&g_dma[dma_num].hal, dma_channel, (uint32_t)g_dma_config[dma_num][dma_channel].src.start_addr, (uint32_t)(g_dma_config[dma_num][dma_channel].src.start_addr + size + remain_len + 4));
            }
        } else {
            return OPRT_INVALID_PARM;
        }
	} else {
		len = size;
        if (dma_hal_get_work_mode(&g_dma[dma_num].hal, dma_channel) != DMA_WORK_MODE_SINGLE) {
            dma_ll_set_work_mode(g_dma[dma_num].hal.hw, dma_channel, DMA_WORK_MODE_SINGLE);
        }
        dma_hal_set_dest_pause_addr(&g_dma[dma_num].hal, dma_channel, (((uint32_t)data + size)));
        if ((g_dma_config[dma_num][dma_channel].dst.dev == DMA_DEV_DTCM) && (g_dma_config[dma_num][dma_channel].dst.addr_inc_en == DMA_ADDR_INC_ENABLE)) {
            dma_hal_set_dest_loop_addr(&g_dma[dma_num].hal, dma_channel, (uint32_t)data, (uint32_t)(data + size));
        }
        if ((g_dma_config[dma_num][dma_channel].dst.dev == DMA_DEV_DTCM) && (g_dma_config[dma_num][dma_channel].dst.addr_inc_en == DMA_ADDR_INC_ENABLE)) {
            dma_hal_set_src_loop_addr(&g_dma[dma_num].hal, dma_channel, (uint32_t)g_dma_config[dma_num][dma_channel].src.start_addr, (uint32_t)(g_dma_config[dma_num][dma_channel].src.start_addr + size));
        }
	}
    dma_hal_set_dest_start_addr(&g_dma[dma_num].hal, dma_channel, (uint32_t)data);
    dma_hal_set_transfer_len(&g_dma[dma_num].hal, dma_channel, len);
    dma_hal_start_common(&g_dma[dma_num].hal, dma_channel);

    return OPRT_OK;
}

OPERATE_RET tkl_dma_register_isr(dma_id_t id, DMA_ISR_T finish_isr)
{
    uint8_t dma_num,dma_channel;
    DMA_RETURN_NOT_INIT();
	dma_id_to_id_ch(id,&dma_num,&dma_channel);
    DMA_RETURN_INVALID_ID(id);
    GLOBAL_INT_DECLARATION();
    GLOBAL_INT_DISABLE();
    g_dma_finish_isr[dma_num][dma_channel] = finish_isr;
    if (finish_isr) {
        dma_id_enable_interrupt(dma_num);
        dma_hal_enable_finish_interrupt(&g_dma[dma_num].hal, dma_channel);
        dma_hal_enable_bus_err_interrupt(&g_dma[dma_num].hal, dma_channel);
    } else {
        dma_hal_disable_finish_interrupt(&g_dma[dma_num].hal, dma_channel);
        dma_hal_clear_finish_interrupt_status(&g_dma[dma_num].hal, dma_channel);
        dma_hal_disable_bus_err_interrupt(&g_dma[dma_num].hal, dma_channel);
        dma_hal_clear_bus_err_interrupt_status(&g_dma[dma_num].hal, dma_channel);
    }
    GLOBAL_INT_RESTORE();

    return OPRT_OK;
}

uint32_t tkl_dma_get_once_transfer_len_max(dma_id_t id)
{
    uint8_t dma_channel,dma_num;
    dma_id_to_id_ch(id,&dma_num,&dma_channel);

    return DMA_TRANS_LEN_MAX;
}

OPERATE_RET tkl_dma_set_transfer_len(dma_id_t id, uint32_t tran_len)
{
    uint8_t dma_channel,dma_num;
    dma_id_to_id_ch(id,&dma_num,&dma_channel);

    DMA_RETURN_NOT_INIT();
    DMA_RETURN_INVALID_ID(id);
    if(tran_len > DMA_TRANS_LEN_MAX) {
        return OPRT_INVALID_PARM;
    }

    tkl_dma_wait_to_idle(id);
    dma_hal_set_transfer_len(&g_dma[dma_num].hal, dma_channel, tran_len);
    return OPRT_OK;
}

uint32_t tkl_dma_get_transfer_len(dma_id_t id)
{
    uint8_t dma_channel,dma_num;
    dma_id_to_id_ch(id,&dma_num,&dma_channel);

    DMA_RETURN_NOT_INIT();
    DMA_RETURN_INVALID_ID(id);

    tkl_dma_wait_to_idle(id);
    return g_dma[dma_num].hal.hw->config_group[dma_channel].ctrl.transfer_len;
}

#ifdef DMA_REG_SUPPORT
OPERATE_RET tkl_dma_set_src_loop_addr(dma_id_t id, uint32_t start_addr, uint32_t end_addr)
{
    uint8_t dma_channel,dma_num;
    dma_id_to_id_ch(id,&dma_num,&dma_channel);

    DMA_RETURN_NOT_INIT();
    DMA_RETURN_INVALID_ID(id);
    dma_hal_set_src_loop_addr(&g_dma[dma_num].hal, dma_channel, start_addr, end_addr);
    return OPRT_OK;
}

OPERATE_RET tkl_dma_set_src_start_addr(dma_id_t id, uint32_t start_addr)
{
    uint8_t dma_channel,dma_num;
    dma_id_to_id_ch(id,&dma_num,&dma_channel);

    DMA_RETURN_NOT_INIT();
    DMA_RETURN_INVALID_ID(id);
    dma_hal_set_src_start_addr(&g_dma[dma_num].hal, dma_channel, start_addr);
    return OPRT_OK;
}

OPERATE_RET tkl_dma_set_dest_loop_addr(dma_id_t id, uint32_t start_addr, uint32_t end_addr)
{
    uint8_t dma_channel,dma_num;
    dma_id_to_id_ch(id,&dma_num,&dma_channel);

    DMA_RETURN_NOT_INIT();
    DMA_RETURN_INVALID_ID(id);
    tkl_dma_wait_to_idle(id);
    dma_hal_set_dest_loop_addr(&g_dma[dma_num].hal, dma_channel, start_addr, end_addr);
    return OPRT_OK;
}

OPERATE_RET tkl_dma_set_dest_start_addr(dma_id_t id, uint32_t start_addr)
{
    uint8_t dma_channel,dma_num;
    dma_id_to_id_ch(id,&dma_num,&dma_channel);

    DMA_RETURN_NOT_INIT();
    DMA_RETURN_INVALID_ID(id);
    tkl_dma_wait_to_idle(id);
    dma_hal_set_dest_start_addr(&g_dma[dma_num].hal, dma_channel, start_addr);
    return OPRT_OK;
}

OPERATE_RET tkl_dma_enable_src_addr_increase(dma_id_t id)
{
    uint8_t dma_channel,dma_num;
    dma_id_to_id_ch(id,&dma_num,&dma_channel);

    DMA_RETURN_NOT_INIT();
    DMA_RETURN_INVALID_ID(id);
    dma_hal_enable_src_addr_inc(&g_dma[dma_num].hal, dma_channel);
    return OPRT_OK;
}

OPERATE_RET tkl_dma_disable_src_addr_increase(dma_id_t id)
{
    uint8_t dma_channel,dma_num;
    dma_id_to_id_ch(id,&dma_num,&dma_channel);

    DMA_RETURN_NOT_INIT();
    DMA_RETURN_INVALID_ID(id);
    dma_hal_disable_src_addr_inc(&g_dma[dma_num].hal, dma_channel);
    return OPRT_OK;
}

OPERATE_RET tkl_dma_enable_src_addr_loop(dma_id_t id)
{
    uint8_t dma_channel,dma_num;
    dma_id_to_id_ch(id,&dma_num,&dma_channel);

    DMA_RETURN_NOT_INIT();
    DMA_RETURN_INVALID_ID(id);
    dma_hal_enable_src_addr_loop(&g_dma[dma_num].hal, dma_channel);
    return OPRT_OK;
}

OPERATE_RET tkl_dma_disable_src_addr_loop(dma_id_t id)
{
    uint8_t dma_channel,dma_num;
    dma_id_to_id_ch(id,&dma_num,&dma_channel);

    DMA_RETURN_NOT_INIT();
    DMA_RETURN_INVALID_ID(id);
    dma_hal_disable_src_addr_loop(&g_dma[dma_num].hal, dma_channel);
    return OPRT_OK;
}

OPERATE_RET tkl_dma_enable_dest_addr_increase(dma_id_t id)
{
    uint8_t dma_channel,dma_num;
    dma_id_to_id_ch(id,&dma_num,&dma_channel);

    DMA_RETURN_NOT_INIT();
    DMA_RETURN_INVALID_ID(id);
    dma_hal_enable_dest_addr_inc(&g_dma[dma_num].hal, dma_channel);
    return OPRT_OK;
}

OPERATE_RET tkl_dma_disable_dest_addr_increase(dma_id_t id)
{
    uint8_t dma_channel,dma_num;
    dma_id_to_id_ch(id,&dma_num,&dma_channel);

    DMA_RETURN_NOT_INIT();
    DMA_RETURN_INVALID_ID(id);
    dma_hal_disable_dest_addr_inc(&g_dma[dma_num].hal, dma_channel);
    return OPRT_OK;
}

OPERATE_RET tkl_dma_enable_dest_addr_loop(dma_id_t id)
{
    uint8_t dma_channel,dma_num;
    dma_id_to_id_ch(id,&dma_num,&dma_channel);

    DMA_RETURN_NOT_INIT();
    DMA_RETURN_INVALID_ID(id);
    dma_hal_enable_dest_addr_loop(&g_dma[dma_num].hal, dma_channel);
    return OPRT_OK;
}

OPERATE_RET tkl_dma_disable_dest_addr_loop(dma_id_t id)
{
    uint8_t dma_channel,dma_num;
    dma_id_to_id_ch(id,&dma_num,&dma_channel);

    DMA_RETURN_NOT_INIT();
    DMA_RETURN_INVALID_ID(id);
    dma_hal_disable_dest_addr_loop(&g_dma[dma_num].hal, dma_channel);
    return OPRT_OK;
}

uint32_t tkl_dma_get_remain_len(dma_id_t id)
{
    uint8_t dma_channel,dma_num;
    dma_id_to_id_ch(id,&dma_num,&dma_channel);

    DMA_RETURN_NOT_INIT();
    DMA_RETURN_ID_NOT_INIT(dma_num,dma_channel);
    return dma_hal_get_remain_len(&g_dma[dma_num].hal, dma_channel);
}

OPERATE_RET tkl_dma_set_src_pause_addr(dma_id_t id, uint32_t addr)
{
    uint8_t dma_channel,dma_num;
    dma_id_to_id_ch(id,&dma_num,&dma_channel);

    DMA_RETURN_NOT_INIT();
    DMA_RETURN_INVALID_ID(id);

    dma_hal_set_src_pause_addr(&g_dma[dma_num].hal, dma_channel, addr);

    return OPRT_OK;
}

OPERATE_RET tkl_dma_set_dst_pause_addr(dma_id_t id, uint32_t addr)
{
    uint8_t dma_channel,dma_num;
    dma_id_to_id_ch(id,&dma_num,&dma_channel);

    DMA_RETURN_NOT_INIT();
    DMA_RETURN_INVALID_ID(id);

    dma_hal_set_dest_pause_addr(&g_dma[dma_num].hal, dma_channel, addr);

    return OPRT_OK;
}

uint32_t tkl_dma_get_src_pause_addr(dma_id_t id)
{
    uint8_t dma_channel,dma_num;
    dma_id_to_id_ch(id,&dma_num,&dma_channel);

    DMA_RETURN_NOT_INIT();
    DMA_RETURN_INVALID_ID(id);

    return dma_hal_get_src_pause_addr(&g_dma[dma_num].hal, dma_channel);
}

uint32_t tkl_dma_get_dst_pause_addr(dma_id_t id)
{
    uint8_t dma_channel,dma_num;
    dma_id_to_id_ch(id,&dma_num,&dma_channel);

    DMA_RETURN_NOT_INIT();
    DMA_RETURN_INVALID_ID(id);

    return dma_hal_get_dest_pause_addr(&g_dma[dma_num].hal, dma_channel);
}

uint32_t tkl_dma_get_src_read_addr(dma_id_t id)
{
    uint8_t dma_channel,dma_num;
    dma_id_to_id_ch(id,&dma_num,&dma_channel);

    DMA_RETURN_NOT_INIT();
    DMA_RETURN_INVALID_ID(id);

    return dma_hal_get_src_read_addr(&g_dma[dma_num].hal, dma_channel);
}

uint32_t tkl_dma_get_src_end_addr(dma_id_t id)
{
    uint8_t dma_channel,dma_num;
    dma_id_to_id_ch(id,&dma_num,&dma_channel);

    DMA_RETURN_NOT_INIT();
    DMA_RETURN_INVALID_ID(id);

    return dma_hal_get_src_end_addr(&g_dma[dma_num].hal, dma_channel);
}

uint32_t tkl_dma_get_dst_end_addr(dma_id_t id)
{
    uint8_t dma_channel,dma_num;
    dma_id_to_id_ch(id,&dma_num,&dma_channel);

    DMA_RETURN_NOT_INIT();
    DMA_RETURN_INVALID_ID(id);

    return dma_hal_get_dest_end_addr(&g_dma[dma_num].hal, dma_channel);
}

uint32_t tkl_dma_get_dest_write_addr(dma_id_t id)
{
    uint8_t dma_channel,dma_num;
    dma_id_to_id_ch(id,&dma_num,&dma_channel);

    DMA_RETURN_NOT_INIT();
    DMA_RETURN_INVALID_ID(id);

    return dma_hal_get_dest_write_addr(&g_dma[dma_num].hal, dma_channel);
}

uint32_t tkl_dma_get_work_mode(dma_id_t id)
{
    uint8_t dma_channel,dma_num;
    dma_id_to_id_ch(id,&dma_num,&dma_channel);

    DMA_RETURN_NOT_INIT();
    DMA_RETURN_INVALID_ID(id);

    return dma_hal_get_work_mode(&g_dma[dma_num].hal, dma_channel);
}

OPERATE_RET tkl_dma_set_src_data_width(dma_id_t id, dma_data_width_t data_width)
{
    uint8_t dma_channel,dma_num;
    dma_id_to_id_ch(id,&dma_num,&dma_channel);

    DMA_RETURN_NOT_INIT();
    DMA_RETURN_INVALID_ID(id);

    tkl_dma_wait_to_idle(id);
    dma_hal_set_src_data_width(&g_dma[dma_num].hal, dma_channel, data_width);
    return OPRT_OK;
}

OPERATE_RET tkl_dma_set_dest_data_width(dma_id_t id, dma_data_width_t data_width)
{
    uint8_t dma_channel,dma_num;
    dma_id_to_id_ch(id,&dma_num,&dma_channel);

    DMA_RETURN_NOT_INIT();
    DMA_RETURN_INVALID_ID(id);

    tkl_dma_wait_to_idle(id);
    dma_hal_set_dest_data_width(&g_dma[dma_num].hal, dma_channel, data_width);
    return OPRT_OK;
}

OPERATE_RET tkl_dma_flush_src_buffer(dma_id_t id)
{
    uint8_t dma_channel,dma_num;
    dma_id_to_id_ch(id,&dma_num,&dma_channel);

    DMA_RETURN_NOT_INIT();
    DMA_RETURN_INVALID_ID(id);

    dma_hal_flush_src_buffer(&g_dma[dma_num].hal, dma_channel);
    return OPRT_OK;
}

#ifdef CONFIG_SPE
OPERATE_RET tkl_dma_set_pixel_trans_type(dma_id_t id, dma_pixel_trans_type_t type)
{
    uint8_t dma_channel,dma_num;
    dma_id_to_id_ch(id,&dma_num,&dma_channel);

    DMA_RETURN_NOT_INIT();
    DMA_RETURN_INVALID_ID(id);


    dma_hal_set_pixel_trans_type(&g_dma[dma_num].hal, id, type);
    return OPRT_OK;
}

uint32_t tkl_dma_get_pixel_trans_type(dma_id_t id)
{
    uint8_t dma_channel,dma_num;
    dma_id_to_id_ch(id,&dma_num,&dma_channel);

    DMA_RETURN_NOT_INIT();
    DMA_RETURN_INVALID_ID(id);


    return dma_hal_get_pixel_trans_type(&g_dma[dma_num].hal, id);
}

OPERATE_RET tkl_dma_set_dest_burst_len(dma_id_t id, dma_burst_len_t len)
{
    uint8_t dma_channel,dma_num;
    dma_id_to_id_ch(id,&dma_num,&dma_channel);

    DMA_RETURN_NOT_INIT();
    DMA_RETURN_INVALID_ID(id);

    dma_hal_set_dest_burst_len(&g_dma[dma_num].hal, dma_channel, len);
    return OPRT_OK;
}

uint32_t tkl_dma_get_dest_burst_len(dma_id_t id)
{
    uint8_t dma_channel,dma_num;
    dma_id_to_id_ch(id,&dma_num,&dma_channel);

    DMA_RETURN_NOT_INIT();
    DMA_RETURN_INVALID_ID(id);

    return dma_hal_get_dest_burst_len(&g_dma[dma_num].hal, dma_channel);
}

OPERATE_RET tkl_dma_set_src_burst_len(dma_id_t id, dma_burst_len_t len)
{
    uint8_t dma_channel,dma_num;
    dma_id_to_id_ch(id,&dma_num,&dma_channel);

    DMA_RETURN_NOT_INIT();
    DMA_RETURN_INVALID_ID(id);

    dma_hal_set_src_burst_len(&g_dma[dma_num].hal, dma_channel, len);
    return OPRT_OK;
}

uint32_t tkl_dma_get_src_burst_len(dma_id_t id)
{
    uint8_t dma_channel,dma_num;
    dma_id_to_id_ch(id,&dma_num,&dma_channel);

    DMA_RETURN_NOT_INIT();
    DMA_RETURN_INVALID_ID(id);

    return dma_hal_get_src_burst_len(&g_dma[dma_num].hal, dma_channel);
}

OPERATE_RET tkl_dma_bus_err_int_enable(dma_id_t id)
{
    uint8_t dma_channel,dma_num;
    dma_id_to_id_ch(id,&dma_num,&dma_channel);

    DMA_RETURN_NOT_INIT();
    DMA_RETURN_INVALID_ID(id);

    dma_hal_bus_err_int_enable(&g_dma[dma_num].hal, dma_channel);
    return OPRT_OK;
}

OPERATE_RET tkl_dma_bus_err_int_diable(dma_id_t id)
{
    uint8_t dma_channel,dma_num;
    dma_id_to_id_ch(id,&dma_num,&dma_channel);

    DMA_RETURN_NOT_INIT();
    DMA_RETURN_INVALID_ID(id);

    dma_hal_bus_err_int_disable(&g_dma[dma_num].hal, dma_channel);
    return OPRT_OK;
}

OPERATE_RET tkl_dma_set_dest_sec_attr(dma_id_t id, dma_sec_attr_t attr)
{
    uint8_t dma_channel,dma_num;
    dma_id_to_id_ch(id,&dma_num,&dma_channel);

    DMA_RETURN_NOT_INIT();
    DMA_RETURN_INVALID_ID(id);

    dma_hal_set_dest_sec_attr(&g_dma[dma_num].hal, dma_channel, attr);
    return OPRT_OK;
}

OPERATE_RET tkl_dma_set_src_sec_attr(dma_id_t id, dma_sec_attr_t attr)
{
    uint8_t dma_channel,dma_num;
    dma_id_to_id_ch(id,&dma_num,&dma_channel);

    DMA_RETURN_NOT_INIT();
    DMA_RETURN_INVALID_ID(id);

    dma_hal_set_src_sec_attr(&g_dma[dma_num].hal, dma_channel, attr);
    return OPRT_OK;
}
#endif

#if (CONFIG_SPE)
OPERATE_RET tkl_dma_set_sec_attr(dma_id_t id, dma_sec_attr_t attr)
{
    uint8_t dma_channel,dma_num;
    dma_id_to_id_ch(id,&dma_num,&dma_channel);

    DMA_RETURN_NOT_INIT();
    DMA_RETURN_INVALID_ID(id);

    dma_hal_set_sec_attr(&g_dma[dma_num].hal, dma_channel, attr);
    return OPRT_OK;
}

OPERATE_RET tkl_dma_set_privileged_attr(dma_id_t id, dma_sec_attr_t attr)
{
    uint8_t dma_channel,dma_num;
    dma_id_to_id_ch(id,&dma_num,&dma_channel);

    DMA_RETURN_NOT_INIT();
    DMA_RETURN_INVALID_ID(id);

    dma_hal_set_privileged_attr(&g_dma[dma_num].hal, dma_channel, attr);
    return OPRT_OK;
}

OPERATE_RET tkl_dma_set_int_allocate(dma_id_t id,dma_int_id_t int_id)
{
    uint8_t dma_channel,dma_num;
    dma_id_to_id_ch(id,&dma_num,&dma_channel);

    DMA_RETURN_NOT_INIT();
    DMA_RETURN_INVALID_ID(id);
    dma_hal_set_int_allocate(&g_dma[dma_num].hal,dma_channel,int_id);
    return OPRT_OK;
}

#endif

uint32_t tkl_dma_get_repeat_wr_pause(dma_id_t id)
{
	uint8_t dma_channel,dma_num;
	dma_id_to_id_ch(id,&dma_num,&dma_channel);

	return dma_hal_repeat_wr_pause(&g_dma[dma_num].hal, dma_channel);
}

uint32_t tkl_dma_get_repeat_rd_pause(dma_id_t id)
{
	uint8_t dma_channel,dma_num;
	dma_id_to_id_ch(id,&dma_num,&dma_channel);

	return dma_hal_repeat_rd_pause(&g_dma[dma_num].hal, dma_channel);
}

uint32_t tkl_dma_get_finish_interrupt_cnt(dma_id_t id)
{
	uint8_t dma_channel,dma_num;
	dma_id_to_id_ch(id,&dma_num,&dma_channel);

	return dma_hal_finish_interrupt_cnt(&g_dma[dma_num].hal, dma_channel);
}

uint32_t tkl_dma_get_half_finish_interrupt_cnt(dma_id_t id)
{
	uint8_t dma_channel,dma_num;
	dma_id_to_id_ch(id,&dma_num,&dma_channel);

	return dma_hal_half_finish_interrupt_cnt(&g_dma[dma_num].hal, dma_channel);
}
#endif

static OPERATE_RET tkl_dma_memcpy_by_chnl(void *out, const void *in, uint32_t len)
{
    DMA_RETURN_NOT_INIT();
    dma_id_t cpy_chnl = 0;
    TKL_DMA_CONFIG_T dma_config;

    os_memset(&dma_config, 0, sizeof(TKL_DMA_CONFIG_T));

    dma_config.mode = DMA_WORK_MODE_SINGLE;

    dma_config.src.dev = DMA_DEV_DTCM;
    dma_config.src.width = DMA_DATA_WIDTH_32BITS;
    dma_config.src.addr_inc_en = DMA_ADDR_INC_ENABLE;
    dma_config.src.start_addr = (uint32_t)in;
    dma_config.src.end_addr = (uint32_t)(in + len);

    dma_config.dst.dev = DMA_DEV_DTCM;
    dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
    dma_config.dst.addr_inc_en = DMA_ADDR_INC_ENABLE;
    dma_config.dst.start_addr = (uint32_t)out;
    dma_config.dst.end_addr = (uint32_t)(out + len);

    GLOBAL_INT_DECLARATION();
    GLOBAL_INT_DISABLE();

    tkl_dma_init(&cpy_chnl, &dma_config);
    DMA_RETURN_INVALID_ID(cpy_chnl);
    uint8_t dma_channel,dma_num;
    dma_id_to_id_ch(cpy_chnl,&dma_num,&dma_channel);
    tkl_dma_wait_to_idle(cpy_chnl);
    dma_hal_set_transfer_len(&g_dma[dma_num].hal, dma_channel, len);
#if (CONFIG_SPE)
    dma_hal_set_src_sec_attr(&g_dma[dma_num].hal, dma_channel, DMA_ATTR_SEC);
    dma_hal_set_dest_sec_attr(&g_dma[dma_num].hal, dma_channel, DMA_ATTR_SEC);
#endif
    dma_hal_start_common(&g_dma[dma_num].hal, dma_channel);
    GLOBAL_INT_RESTORE();

//TODO:I think no need to wait copy data finish,just confirm before copy start, the previous one is finish.
    uint32_t wait_dma_cpy_done_cnt = 0;
    while(dma_hal_get_enable_status(&g_dma[dma_num].hal, dma_channel)) {
        if(wait_dma_cpy_done_cnt > 1) {
            dma_hal_stop_common(&g_dma[dma_num].hal, dma_channel);
            os_memcpy(out, in, len);
            break;
        }
        rtos_delay_milliseconds(2);
        wait_dma_cpy_done_cnt++;
    }

    tkl_dma_deinit(cpy_chnl);
    return OPRT_OK;
}

OPERATE_RET tkl_dma_memcpy(void *out, const void *in, uint32_t len)
{
    DMA_RETURN_NOT_INIT();

    OPERATE_RET ret;

    ret = tkl_dma_memcpy_by_chnl(out, in, len);

    return ret;
}

void tkl_dma_isr_common(dma_unit_t dma_unit_id, uint32_t id)
{
    dma_hal_t *hal = &g_dma[dma_unit_id].hal;
    if (g_dma[dma_unit_id].id_init_bits == 0) {
        return;
    }
    if (dma_hal_is_finish_interrupt_triggered(hal, id)) {
        if (g_dma_finish_isr[dma_unit_id][id]) {
            dma_hal_clear_finish_interrupt_status(hal, id);
            if (dma_hal_get_work_mode(&g_dma[dma_unit_id].hal, id) == DMA_WORK_MODE_REPEAT) {
                uint32_t src_addr = dma_hal_get_src_pause_addr(hal, id);
                uint32_t dst_addr = dma_hal_get_dest_pause_addr(hal, id);
                uint32_t src_cur_addr = dma_hal_get_src_read_addr(hal, id);
                uint32_t dst_cur_addr = dma_hal_get_dest_write_addr(hal, id);
                uint32_t src_end_addr = dma_hal_get_src_end_addr(hal, id);
                uint32_t dst_end_addr = dma_hal_get_dest_end_addr(hal, id);
                bool src_complete = (src_cur_addr >= src_addr);
                bool dst_complete = (dst_cur_addr >= dst_addr);
                // 如果还没有达到暂停地址，说明只是部分传输完成
                if (!src_complete || !dst_complete) {
                    return;
                }
                if (((dst_end_addr - dst_addr - 4) < 4) && ((dst_end_addr - dst_addr - 4) > 0)) {
                    tkl_dma_write(id, dst_addr, (dst_end_addr - dst_addr) % 4);
                    return;
                }
                if (((src_end_addr - src_addr - 4) < 4) && ((src_end_addr - src_addr - 4) > 0)) {
                    tkl_dma_write(id, src_addr, (src_end_addr - src_addr) % 4);
                    return;
                }
            }
            g_dma_finish_isr[dma_unit_id][id](id, FINISH_ISR);
        }
    }
    if (dma_hal_is_bus_err_interrupt_triggered(hal, id)) {
        if (g_dma_finish_isr[dma_unit_id][id]) {
            dma_hal_clear_bus_err_interrupt_status(hal, id);
            g_dma_finish_isr[dma_unit_id][id](id, BUS_ERROR_ISR);
        }
    }
}

