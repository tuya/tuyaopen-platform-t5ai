#include "tkl_spi.h"
#include <driver/spi.h>
#include <sdkconfig.h>

OPERATE_RET tkl_spi_init(TUYA_SPI_NUM_E port, const TUYA_SPI_BASE_CFG_T *cfg)
{
    spi_config_t spi_config = {0};

    spi_config.wire_mode = SPI_4WIRE_MODE;
    spi_config.baud_rate = cfg->freq_hz;
#if (CONFIG_SPI_DMA)
    spi_config.dma_mode = SPI_DMA_MODE_DISABLE;
#endif
    switch (cfg->role) {
    case TUYA_SPI_ROLE_SLAVE:
    case TUYA_SPI_ROLE_SLAVE_SIMPLEX:
        spi_config.role = SPI_ROLE_SLAVE;
        break;
    case TUYA_SPI_ROLE_MASTER:
    case TUYA_SPI_ROLE_MASTER_SIMPLEX:
    default:
        spi_config.role = SPI_ROLE_MASTER;
        break;
    }

    if (cfg->databits == TUYA_SPI_DATA_BIT16) {
        spi_config.bit_width = SPI_BIT_WIDTH_16BITS;
    } else {
        spi_config.bit_width = SPI_BIT_WIDTH_8BITS;
    }

    switch (cfg->mode) {
    case TUYA_SPI_MODE0:
        spi_config.polarity = SPI_POLARITY_LOW;
        spi_config.phase = SPI_PHASE_1ST_EDGE;
        break;
    case TUYA_SPI_MODE1:
        spi_config.polarity = SPI_POLARITY_LOW;
        spi_config.phase = SPI_PHASE_2ND_EDGE;
        break;
    case TUYA_SPI_MODE2:
        spi_config.polarity = SPI_POLARITY_HIGH;
        spi_config.phase = SPI_PHASE_1ST_EDGE;
        break;
    case TUYA_SPI_MODE3:
    default:
        spi_config.polarity = SPI_POLARITY_HIGH;
        spi_config.phase = SPI_PHASE_2ND_EDGE;
        break;
    }

    if (cfg->bitorder == TUYA_SPI_ORDER_LSB2MSB) {
        spi_config.bit_order = SPI_LSB_FIRST;
    } else {
       spi_config.bit_order = SPI_MSB_FIRST;
    }

    BK_RETURN_ON_ERR(bk_spi_init((spi_id_t)port, &spi_config));
    return OPRT_OK;
}

OPERATE_RET tkl_spi_deinit(TUYA_SPI_NUM_E port)
{
    BK_RETURN_ON_ERR(bk_spi_deinit((spi_id_t)port));
    return OPRT_OK;
}

OPERATE_RET tkl_spi_send(TUYA_SPI_NUM_E port, void *data, uint16_t size)
{
    bk_spi_write_bytes((spi_id_t)port, data, size);
    return OPRT_OK;
}

OPERATE_RET tkl_spi_recv(TUYA_SPI_NUM_E port, void *data, uint16_t size)
{
    bk_spi_read_bytes((spi_id_t)port, data, size);
    return OPRT_OK;
}

OPERATE_RET tkl_spi_transfer(TUYA_SPI_NUM_E port, void* send_buf, void* receive_buf, uint32_t length)
{
    bk_spi_transmit((spi_id_t)port, send_buf, length, receive_buf, length);
    return OPRT_OK;
}

OPERATE_RET tkl_spi_abort_transfer(TUYA_SPI_NUM_E port)
{
    bk_spi_deinit((spi_id_t)port);
    return OPRT_OK;
}
