#ifndef DMA_H
#define DMA_H
#include "tuya_cloud_types.h"
#include "dma_hal.h"
#include "dma_ll.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef enum {
    BUS_ERROR_ISR,
    FINISH_ISR
} DMA_ISR_TYPE_E;

typedef enum {
    TKL_DMA_ADDR_INC_DISABLE = 0, /**< DMA disable addrress increase */
    TKL_DMA_ADDR_INC_ENABLE,      /**< DMA enable addrress increase */
} TKL_DMA_INC_EN_T;

typedef enum {
    TKL_DMA_ADDR_LOOP_DISABLE = 0, /**< DMA disable addrress loop */
    TKL_DMA_ADDR_LOOP_ENABLE,      /**< DMA enable addrress loop */
} TKL_DMA_LOOP_EN_T;

typedef enum {
    TKL_DMA_DATA_WIDTH_8BITS = 0, /**< DMA data width 8bit */
    TKL_DMA_DATA_WIDTH_16BITS,    /**< DMA data width 16bit */
    TKL_DMA_DATA_WIDTH_32BITS,    /**< DMA data width 32bit */
} TKL_DATA_WIDTH_T;

typedef struct {
    UINT8_T dev;                /**< DMA device */
    TKL_DATA_WIDTH_T width;       /**< DMA data width */
    TKL_DMA_INC_EN_T addr_inc_en;   /**< enable/disable DMA address increase */
    TKL_DMA_LOOP_EN_T addr_loop_en; /**< enable/disable DMA address loop */
    UINT32_T start_addr;          /**< DMA start address */
    UINT32_T end_addr;            /**< DMA end address */
    UINT32_T brust_len; 
} TKL_DMA_PORT_T;

typedef enum {
    TKL_DMA_WORK_MODE_SINGLE = 0, /**< DMA work mode single_mode */
    TKL_DMA_WORK_MODE_REPEAT,     /**< DMA work mode repeat_mode (forever repeat until software clear dma_en) */
} TKL_WORK_MODE_E;

typedef struct {
    TKL_WORK_MODE_E mode;          /**< DMA work mode */
    TKL_DMA_PORT_T src;         /**< DMA source configuration */
    TKL_DMA_PORT_T dst;         /**< DMA dest configuration */
    UINT32_T dest_wr_intlv; /**< DMA Destination Write operate interval.unit is cycle */
    UINT32_T src_rd_intlv; /**< DMA Source Read operate interval.unit is cycle */
    UINT32_T dev_id; /**< DMA Source Read operate interval.unit is cycle */
} TKL_DMA_CONFIG_T;

typedef void (*DMA_ISR_T)(dma_id_t dma_id, DMA_ISR_TYPE_E type);

/**
 * @brief     Init the DMA channel
 *
 * @attention 1. the higher channel priority value, the higher the priority
 *
 * @param id DMA channel
 * @param config DMA configuration
 *
 * @return
 *    - OPRT_OK: succeed
 *    - BK_ERR_DMA_NOT_INIT: DMA driver not init
 *    - BK_ERR_NULL_PARAM: config is NULL
 *    - BK_ERR_DMA_ID: invalid DMA channel
 *    - BK_ERR_DMA_INVALID_ADDR: invalid DMA address
 *    - others: other errors.
 */
OPERATE_RET tkl_dma_init(dma_id_t *id, const TKL_DMA_CONFIG_T *config);

/**
 * @brief     Deinit a DMA channel
 *
 * This API deinit the DMA channel:
 *   - Stop the DMA channel
 *   - Reset all configuration of DMA channel to default value
 *
 * @return
 *    - OPRT_OK: succeed
 *    - others: other errors.
 */
OPERATE_RET tkl_dma_deinit(dma_id_t id);

/**
 * @brief     Start a DMA channel
 *
 * @param id DMA channel
 *
 * @return
 *    - OPRT_OK: succeed
 *    - others: other errors.
 */
OPERATE_RET tkl_dma_start(dma_id_t id);

/**
 * @brief     Stop a DMA channel
 *
 * @param id DMA channel
 *
 * @return
 *    - OPRT_OK: succeed
 *    - others: other errors.
 */
OPERATE_RET tkl_dma_stop(dma_id_t id);

/**
 * @brief     Transfer data from memory to peripheral
 *
 * @param id DMA channel
 * @param data DMA transfer data
 * @param size data size
 *
 * @return
 *    - OPRT_OK: succeed
 *    - others: other errors.
 */
OPERATE_RET tkl_dma_write(dma_id_t id, const uint8_t *data, UINT32_T size);

/**
 * @brief     Transfer data from peripheral to memory
 *
 * @param id DMA channel
 * @param data DMA transfer data
 * @param size data size
 *
 * @return
 *    - OPRT_OK: succeed
 *    - others: other errors.
 */
OPERATE_RET tkl_dma_read(dma_id_t id, uint8_t *data, UINT32_T size);

/**
 * @brief     Register the interrupt service routine for DMA channel
 *
 * This API regist dma isr callback function.
 *
 * @param id DMA channel
 * @param finish_isr DMA finish callback
 *
 * @return
 *    - OPRT_OK: succeed
 *    - others: other errors.
 */
OPERATE_RET tkl_dma_register_isr(dma_id_t id, DMA_ISR_T finish_isr);

/**
 * @brief     get DMA transfer max length in one round, default value is 65536 bytes.
 *
 * @param id DMA channel
 *
 * @return
 *    - max len: how many bytes can be copy by DMA in one round.
 */
UINT32_T tkl_dma_get_once_transfer_len_max(dma_id_t id);

/**
 * @brief     Set DMA transfer length
 *
 * @param id DMA channel
 * @param tran_len DMA transfer length, the tran_len should be <= tkl_dma_get_transfer_len_max() bytes.
 *
 * @return
 *    - OPRT_OK: succeed
 *    - others: other errors.
 */
OPERATE_RET tkl_dma_set_transfer_len(dma_id_t id, UINT32_T tran_len);

#ifdef DMA_REG_SUPPORT
/**
 * @brief     Set DMA source address
 *
 * @attention 1. address should be zero when there is no value, e.g. tkl_dma_set_src_loop_addr(1, 0x80210C, 0)
 *
 * @param id DMA channel
 * @param start_addr DMA source start address
 * @param end_addr DMA source end address
 *
 * @return
 *    - OPRT_OK: succeed
 *    - others: other errors.
 */
OPERATE_RET tkl_dma_set_src_loop_addr(dma_id_t id, UINT32_T start_addr, UINT32_T end_addr);

/**
 * @brief     Set DMA source start address
 *
 * @param id DMA channel
 * @param start_addr DMA source start address
 *
 * @return
 *    - OPRT_OK: succeed
 *    - others: other errors.
 */
OPERATE_RET tkl_dma_set_src_start_addr(dma_id_t id, UINT32_T start_addr);

/**
 * @brief     Set DMA dest address
 *
 * @attention 1. address should be zero when there is no value, e.g. tkl_dma_set_dest_loop_addr(1, 0x80210C, 0)
 *
 * @param id DMA channel
 * @param start_addr DMA dest start address
 * @param end_addr DMA dest end address
 *
 * @return
 *    - OPRT_OK: succeed
 *    - others: other errors.
 */
OPERATE_RET tkl_dma_set_dest_loop_addr(dma_id_t id, UINT32_T start_addr, UINT32_T end_addr);

/**
 * @brief     Set DMA dest start address
 *
 * @param id DMA channel
 * @param start_addr DMA dest start address
 *
 * @return
 *    - OPRT_OK: succeed
 *    - others: other errors.
 */
OPERATE_RET tkl_dma_set_dest_start_addr(dma_id_t id, UINT32_T start_addr);

/**
 * @brief     Enable DMA source address increase
 *
 * @param id DMA channel
 *
 * @return
 *    - OPRT_OK: succeed
 *    - others: other errors.
 */
OPERATE_RET tkl_dma_enable_src_addr_increase(dma_id_t id);

/**
 * @brief     Disable DMA source address increase
 *
 * @param id DMA channel
 *
 * @return
 *    - OPRT_OK: succeed
 *    - others: other errors.
 */
OPERATE_RET tkl_dma_disable_src_addr_increase(dma_id_t id);

/**
 * @brief     Enable DMA source address loop
 *
 * @param id DMA channel
 *
 * @return
 *    - OPRT_OK: succeed
 *    - others: other errors.
 */
OPERATE_RET tkl_dma_enable_src_addr_loop(dma_id_t id);

/**
 * @brief     Disable DMA source address loop
 *
 * @param id DMA channel
 *
 * @return
 *    - OPRT_OK: succeed
 *    - others: other errors.
 */
OPERATE_RET tkl_dma_disable_src_addr_loop(dma_id_t id);

/**
 * @brief     Enable DMA dest address increase
 *
 * @param id DMA channel
 *
 * @return
 *    - OPRT_OK: succeed
 *    - others: other errors.
 */
OPERATE_RET tkl_dma_enable_dest_addr_increase(dma_id_t id);

/**
 * @brief     Disable DMA dest address increase
 *
 * @param id DMA channel
 *
 * @return
 *    - OPRT_OK: succeed
 *    - others: other errors.
 */
OPERATE_RET tkl_dma_disable_dest_addr_increase(dma_id_t id);

/**
 * @brief     Enable DMA dest address loop
 *
 * @param id DMA channel
 *
 * @return
 *    - OPRT_OK: succeed
 *    - others: other errors.
 */
OPERATE_RET tkl_dma_enable_dest_addr_loop(dma_id_t id);

/**
 * @brief     Disable DMA dest address loop
 *
 * @param id DMA channel
 *
 * @return
 *    - OPRT_OK: succeed
 *    - others: other errors.
 */
OPERATE_RET tkl_dma_disable_dest_addr_loop(dma_id_t id);

/**
 * @brief     Get DMA transfer remain length
 *
 * @param id DMA channel
 *
 * @return DMA transfer remain length
 */
UINT32_T tkl_dma_get_remain_len(dma_id_t id);

/**
 * @brief     Gets the current DMA channel working status
 *
 * @param id DMA channel
 *
 * @return
 *    - 0: Channel idle state
 *    - others: Channel busy state.
 */
UINT32_T tkl_dma_get_enable_status(dma_id_t id);

/**
 * @brief    flush reserved data in dma internal buffer
 *           I.E:If source data width is not 4bytes, and data size isn't 4bytes align,
 *           but dest data width is 4bytes, then maybe 1~3 bytes data reserved in
 *           dma internal buffer, then the left 1~3 bytes data not copy to dest address.
 *
 * @param id DMA channel
 * @param attr DMA privileged attr
 *
 * @return
 *    - 0: Channel idle state
 *    - others: Channel busy state.
 */
OPERATE_RET tkl_dma_flush_src_buffer(dma_id_t id);

/**
 * @brief     Set DMA source pause address
 *
 * @param id DMA channel
 * @param addr DMA source pause address
 *
 * @return
 *    - OPRT_OK: succeed
 *    - others: other errors.
 */
OPERATE_RET tkl_dma_set_src_pause_addr(dma_id_t id, UINT32_T addr);

/**
 * @brief     Set DMA destination pause address
 *
 * @param id DMA channel
 * @param addr DMA destination pause address
 *
 * @return
 *    - OPRT_OK: succeed
 *    - others: other errors.
 */
OPERATE_RET tkl_dma_set_dst_pause_addr(dma_id_t id, UINT32_T addr);

/**
 * @brief     Get DMA source pause address
 *
 * @param id DMA channel
 *
 * @return DMA source pause address
 */
UINT32_T tkl_dma_get_src_pause_addr(dma_id_t id);

/**
 * @brief     Get DMA destination pause address
 *
 * @param id DMA channel
 *
 * @return DMA destination pause address
 */
UINT32_T tkl_dma_get_dst_pause_addr(dma_id_t id);

/**
 * @brief     Get DMA destination write address
 *
 * @param id DMA channel
 *
 * @return DMA destination write address
 */
UINT32_T tkl_dma_get_dest_write_addr(dma_id_t id);

/**
 * @brief     Get DMA destination end address
 *
 * @param id DMA channel
 *
 * @return DMA destination end address
 */
UINT32_T tkl_dma_get_dst_end_addr(dma_id_t id);

/**
 * @brief     Get DMA repeat read pause value
 *
 * @param id DMA channel
 *
 * @return DMA repeat read pause value
 */
UINT32_T tkl_dma_get_repeat_rd_pause(dma_id_t id);

/**
 * @brief     Get DMA repeat write pause value
 *
 * @param id DMA channel
 *
 * @return DMA repeat write pause value
 */
UINT32_T tkl_dma_get_repeat_wr_pause(dma_id_t id);
#ifdef CONFIG_SPE
/**
 * @brief    To configure the beat length,
 *           DMA actually applies for the bus one at a time and divides the total
 *           amount of data to be transmitted into small data blocks. For example, if
 *           you want to transfer 64 bytes, then dma may be divided into 2 times internally.
 *           64/2=32 bytes are transferred at one time.This 2(a) times is called burst.
 *
 * @param id DMA channel
 * @param len DMA dest burst len
 *
 * @return
 *    - OPRT_OK: succeed
 *    - others: other errors.
 */
OPERATE_RET tkl_dma_set_dest_burst_len(dma_id_t id, dma_burst_len_t len);

/**
 * @brief    Get the configured length of burst
 *
 * @param id DMA channel
 *
 * @return DMA dest burst length
 */
UINT32_T tkl_dma_get_dest_burst_len(dma_id_t id);

/**
 * @brief    To configure the beat length,
 *           DMA actually applies for the bus one at a time and divides the total
 *           amount of data to be transmitted into small data blocks. For example, if
 *           you want to transfer 64 bytes, then dma may be divided into 2 times internally.
 *           64/2=32 bytes are transferred at one time.This 2(a) times is called burst.
 *
 * @param id DMA channel
 * @param len DMA dest burst len
 *
 * @return
 *    - OPRT_OK: succeed
 *    - others: other errors.
 */
OPERATE_RET tkl_dma_set_src_burst_len(dma_id_t id, dma_burst_len_t len);

/**
 * @brief    Get the configured length of burst
 *
 * @param id DMA channel
 *
 * @return DMA dest burst length
 */
UINT32_T tkl_dma_get_src_burst_len(dma_id_t id);

/**
 * @brief    Select the conversion mode that needs to be configured for DMA conversion of
 *           video formats during data transfer.
 *
 * @param id DMA channel
 * @param type DMA Select conversion mode
 *
 * @return
 *    - OPRT_OK: succeed
 *    - others: other errors.
 */
OPERATE_RET tkl_dma_set_pixel_trans_type(dma_id_t id, dma_pixel_trans_type_t type);

/**
 * @brief    Get the conversion mode
 *
 * @param id DMA channel
 *
 * @return DMA conversion mode
 */
UINT32_T tkl_dma_get_pixel_trans_type(dma_id_t id);

/**
 * @brief     Set the current DMA channel dest secure attr
 *
 * @param id DMA channel
 * @param attr DMA secure attr
 *
 * @return
 *    - 0: Channel idle state
 *    - others: Channel busy state.
 */
OPERATE_RET tkl_dma_set_dest_sec_attr(dma_id_t id, dma_sec_attr_t attr);

/**
 * @brief     Set the current DMA channel src secure attr
 *
 * @param id DMA channel
 * @param attr DMA secure attr
 *
 * @return
 *    - 0: Channel idle state
 *    - others: Channel busy state.
 */
OPERATE_RET tkl_dma_set_src_sec_attr(dma_id_t id, dma_sec_attr_t attr);
#endif

#if (CONFIG_SPE)
/**
 * @brief     Set the all DMA channel secure attr
 *
 * @param id DMA channel
 * @param attr DMA secure attr
 *
 * @return
 *    - 0: Channel idle state
 *    - others: Channel busy state.
 */
OPERATE_RET tkl_dma_set_sec_attr(dma_id_t id, dma_sec_attr_t attr);

/**
 * @brief     Set the all DMA channel privileded attr
 *
 * @param id DMA channel
 * @param attr DMA privileged attr
 *
 * @return
 *    - 0: Channel idle state
 *    - others: Channel busy state.
 */
OPERATE_RET tkl_dma_set_privileged_attr(dma_id_t id, dma_sec_attr_t attr);

/**
 * @brief     Configure the same DMA channel to trigger the interrupt state on a fixed core
 *
 * @param id DMA channel
 * @param DMA interrupt id
 *
 * @return
 *    - 0: Channel idle state
 *    - others: Channel busy state.
 */
OPERATE_RET tkl_dma_set_int_allocate(dma_id_t id,dma_int_id_t int_id);
#endif
#endif

#ifdef __cplusplus
}
#endif

#endif