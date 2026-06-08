// Copyright 2020-2021 Beken
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


#include "i2s_hw.h"
#include <driver/i2s_types.h>
#include "i2s_hal.h"


bk_err_t i2s_hal_soft_reset_by_id(i2s_gpio_group_id_t id)
{
	i2s_ll_set_smb_clkrst_value(I2S_SOFT_RESET_MASK << I2S_SOFT_RESET_POS, id);
	return BK_OK;
}

bk_err_t i2s_hal_config_by_id(i2s_gpio_group_id_t id, const i2s_cfg_t *config)
{
	i2s_ll_set_pcm_cfg_i2spcmen(config->i2s_en, id);
	i2s_ll_set_pcm_cfg_msten(config->role, id);
	i2s_ll_set_pcm_cfg_modesel(config->work_mode, id);
	i2s_ll_set_pcm_cfg_lrckrp(config->lrck_invert, id);
	i2s_ll_set_pcm_cfg_sclkinv(config->sck_invert, id);
	i2s_ll_set_pcm_cfg_lsbfirst(config->lsb_first_en, id);
	i2s_ll_set_pcm_cfg_synclen(config->sync_length, id);
	i2s_ll_set_pcm_cfg_datalen(config->data_length - 1, id);

	i2s_ll_set_pcm_cfg_pcm_dlen(config->pcm_dlength, id);
	i2s_ll_set_pcm_cfg_smpratio(config->sample_ratio, id);
	i2s_ll_set_pcm_cfg_bitratio(config->sck_ratio, id);

	i2s_ll_set_pcm_cn_parallel_en(config->parallel_en, id);
	i2s_ll_set_pcm_cn_lrcom_store(config->store_mode, id);
	i2s_ll_set_pcm_cn_bitratio_h4b(config->sck_ratio_h4b, id);
	i2s_ll_set_pcm_cn_smpratio_h2b(config->sample_ratio_h2b, id);
	i2s_ll_set_pcm_cn_txint_level(config->txint_level, id);
	i2s_ll_set_pcm_cn_rxint_level(config->rxint_level, id);

	return BK_OK;
}

bk_err_t i2s_hal_deconfig_by_id(i2s_gpio_group_id_t id)
{
	i2s_ll_set_pcm_cfg_value(0, id);
	i2s_ll_set_pcm_cn_value(0, id);
	i2s_ll_set_pcm_dat_value(0, id);
	i2s_ll_set_pcm_cn_lt2_value(0, id);
	i2s_ll_clear_pcm_stat_lt2_rx2ovf(id);
	i2s_ll_clear_pcm_stat_lt2_tx2udf(id);
	i2s_ll_set_pcm_dat2_value(0, id);
	i2s_ll_set_pcm_dat3_value(0, id);
	i2s_ll_set_pcm_dat4_value(0, id);

	return BK_OK;
}

bk_err_t i2s_hal_int_status_get_by_id(i2s_gpio_group_id_t id, i2s_int_status_t *int_status)
{
	switch(int_status->channel_id)
	{
		case I2S_CHANNEL_1:
			int_status->tx_udf = (bool)i2s_ll_get_pcm_stat_txudf(id);
			int_status->rx_ovf = (bool)i2s_ll_get_pcm_stat_rxovf(id);
			int_status->tx_int = (bool)i2s_ll_get_pcm_stat_txint(id);
			int_status->rx_int = (bool)i2s_ll_get_pcm_stat_rxint(id);
			break;
		case I2S_CHANNEL_2:
			int_status->tx_udf = (bool)i2s_ll_get_pcm_stat_lt2_tx2udf(id);
			int_status->rx_ovf = (bool)i2s_ll_get_pcm_stat_lt2_rx2ovf(id);
			int_status->tx_int = (bool)i2s_ll_get_pcm_stat_lt2_tx2int(id);
			int_status->rx_int = (bool)i2s_ll_get_pcm_stat_lt2_rx2int(id);
			break;
		case I2S_CHANNEL_3:
			int_status->tx_udf = (bool)i2s_ll_get_pcm_stat_lt2_tx3udf(id);
			int_status->rx_ovf = (bool)i2s_ll_get_pcm_stat_lt2_rx3ovf(id);
			int_status->tx_int = (bool)i2s_ll_get_pcm_stat_lt2_tx3int(id);
			int_status->rx_int = (bool)i2s_ll_get_pcm_stat_lt2_rx3int(id);
			break;
		default:
			break;
	}

	return BK_OK;
}

bk_err_t i2s_hal_read_ready_get_by_id(i2s_channel_id_t channel_id, uint32_t *read_flag)
{
	*read_flag = i2s_ll_get_pcm_stat_rxfifo_rd_ready(channel_id);
	return BK_OK;
}

bk_err_t i2s_hal_write_ready_get_by_id(i2s_channel_id_t channel_id, uint32_t *write_flag)
{
	*write_flag = i2s_ll_get_pcm_stat_txfifo_wr_ready(channel_id);
	return BK_OK;
}

bk_err_t i2s_hal_en_set_by_id(i2s_gpio_group_id_t id, uint32_t value)
{
	i2s_ll_set_pcm_cfg_i2spcmen(value, id);
	return BK_OK;
}

bk_err_t i2s_hal_int_set_by_id(i2s_gpio_group_id_t id, i2s_isr_id_t int_id, uint32_t value)
{
	switch (int_id) {
		case I2S_ISR_CHL1_TXUDF:
			i2s_ll_set_pcm_cn_txudf_en(value, id);
			break;
		case I2S_ISR_CHL1_RXOVF:
			i2s_ll_set_pcm_cn_rxovf_en(value, id);
			break;

		case I2S_ISR_CHL1_TXINT:
			i2s_ll_set_pcm_cn_txint_en(value, id);
			break;
		case I2S_ISR_CHL1_RXINT:
			i2s_ll_set_pcm_cn_rxint_en(value, id);
			break;

		case I2S_ISR_CHL2_TXUDF:
			i2s_ll_set_pcm_cn_lt2_tx2udf_en(value, id);
			break;
		case I2S_ISR_CHL2_RXOVF:
			i2s_ll_set_pcm_cn_lt2_rx2ovf_en(value, id);
			break;
		case I2S_ISR_CHL2_TXINT:
			i2s_ll_set_pcm_cn_lt2_tx2int_en(value, id);
			break;
		case I2S_ISR_CHL2_RXINT:
			i2s_ll_set_pcm_cn_lt2_rx2int_en(value, id);
			break;

		case I2S_ISR_CHL3_TXUDF:
			i2s_ll_set_pcm_cn_lt2_tx3udf_en(value, id);
			break;
		case I2S_ISR_CHL3_RXOVF:
			i2s_ll_set_pcm_cn_lt2_rx3ovf_en(value, id);
			break;
		case I2S_ISR_CHL3_TXINT:
			i2s_ll_set_pcm_cn_lt2_tx3int_en(value, id);
			break;
		case I2S_ISR_CHL3_RXINT:
			i2s_ll_set_pcm_cn_lt2_rx3int_en(value, id);
			break;

		default:
			break;
	}

	return BK_OK;
}

bk_err_t i2s_hal_role_set_by_id(i2s_gpio_group_id_t id, uint32_t value)
{
	i2s_ll_set_pcm_cfg_msten(value, id);
	return BK_OK;
}

bk_err_t i2s_hal_work_mode_set_by_id(i2s_gpio_group_id_t id, uint32_t value)
{
	i2s_ll_set_pcm_cfg_modesel(value, id);
	return BK_OK;
}

bk_err_t i2s_hal_lrck_invert_set_by_id(i2s_gpio_group_id_t id, uint32_t value)
{
	i2s_ll_set_pcm_cfg_lrckrp(value, id);
	return BK_OK;
}

bk_err_t i2s_hal_sck_invert_set_by_id(i2s_gpio_group_id_t id, uint32_t value)
{
	i2s_ll_set_pcm_cfg_sclkinv(value, id);
	return BK_OK;
}

bk_err_t i2s_hal_lsb_first_set_by_id(i2s_gpio_group_id_t id, uint32_t value)
{
	i2s_ll_set_pcm_cfg_lsbfirst(value, id);
	return BK_OK;
}

bk_err_t i2s_hal_sync_len_set_by_id(i2s_gpio_group_id_t id, uint32_t value)
{
	i2s_ll_set_pcm_cfg_synclen(value, id);
	return BK_OK;
}

bk_err_t i2s_hal_data_len_set_by_id(i2s_gpio_group_id_t id, uint32_t value)
{
	i2s_ll_set_pcm_cfg_datalen(value, id);
	return BK_OK;
}

bk_err_t i2s_hal_pcm_dlen_set_by_id(i2s_gpio_group_id_t id, uint32_t value)
{
	i2s_ll_set_pcm_cfg_pcm_dlen(value, id);
	return BK_OK;
}

bk_err_t i2s_hal_store_mode_set_by_id(i2s_gpio_group_id_t id, uint32_t value)
{
	i2s_ll_set_pcm_cn_lrcom_store(value, id);
	return BK_OK;
}

bk_err_t i2s_hal_rxfifo_clear_by_id(i2s_gpio_group_id_t id)
{
	i2s_ll_set_pcm_cn_rxfifo_clr(1, id);
	return BK_OK;
}

bk_err_t i2s_hal_txfifo_clear_by_id(i2s_gpio_group_id_t id)
{
	i2s_ll_set_pcm_cn_txfifo_clr(1, id);
	return BK_OK;
}

bk_err_t i2s_hal_txudf_int_clear_by_id(i2s_gpio_group_id_t id, i2s_channel_id_t channel_id)
{
	switch (channel_id) {
		case I2S_CHANNEL_1:
			i2s_ll_clear_pcm_stat_txudf(id);
			break;
		case I2S_CHANNEL_2:
			i2s_ll_clear_pcm_stat_lt2_tx2udf(id);
			break;
		case I2S_CHANNEL_3:
			i2s_ll_clear_pcm_stat_lt2_tx3udf(id);
			break;
		default:
			break;
	}

	return BK_OK;
}

bk_err_t i2s_hal_rxovf_int_clear_by_id(i2s_gpio_group_id_t id, i2s_channel_id_t channel_id)
{
	switch (channel_id) {
		case I2S_CHANNEL_1:
			i2s_ll_clear_pcm_stat_rxovf(id);
			break;
		case I2S_CHANNEL_2:
			i2s_ll_clear_pcm_stat_lt2_rx2ovf(id);
			break;
		case I2S_CHANNEL_3:
			i2s_ll_clear_pcm_stat_lt2_rx3ovf(id);
			break;
		default:
			break;
	}

	return BK_OK;
}

bk_err_t i2s_hal_txint_level_set_by_id(i2s_gpio_group_id_t id, uint32_t value)
{
	i2s_ll_set_pcm_cn_txint_level(value, id);
	return BK_OK;
}

bk_err_t i2s_hal_rxint_level_set_by_id(i2s_gpio_group_id_t id, uint32_t value)
{
	i2s_ll_set_pcm_cn_rxint_level(value, id);
	return BK_OK;
}

bk_err_t i2s_hal_data_write_by_id(i2s_gpio_group_id_t id, uint32_t channel_id, uint32_t value)
{
	switch (channel_id) {
		case I2S_CHANNEL_1:
			i2s_ll_set_pcm_dat_value(value, id);
			break;
		case I2S_CHANNEL_2:
			i2s_ll_set_pcm_dat2_value(value, id);
			break;
		case I2S_CHANNEL_3:
			i2s_ll_set_pcm_dat3_value(value, id);
			break;
		default:
			break;
	}

	return BK_OK;
}

bk_err_t i2s_hal_data_read_by_id(i2s_gpio_group_id_t id, uint32_t *value)
{
	*value = i2s_ll_get_pcm_dat_i2s_dat(id);
	return BK_OK;
}

bk_err_t i2s_hal_sample_ratio_set_by_id(i2s_gpio_group_id_t id, uint32_t value)
{
	i2s_ll_set_pcm_cfg_smpratio(value, id);
	return BK_OK;
}

bk_err_t i2s_hal_sck_ratio_set_by_id(i2s_gpio_group_id_t id, uint32_t value)
{
	i2s_ll_set_pcm_cfg_bitratio(value, id);
	return BK_OK;
}

bk_err_t i2s_hal_sample_ratio_h2b_set_by_id(i2s_gpio_group_id_t id, uint32_t value)
{
	i2s_ll_set_pcm_cn_smpratio_h2b(value, id);
	return BK_OK;
}

bk_err_t i2s_hal_sck_ratio_h4b_set_by_id(i2s_gpio_group_id_t id, uint32_t value)
{
	i2s_ll_set_pcm_cn_bitratio_h4b(value, id);
	return BK_OK;
}

