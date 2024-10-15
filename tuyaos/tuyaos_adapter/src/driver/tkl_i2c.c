#include "tkl_i2c.h"
#include <driver/i2c.h>
#include <os/mem.h>

#define I2C_SLAVE_MODE             0
#define I2C_MASTER_MODE            1

#define I2C_DIRECTION_TRANSMIT     0
#define I2C_DIRECTION_RECEIVE      1

#define I2C_BUS_CLOCK_FREQ_100K    100000
#define I2C_BUS_CLOCK_FREQ_400K    400000
#define I2C_BUS_CLOCK_FREQ_1M      1000000
#define I2C_BUS_CLOCK_FREQ_3_4M    3400000 //3.4MHz

#define I2C_WRITE_WAIT_MAX_MS      (500)
#define I2C_READ_WAIT_MAX_MS       (500)

static TUYA_IIC_STATUS_T s_i2c_status = {0};

OPERATE_RET tkl_i2c_init(TUYA_I2C_NUM_E port, const TUYA_IIC_BASE_CFG_T *cfg)
{
	i2c_config_t i2c_config = {0};

	if (cfg->role == TUYA_IIC_MODE_SLAVE) {
		s_i2c_status.mode = I2C_SLAVE_MODE;
	} else {
		s_i2c_status.mode = I2C_MASTER_MODE;
	}
	s_i2c_status.mode = cfg->role;
	switch (cfg->speed) {
	case TUYA_IIC_BUS_SPEED_100K:
		i2c_config.baud_rate = I2C_BUS_CLOCK_FREQ_100K;
		break;
	case TUYA_IIC_BUS_SPEED_400K:
		i2c_config.baud_rate = I2C_BUS_CLOCK_FREQ_400K;
		break;
	case TUYA_IIC_BUS_SPEED_1M:
		i2c_config.baud_rate = I2C_BUS_CLOCK_FREQ_1M;
		break;
	case TUYA_IIC_BUS_SPEED_3_4M:
		i2c_config.baud_rate = I2C_BUS_CLOCK_FREQ_3_4M;
		break;
	default:
		i2c_config.baud_rate = I2C_BUS_CLOCK_FREQ_100K;
		break;
	}

	if (cfg->addr_width == TUYA_IIC_ADDRESS_10BIT) {
		i2c_config.addr_mode = I2C_ADDR_MODE_10BIT;
	} else {
		i2c_config.addr_mode = I2C_ADDR_MODE_7BIT;
	}

	bk_i2c_init((i2c_id_t)port, &i2c_config);
	return OPRT_OK;
}

OPERATE_RET tkl_i2c_deinit(TUYA_I2C_NUM_E port)
{
	bk_i2c_deinit((i2c_id_t)port);
	return OPRT_OK;
}

OPERATE_RET tkl_i2c_irq_init(TUYA_I2C_NUM_E port, TUYA_I2C_IRQ_CB cb)
{
	// TODO
	return OPRT_OK;
}

OPERATE_RET tkl_i2c_irq_enable(TUYA_I2C_NUM_E port)
{
	bk_i2c_enable_interrupt((i2c_id_t)port);
	return OPRT_OK;
}

OPERATE_RET tkl_i2c_irq_disable(TUYA_I2C_NUM_E port)
{
	bk_i2c_disable_interrupt((i2c_id_t)port);
	return OPRT_OK;
}

OPERATE_RET tkl_i2c_master_send(TUYA_I2C_NUM_E port, uint16_t dev_addr, const void *data, uint32_t size, BOOL_T xfer_pending)
{
	s_i2c_status.direction = I2C_DIRECTION_TRANSMIT;
	bk_i2c_master_write((i2c_id_t)port, dev_addr, data, size, I2C_WRITE_WAIT_MAX_MS);
	return OPRT_OK;
}

OPERATE_RET tkl_i2c_master_receive(TUYA_I2C_NUM_E port, uint16_t dev_addr, void *data, uint32_t size, BOOL_T xfer_pending)
{
	s_i2c_status.direction = I2C_DIRECTION_RECEIVE;
	bk_i2c_master_write((i2c_id_t)port, dev_addr, data, size, I2C_READ_WAIT_MAX_MS);
	return OPRT_OK;
}

OPERATE_RET tkl_i2c_set_slave_addr(TUYA_I2C_NUM_E port, uint16_t dev_addr)
{
	bk_i2c_set_slave_address((i2c_id_t)port, dev_addr);
	return OPRT_OK;
}

OPERATE_RET tkl_i2c_slave_send(TUYA_I2C_NUM_E port, const void *data, uint32_t size)
{
	s_i2c_status.direction = I2C_DIRECTION_TRANSMIT;
	bk_i2c_slave_write((i2c_id_t)port, data, size, I2C_WRITE_WAIT_MAX_MS);
	return OPRT_OK;
}

OPERATE_RET tkl_i2c_slave_receive(TUYA_I2C_NUM_E port, void *data, uint32_t size)
{
	s_i2c_status.direction = I2C_DIRECTION_RECEIVE;
	bk_i2c_slave_read((i2c_id_t)port, data, size, I2C_READ_WAIT_MAX_MS);
	return OPRT_OK;
}

OPERATE_RET tkl_i2c_get_status(TUYA_I2C_NUM_E port, TUYA_IIC_STATUS_T *status)
{
    // TODO
    return OPRT_NOT_SUPPORTED;
	//s_i2c_status.busy = bk_i2c_is_bus_busy((i2c_id_t)port);
	//return &s_i2c_status;
}

OPERATE_RET tkl_i2c_reset(TUYA_I2C_NUM_E port)
{
	// TODO reset what?
	return OPRT_OK;
}
