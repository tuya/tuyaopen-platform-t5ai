// Copyright 2023-2024 Beken
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <os/os.h>
#include <os/mem.h>
#include <driver/gpio.h>
#include <driver/int.h>
#include <driver/can.h>
#include <driver/can_types.h>
#include "clock_driver.h"
#include "gpio_driver.h"
#include "power_driver.h"
#include "can_statis.h"
#include "can_driver.h"
#include "can_hal.h"
#include "sys_driver.h"
#include "FreeRTOS.h"
#include "task.h"

#if CONFIG_CAN_PM_SUPPORT
#include <modules/pm.h>
#endif

#define CAN_RETURN_ON_DEVICE_NOT_INIT() do { \
	if (!s_can_driver_is_init) { \
			CAN_LOGE("can driver not init\r\n"); \
			return BK_ERR_CAN_NOT_INIT; \
		} \
	} while(0)

#define DEFAULT_FIFO_SIZE          (1 << 8)
#define CAN_SPINLOCK_TIMEOUT_MS 20

static can_env_t *s_can_env;
static can_dev_t s_can_dev;
static can_gpio_t s_can_gpio[CAN_CHAN_MAX] = GPIO_CAN_MAP_TABLE;
static bool s_can_driver_is_init = false;
static can_callback_des_t s_can_isr_user_rx_cb;
static can_callback_des_t s_can_isr_user_tx_cb;
static can_callback_des_t s_can_isr_user_err_cb;

bk_err_t bk_can_gpio_init(can_channel_t chn)
{
	if (chn >= CAN_CHAN_MAX || chn < CAN_CHAN_0) {
		CAN_LOGV("unsupported can chnnal\r\n");
		return BK_ERR_PARAM;
	}
	BK_LOG_ON_ERR(gpio_dev_unmap(s_can_gpio[chn].tx.id));
	BK_LOG_ON_ERR(gpio_dev_map(s_can_gpio[chn].tx.id, s_can_gpio[chn].tx.dev));
	bk_gpio_set_value(s_can_gpio[chn].tx.id, 0x348);

	BK_LOG_ON_ERR(gpio_dev_unmap(s_can_gpio[chn].rx.id));
	BK_LOG_ON_ERR(gpio_dev_map(s_can_gpio[chn].rx.id, s_can_gpio[chn].rx.dev));
	bk_gpio_set_value(s_can_gpio[chn].rx.id, 0x37c);

	BK_LOG_ON_ERR(gpio_dev_unmap(s_can_gpio[chn].standby.id));
	BK_LOG_ON_ERR(gpio_dev_map(s_can_gpio[chn].standby.id, s_can_gpio[chn].standby.dev));
	bk_gpio_set_value(s_can_gpio[chn].standby.id, 0x348);
	return BK_OK;
}

bk_err_t bk_can_gpio_deinit(can_channel_t chn)
{
	if (chn >= CAN_CHAN_MAX || chn < CAN_CHAN_0) {
		CAN_LOGV("unsupported can chnnal\r\n");
		return BK_ERR_PARAM;
	}
	BK_LOG_ON_ERR(gpio_dev_unmap(s_can_gpio[chn].tx.id));
	BK_LOG_ON_ERR(gpio_dev_unmap(s_can_gpio[chn].rx.id));
	BK_LOG_ON_ERR(gpio_dev_unmap(s_can_gpio[chn].standby.id));

	return BK_OK;
}

bk_err_t bk_can_clock_enable(void)
{
    sys_hal_can_set_sel_clk(1);
	sys_drv_dev_clk_pwr_up(CLK_PWR_ID_CAN, CLK_PWR_CTRL_PWR_UP);

	return BK_OK;
}

bk_err_t bk_can_clock_disable(void)
{
	sys_drv_dev_clk_pwr_up(CLK_PWR_ID_CAN, CLK_PWR_CTRL_PWR_DOWN);

	return BK_OK;
}

bk_err_t bk_can_interrupt_enable(void)
{
	sys_drv_int_enable(CAN_INTERRUPT_CTRL_BIT);

	return BK_OK;
}

bk_err_t bk_can_interrupt_disable(void)
{
	sys_drv_int_disable(CAN_INTERRUPT_CTRL_BIT);

	return BK_OK;
}

static void bk_can_base_init(void)
{
    bk_can_clock_enable();

    bk_can_gpio_init(CAN_CHAN_0);

    bk_can_interrupt_enable();
}

static void bk_can_base_deinit(void)
{
    bk_can_interrupt_disable();

    bk_can_gpio_deinit(CAN_CHAN_0);

    bk_can_clock_disable();
}

static void can_rx_cb(void *param)
{
    rtos_set_semaphore(&(s_can_env->rx_semphr));
}

static void can_tx_cb(void *param)
{
    rtos_set_semaphore(&(s_can_env->tx_semphr));
}

static uint32_t can_tx_fifo_get(uint8_t *buf, uint32_t len)
{
    uint32_t size = 0;

    if (s_can_env->can_f.tx) {
        size = kfifo_get(s_can_env->can_f.tx, buf, len);
    }

    return size;
}

static uint32_t can_tx_fifo_put(uint8_t *buf, uint32_t len)
{
    uint32_t size = 0;

    if (s_can_env->can_f.tx) {
        size = kfifo_put(s_can_env->can_f.tx, buf, len);
    }

    return size;
}

static uint32_t can_tx_size_get(void)
{
    uint32_t size = 0;

    if (s_can_env->can_f.tx) {
        size = kfifo_data_size(s_can_env->can_f.tx);
    }

    return size;
}

static void can_tx_fifo_clr(void)
{
    if (s_can_env->can_f.tx) {
        // kfifo_clear(s_can_env->can_f.tx);
        s_can_env->can_f.tx->in = 0;
        s_can_env->can_f.tx->out = 0;
    }
}

static uint32_t can_rx_fifo_get(uint8_t *buf, uint32_t len)
{
    uint32_t size = 0;
    uint32_t flag;
	flag = rtos_disable_int();
	spinlock_acquire(&s_can_env->rx_spin, CAN_SPINLOCK_TIMEOUT_MS);
    if (s_can_env->can_f.rx) {
        size = kfifo_get(s_can_env->can_f.rx, buf, len);
    }

	spinlock_release(&s_can_env->rx_spin, flag);
	rtos_enable_int(flag);
    return size;
}

static uint32_t can_rx_fifo_put(uint8_t *buf, uint32_t len)
{
    uint32_t size = 0;
    uint32_t flag;
	flag = rtos_disable_int();
	spinlock_acquire(&s_can_env->rx_spin, CAN_SPINLOCK_TIMEOUT_MS);

    if (s_can_env->can_f.rx) {
        size = kfifo_put(s_can_env->can_f.rx, buf, len);
    }

	spinlock_release(&s_can_env->rx_spin, flag);
	rtos_enable_int(flag);
    return size;
}

static uint32_t can_rx_size_get(void)
{
    uint32_t size = 0;
    uint32_t flag;
	flag = rtos_disable_int();
	spinlock_acquire(&s_can_env->rx_spin, CAN_SPINLOCK_TIMEOUT_MS);

    if (s_can_env->can_f.rx) {
        size = kfifo_data_size(s_can_env->can_f.rx);
    }

	spinlock_release(&s_can_env->rx_spin, flag);
	rtos_enable_int(flag);

    return size;
}

bk_err_t bk_can_receive(uint8_t *data, uint32_t expect_size, uint32_t *recv_size, uint32_t timeout)
{
    uint32_t rx_size = 0, read_size = 0;
    bk_err_t ret = BK_OK;

    if (data == NULL || 0 == expect_size || NULL == s_can_env || NULL == s_can_env->can_f.rx) {
        return BK_ERR_PARAM;
    }

    CAN_RETURN_ON_DEVICE_NOT_INIT();

    if (recv_size) {
        *recv_size = 0;
    }

    while (1) {
        rx_size = can_rx_size_get();

        read_size = min(rx_size, expect_size);
        if (read_size) {
            can_rx_fifo_get(data, read_size);

            expect_size -= read_size;
            data += read_size;
            if (recv_size) {
                *recv_size += read_size;
            }
        }

        if (expect_size) {
            if (rtos_get_semaphore(&(s_can_env->rx_semphr), timeout) != BK_OK) {
                ret = BK_ERR_TIMEOUT;
                break;
            }
        } else {
            ret = BK_OK;
            break;
        }
    }

    return ret;
}

bk_err_t bk_can_send_ptb(can_frame_s* frame)
{
    CAN_RETURN_ON_DEVICE_NOT_INIT();
    if ((frame->tag.fdf == CAN_PROTO_20) && (frame->size > 8)) {
        return BK_ERR_PARAM;
    }

    if ((frame->tag.fdf == CAN_PROTO_20) && (frame->tag.brs == CAN_BIT_RATE_FAST)) {
        return BK_ERR_PARAM;
    }

    if ((frame->tag.fdf == CAN_PROTO_FD) && ((frame->size > 64) || ((frame->size > 8) && (frame->size%4)))) {
        return BK_ERR_PARAM;
    }

    if ((frame->tag.fdf == CAN_PROTO_FD) && (frame->tag.rtr == CAN_FRAME_REMT)) {
        return BK_ERR_PARAM;
    }

    can_hal_ctrl(CMD_CAN_PTB_INBUF, frame);

    uint32_t param = CAN_TPE;
    can_hal_ctrl(CMD_CAN_TRANS_SWITCH, (void *)param);

    return BK_OK;
}

bk_err_t bk_can_abort_ptb(void)
{
    uint32_t param = CAN_TPA;

    CAN_RETURN_ON_DEVICE_NOT_INIT();
    can_hal_ctrl(CMD_CAN_TRANS_SWITCH, (void *)param);

    return BK_OK;
}

bk_err_t bk_can_send(can_frame_s* frame, uint32_t timeout)
{
    uint32_t t_size;
    uint8_t *data = frame->data;
    uint32_t size = frame->size;
    uint32_t param = 0;
    bk_err_t ret = BK_OK;
    uint32_t flag;

	CAN_RETURN_ON_DEVICE_NOT_INIT();
    if (s_can_env == NULL || s_can_env->can_f.tx == NULL) {
        return BK_ERR_NULL_PARAM;
    }

    can_frame_tag_t tag = frame->tag;
    can_hal_ctrl(CMD_CAN_SET_TX_FRAME_TAG, &tag);

	flag = rtos_disable_int();
	spinlock_acquire(&s_can_env->tx_spin, CAN_SPINLOCK_TIMEOUT_MS);

    while (size) {
        t_size = kfifo_unused(s_can_env->can_f.tx);

        if (t_size == 0) {
            can_hal_ctrl(CMD_CAN_STB_INBUF, &param);

            if (rtos_get_semaphore(&(s_can_env->tx_semphr), timeout) != BK_OK) {
                ret = BK_ERR_TIMEOUT;
                break;
            }
            continue;
        }

        if (t_size >= size) {
            t_size = size;
        }

        can_tx_fifo_put(data, t_size);

        size -= t_size;
        data += t_size;
    }

    can_hal_ctrl(CMD_CAN_STB_INBUF, &param);

    param = CAN_TSALL;
    can_hal_ctrl(CMD_CAN_TRANS_SWITCH, (void *)param);

	spinlock_release(&s_can_env->tx_spin, flag);
	rtos_enable_int(flag);

    return ret;
}

bk_err_t bk_can_abort_all(void)
{
    CAN_RETURN_ON_DEVICE_NOT_INIT();
    can_tx_fifo_clr();
    uint32_t param = CAN_TPA | CAN_TSA;
    can_hal_ctrl(CMD_CAN_TRANS_SWITCH, (void *)param);

    return BK_OK;
}

bk_err_t bk_can_acc_filter_set(can_acc_filter_cmd_s* cmd)
{
	CAN_RETURN_ON_DEVICE_NOT_INIT();
    can_hal_ctrl(CMD_CAN_RESET_REQ, (void *)CAN_RESET_REQ_EN);

    can_hal_ctrl(CMD_CAN_ACC_FILTER_SET, (void *)cmd);

    can_hal_ctrl(CMD_CAN_RESET_REQ, (void *)CAN_RESET_REQ_NO);

    return BK_OK;
}



static bk_err_t bk_can_busoff_clr(void)
{
    CAN_RETURN_ON_DEVICE_NOT_INIT();
    can_hal_ctrl(CMD_CAN_BUSOFF_CLR, NULL);

    return BK_OK;
}

static bk_err_t bk_can_get_koer(void)
{
    can_koer_code_e koer_c;

    CAN_RETURN_ON_DEVICE_NOT_INIT();
    can_hal_ctrl(CMD_CAN_GET_KOER, &koer_c);

    return koer_c;
}

void bk_can_register_err_callback(can_callback_des_t *err_cb)
{
	if(err_cb) {
		s_can_isr_user_err_cb.cb = err_cb->cb;
		s_can_isr_user_err_cb.param = err_cb->param;
	}
}

static void can_err_int(void *param)
{
    uint32_t err_code = (uint32_t)param;
    CAN_LOGD("%s,%d err code 0x%x\r\n", __func__, __LINE__, err_code);

    if(s_can_isr_user_err_cb.cb) {
        s_can_isr_user_err_cb.cb(s_can_isr_user_err_cb.param);
    }

    if (err_code & CAN_ERRINT_WARN_LIM) {
        bk_can_abort_all();
        bk_can_busoff_clr();
    }
}

void bk_can_register_isr_callback(can_callback_des_t *rx_cb, can_callback_des_t *tx_cb)
{
	if(rx_cb) {
		s_can_isr_user_rx_cb.cb = rx_cb->cb;
		s_can_isr_user_rx_cb.param = rx_cb->param;
	}

	if(tx_cb) {
		s_can_isr_user_tx_cb.cb = tx_cb->cb;
		s_can_isr_user_tx_cb.param = tx_cb->param;
	}
}

void can_isr(void)
{
    uint32_t intc_stat;
    uint32_t err_c = 0;
    __attribute__((__unused__)) can_statis_t *can_statis = can_statis_get_statis();

    intc_stat = can_hal_get_ie_value();
    CAN_LOGV(" %s, %d intc_stat 0x%x\r\n\r\n", __func__, __LINE__, intc_stat);
    CAN_STATIS_INC(can_statis->isr_cnt);

    if (intc_stat & RX_INT_FLAG_GROUP) {
        CAN_LOGV("RECV message\r\n");
        can_hal_receive_frame();
        CAN_LOGV("%s,%d\r\n", __func__, __LINE__);
        CAN_STATIS_INC(can_statis->rx_cnt);
        if(s_can_isr_user_rx_cb.cb) {
            s_can_isr_user_rx_cb.cb(s_can_isr_user_rx_cb.param);
        }
    }

    if (intc_stat & TX_INT_FLAG_GROUP) {
        CAN_LOGV("SEND message\r\n");
        can_hal_send_frame();
        CAN_STATIS_INC(can_statis->tx_cnt);
        if(s_can_isr_user_tx_cb.cb){
            s_can_isr_user_tx_cb.cb(s_can_isr_user_tx_cb.param);
        }
    }

    if (intc_stat & ERR_INT_FLAG_GROUP) {
        if (intc_stat & (1 << CAN_IE_BEIF_POS)) {
            CAN_LOGD(" BUS ERROR\r\n\r\n");
            err_c |= CAN_ERRINT_BUS;
            CAN_STATIS_INC(can_statis->beif_cnt);
        }

        if (intc_stat & (1 << CAN_IE_ALIF_POS)) {
            CAN_LOGD(" Arbitration Lost\r\n\r\n");
            err_c |= CAN_ERRINT_ARB_LOST;
            CAN_STATIS_INC(can_statis->alif_cnt);
        }

        if (intc_stat & (1 << CAN_IE_EPIF_POS)) {
            CAN_LOGD(" Error Passive\r\n\r\n");
            err_c |= CAN_ERRINT_PASSIVE;
            CAN_STATIS_INC(can_statis->epif_cnt);
        }

        if (intc_stat & (1 << CAN_IE_EWARN_POS)) {
            CAN_LOGD(" Error Warning Limit\r\n\r\n");
            err_c |= CAN_ERRINT_WARN_LIM;
            CAN_STATIS_INC(can_statis->ewarn_cnt);
        }

        can_hal_error_analysis(err_c);
    }

    if (intc_stat & (1 << CAN_IE_AIF_POS)) {
        CAN_LOGD("Abort Handled \r\n\r\n");
        CAN_STATIS_INC(can_statis->aif_cnt);
    }

	can_hal_set_ie_value(intc_stat);
}

bk_err_t can_driver_bit_rate_config(can_bit_rate_e s_speed, can_bit_rate_e f_speed)
{
    if (s_speed < CAN_BR_125K || s_speed > CAN_BR_5M || f_speed < CAN_BR_250K || f_speed > CAN_BR_5M) {
        CAN_LOGE("beyond configurable range!!!\r\n");
        return BK_ERR_PARAM;
    }
    can_hal_set_reset(1);
    can_hal_bit_rate_config(s_speed, f_speed);
    can_hal_set_reset(0);

    return BK_OK;
}

bk_err_t bk_can_init(can_dev_t *can)
{
    if (can == NULL) {
        return BK_ERR_PARAM;
    }

    if (can->config.s_speed > CAN_BR_1M) {
        return BK_ERR_PARAM;
    }

    if (s_can_env == NULL) {
        s_can_env = os_zalloc(sizeof(can_env_t));
        if(!s_can_env) {
            CAN_LOGE("%s,%d s_can_env malloc fail\r\n", __func__, __LINE__);
            return BK_ERR_CAN_CHK_ERROR;
        }
    }

    bk_can_base_init();
    rtos_init_semaphore(&(s_can_env->rx_semphr), 1);
    rtos_init_semaphore(&(s_can_env->tx_semphr), 1);
    spinlock_init(&s_can_env->rx_spin);
    spinlock_init(&s_can_env->tx_spin);

    can_speed_t can_speed;
    can_speed.s_speed = can->config.s_speed;
    can_speed.f_speed = can->config.f_speed;
    can_hal_ctrl(CMD_CAN_MODUILE_INIT, &can_speed);

    s_can_env->status = CAN_STATUS_IDLE;

    if (can->config.rx_size > 0) {
        if (s_can_env->can_f.rx) {
            kfifo_free(s_can_env->can_f.rx);
        }
        s_can_env->can_f.rx = kfifo_alloc(can->config.rx_size);
        can_hal_ctrl(CMD_CAN_SET_RX_FIFO, can_rx_fifo_put);
    }

    if (can->config.tx_size > 0) {
        if (s_can_env->can_f.tx) {
            kfifo_free(s_can_env->can_f.tx);
        }
        s_can_env->can_f.tx = kfifo_alloc(can->config.tx_size);
        can_hal_ctrl(CMD_CAN_SET_TX_FIFO, can_tx_fifo_get);
        can_hal_ctrl(CMD_CAN_GET_TX_SIZE, can_tx_size_get);
    }

    if (can->err_cb.cb != NULL) {
        can_hal_ctrl(CMD_CAN_SET_ERR_CALLBACK, &can->err_cb);
    }

    can_callback_des_t reg_cb;

    reg_cb.cb = can_rx_cb;
    reg_cb.param = NULL;
    can_hal_ctrl(CMD_CAN_SET_RX_CALLBACK, &reg_cb);

    reg_cb.cb = can_tx_cb;
    reg_cb.param = NULL;
    can_hal_ctrl(CMD_CAN_SET_TX_CALLBACK, &reg_cb);

    bk_int_isr_register(INT_SRC_CAN, can_isr, NULL);

    return BK_OK;
}

bk_err_t bk_can_deinit(void)
{
    bk_can_base_deinit();
    bk_int_isr_unregister(INT_SRC_CAN);

    if (s_can_env != NULL) {
        rtos_deinit_semaphore(&(s_can_env->rx_semphr));
        rtos_deinit_semaphore(&(s_can_env->tx_semphr));

        if (s_can_env->can_f.rx) {
            kfifo_free(s_can_env->can_f.rx);
            s_can_env->can_f.rx = NULL;
        }

        if (s_can_env->can_f.tx) {
            kfifo_free(s_can_env->can_f.tx);
            s_can_env->can_f.tx = NULL;
        }
        os_free(s_can_env);
        s_can_env = NULL;
    }

    return BK_OK;
}

#if (CONFIG_CAN_PM_SUPPORT)
static uint32_t s_can_pm_backup[16];
static int bk_can_backup(uint64_t sleep_time_ms, void *args)
{
	CAN_RETURN_ON_DEVICE_NOT_INIT();

	s_can_pm_backup[0] = can_hal_get_fd_enable();
	s_can_pm_backup[1] = can_hal_get_tid_esi_value();
	s_can_pm_backup[2] = can_hal_get_tbuf_ctrl_value();
	s_can_pm_backup[3] = can_hal_get_cfg_value();
	s_can_pm_backup[4] = can_hal_get_ie_value();
	s_can_pm_backup[5] = can_hal_get_sseg_value();
	s_can_pm_backup[6] = can_hal_get_fseg_value();
	s_can_pm_backup[7] = can_hal_get_cap_value();
	s_can_pm_backup[8] = can_hal_get_acf_value();
	s_can_pm_backup[9] = can_hal_get_aid_value();
	s_can_pm_backup[10] = can_hal_get_ttcfg_value();
	s_can_pm_backup[11] = can_hal_get_ref_msg_value();
	s_can_pm_backup[12] = can_hal_get_trig_cfg_value();
	s_can_pm_backup[13] = can_hal_get_mem_stat_value();
	s_can_pm_backup[14] = can_hal_get_mem_es_value();
	s_can_pm_backup[15] = can_hal_get_scfg_value();

	bk_can_clock_disable();

	return BK_OK;
}

static int bk_can_restore(uint64_t sleep_time_ms, void *args)
{
	CAN_RETURN_ON_DEVICE_NOT_INIT();

	bk_can_clock_enable();
	can_hal_set_fd_enable(s_can_pm_backup[0]);
	can_hal_set_tid_esi_value(s_can_pm_backup[1]);
	can_hal_set_tbuf_ctrl_value(s_can_pm_backup[2]);
	can_hal_set_cfg_value(s_can_pm_backup[3]);
	can_hal_set_ie_value(s_can_pm_backup[4]);
	can_hal_set_sseg_value(s_can_pm_backup[5]);
	can_hal_set_fseg_value(s_can_pm_backup[6]);
	can_hal_set_cap_value(s_can_pm_backup[7]);
	can_hal_set_acf_value(s_can_pm_backup[8]);
	can_hal_set_aid_value(s_can_pm_backup[9]);
	can_hal_set_ttcfg_value(s_can_pm_backup[10]);
	can_hal_set_ref_msg_value(s_can_pm_backup[11]);
	can_hal_set_trig_cfg_value(s_can_pm_backup[12]);
	can_hal_set_mem_stat_value(s_can_pm_backup[13]);
	can_hal_set_mem_es_value(s_can_pm_backup[14]);
	can_hal_set_scfg_value(s_can_pm_backup[15]);

	return BK_OK;
}
#endif

bk_err_t bk_can_driver_init(void)
{
	if (s_can_driver_is_init) {
		return BK_OK;
	}

	s_can_dev.config.s_speed = CAN_BR_1M;
	s_can_dev.config.f_speed = CAN_BR_4M;
	s_can_dev.config.rx_size = DEFAULT_FIFO_SIZE;
	s_can_dev.config.tx_size = DEFAULT_FIFO_SIZE;
	s_can_dev.err_cb.cb = can_err_int;
	s_can_dev.err_cb.param = NULL;

	bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_AHBP_CAN, PM_POWER_MODULE_STATE_ON);
#if (CONFIG_CAN_PM_SUPPORT)
	pm_cb_conf_t enter_config = {bk_can_backup, NULL};
	pm_cb_conf_t exit_config = {bk_can_restore, NULL};
	bk_pm_sleep_register_cb(PM_MODE_LOW_VOLTAGE, PM_DEV_ID_CAN, &enter_config, &exit_config);
#endif

	BK_LOG_ON_ERR(bk_can_init(&s_can_dev));
	s_can_driver_is_init = true;

#if CONFIG_CAN_TEST
    int bk_can_register_cli_test_feature(void);
    bk_can_register_cli_test_feature();
#endif

#if CONFIG_CAN_DEMO
    int bk_can_register_cli_demo(void);
    bk_can_register_cli_demo();
#endif
	return BK_OK;
}

bk_err_t bk_can_driver_deinit(void)
{
	if (!s_can_driver_is_init) {
		return BK_OK;
	}
	s_can_driver_is_init = false;
#if (CONFIG_CAN_PM_SUPPORT)
	bk_pm_sleep_unregister_cb(PM_MODE_LOW_VOLTAGE, PM_DEV_ID_CAN, true, true);
#endif
	bk_can_deinit();
	bk_pm_module_vote_power_ctrl(PM_POWER_SUB_MODULE_NAME_AHBP_CAN, PM_POWER_MODULE_STATE_OFF);
	return BK_OK;
}