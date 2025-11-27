/*
 * test_dma.c
 * Copyright (C) 2024 cc <cc@tuya>
 *
 * Distributed under terms of the MIT license.
 */

#include "cli.h"
#include "cli_tuya_test.h"
#include "tkl_system.h"

#include <driver/dma.h>
#include <os/os.h>
#include "tuya_cloud_types.h"
static uint32_t dma_id = 0;
static void aud_dma_finish_isr(void)
{
	bk_printf("cur addr %x %x\n", dma_get_dest_write_addr(dma_id), bk_dma_get_repeat_wr_pause(dma_id));
}

void cli_dma_cmd(void)
{
    int i, j;


    dma_config_t dma_config = {0};
    uint32_t frame_size = 0;
	uint32_t *src_buff = NULL;
	uint32_t *dst_buff = NULL;

    os_memset(&dma_config, 0, sizeof(dma_config_t));

    /* malloc dma channel */
    dma_id = bk_dma_alloc(DMA_DEV_DTCM);
    if ((dma_id < DMA_ID_0) || (dma_id >= DMA_ID_MAX))
    {
        bk_printf("malloc dma fail \n");
        goto exit;
    }

    /* DMA must carry adcl and adcr data together. frame_size is one channel data size.
     * If channel number is one, need double frame_size.
     */

    frame_size = 100 * 2;

    /* init ringbuffer to save two frame data. */
    src_buff = (int8_t *)tkl_system_psram_malloc(frame_size);
    dst_buff = (int8_t *)tkl_system_psram_malloc(frame_size);
    /* init dma channel */
    dma_config.mode = DMA_WORK_MODE_REPEAT;
    dma_config.chan_prio = 1;
    dma_config.trans_type = DMA_TRANS_DEFAULT;
    dma_config.src.dev = DMA_DEV_DTCM;
    dma_config.dst.dev = DMA_DEV_DTCM;
    dma_config.src.width = DMA_DATA_WIDTH_32BITS;
    dma_config.dst.width = DMA_DATA_WIDTH_32BITS;
    /* get adc fifo address */

    dma_config.src.addr_inc_en = DMA_ADDR_INC_ENABLE;
    dma_config.src.addr_loop_en = DMA_ADDR_LOOP_ENABLE;
    dma_config.src.start_addr = src_buff;
    dma_config.src.end_addr = src_buff + frame_size;

    dma_config.trans_type = DMA_TRANS_DEFAULT;
    dma_config.dst.addr_inc_en = DMA_ADDR_INC_ENABLE;
    dma_config.dst.addr_loop_en = DMA_ADDR_LOOP_ENABLE;
    dma_config.dst.start_addr = (uint32_t)(uintptr_t)dst_buff;
    dma_config.dst.end_addr = (uint32_t)(uintptr_t)dst_buff + frame_size;
    int ret = bk_dma_init(dma_id, &dma_config);
    if (ret != BK_OK)
    {
        bk_printf("%s, %d, dma_init fail\n", __func__, __LINE__);
        goto exit;
    }
	// dma_set_dst_pause_addr(dma_id, (((uint32_t)dst_buff) + (frame_size / 2) + 1));
    /* set dma transfer length */
    bk_dma_set_transfer_len(dma_id, frame_size / 2);
    /* register dma isr */
    bk_dma_register_isr(dma_id, NULL, (void *)aud_dma_finish_isr);
    bk_dma_enable_finish_interrupt(dma_id);

#if (CONFIG_SPE)
    bk_dma_set_dest_sec_attr(dma_id, DMA_ATTR_SEC);
    bk_dma_set_src_sec_attr(dma_id, DMA_ATTR_SEC);
#endif

    bk_printf("adc_dma_cfg mic_dma_id: %d, transfer_len: %d \n", dma_id, frame_size/2);
    bk_printf("src_start_addr: 0x%08x, src_end_addr: 0x%08x \n", dma_config.src.start_addr, dma_config.src.end_addr);
    bk_printf("dst_start_addr: 0x%08x, dst_end_addr: 0x%08x \n", dma_config.dst.start_addr, dma_config.dst.end_addr);
	bk_dma_start(dma_id);
    return BK_OK;
exit:
    return BK_FAIL;
}


