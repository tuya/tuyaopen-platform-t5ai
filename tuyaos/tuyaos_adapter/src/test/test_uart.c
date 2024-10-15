#include "hal_config.h"
#include "uart_hw.h"
#include "uart_hal.h"
#include "uart_ll.h"

void uart_struct_dump(uart_id_t id)
{
    uart_hw_t *hw = (uart_hw_t *)UART_LL_REG_BASE(0);
    bk_printf("base=%x\r\n", (uint32_t)hw);

    bk_printf("uart(%x)\n", id);

    if (id >= SOC_UART_ID_NUM_PER_UNIT) {
        SOC_LOGW("[%s] invalid uart id\n");
        return;
    }

    bk_printf("  config=0x%x value=0x%x\n", &hw->config, hw->config.v);
    bk_printf("    tx_enable: %x\n", hw->config.tx_enable);
    bk_printf("    rx_enable: %x\n", hw->config.rx_enable);
    bk_printf("    mode:      %x\n", hw->config.mode);
    bk_printf("    data_bits: %x\n", hw->config.data_bits);
    bk_printf("    parity_en: %x\n", hw->config.parity_en);
    bk_printf("    parity:    %x\n", hw->config.parity);
    bk_printf("    stop_bits: %x\n", hw->config.stop_bits);
    bk_printf("    clk_div:   %x\n", hw->config.clk_div);

    bk_printf("\n");
    bk_printf("  fifo_config=0x%x value=0x%x\n", &hw->fifo_config, hw->fifo_config.v);
    bk_printf("    tx_fifo_threshold:   %x\n", hw->fifo_config.tx_fifo_threshold);
    bk_printf("    rx_fifo_threshold:   %x\n", hw->fifo_config.rx_fifo_threshold);
    bk_printf("    rx_stop_detect_time: %x\n", hw->fifo_config.rx_stop_detect_time);

    bk_printf("\n");
    bk_printf("  fifo_status=0x%x value=0x%x\n", &hw->fifo_status, hw->fifo_status.v);
    bk_printf("    tx_fifo_count: %x\n", hw->fifo_status.tx_fifo_count);
    bk_printf("    rx_fifo_count: %x\n", hw->fifo_status.rx_fifo_count);
    bk_printf("    tx_fifo_full:  %x\n", hw->fifo_status.tx_fifo_full);
    bk_printf("    tx_fifo_empty: %x\n", hw->fifo_status.tx_fifo_empty);
    bk_printf("    rx_fifo_full:  %x\n", hw->fifo_status.rx_fifo_full);
    bk_printf("    rx_fifo_empty: %x\n", hw->fifo_status.rx_fifo_empty);
    bk_printf("    fifo_wr_ready: %x\n", hw->fifo_status.fifo_wr_ready);
    bk_printf("    fifo_rd_ready: %x\n", hw->fifo_status.fifo_rd_ready);

    bk_printf("\n");
    bk_printf("  fifo_port=0x%x value=0x%x\n", &hw->fifo_port, hw->fifo_port.v);
    bk_printf("    tx_fifo_data_in:  %x\n", hw->fifo_port.tx_fifo_data_in);
    bk_printf("    rx_fifo_data_out: %x\n", hw->fifo_port.rx_fifo_data_out);

    bk_printf("\n");
    bk_printf("  int_enable=0x%x value=0x%x\n", &hw->int_enable, hw->int_enable.v);
    bk_printf("    tx_fifo_need_write: %x\n", hw->int_enable.tx_fifo_need_write);
    bk_printf("    rx_fifo_need_read:  %x\n", hw->int_enable.rx_fifo_need_read);
    bk_printf("    rx_fifo_overflow:   %x\n", hw->int_enable.rx_fifo_overflow);
    bk_printf("    rx_parity_err:      %x\n", hw->int_enable.rx_parity_err);
    bk_printf("    rx_stop_bits_err:   %x\n", hw->int_enable.rx_stop_bits_err);
    bk_printf("    tx_finish:          %x\n", hw->int_enable.tx_finish);
    bk_printf("    rx_finish:          %x\n", hw->int_enable.rx_finish);
    bk_printf("    rxd_wakeup:         %x\n", hw->int_enable.rxd_wakeup);

    bk_printf("\n");
    bk_printf("  int_status=0x%x value=0x%x\n", &hw->int_status, hw->int_status.v);
    bk_printf("    tx_fifo_need_write: %x\n", hw->int_status.tx_fifo_need_write);
    bk_printf("    rx_fifo_need_read:  %x\n", hw->int_status.rx_fifo_need_read);
    bk_printf("    rx_fifo_overflow:   %x\n", hw->int_status.rx_fifo_overflow);
    bk_printf("    rx_parity_err:      %x\n", hw->int_status.rx_parity_err);
    bk_printf("    rx_stop_bits_err:   %x\n", hw->int_status.rx_stop_bits_err);
    bk_printf("    tx_finish:          %x\n", hw->int_status.tx_finish);
    bk_printf("    rx_finish:          %x\n", hw->int_status.rx_finish);
    bk_printf("    rxd_wakeup:         %x\n", hw->int_status.rxd_wakeup);

    bk_printf("\n");
    bk_printf("  flow_config=0x%x value=0x%x\n", &hw->flow_ctrl_config, hw->flow_ctrl_config.v);
    bk_printf("    flow_ctrl_low_cnt:  %x\n", hw->flow_ctrl_config.flow_ctrl_low_cnt);
    bk_printf("    flow_ctrl_high_cnt: %x\n", hw->flow_ctrl_config.flow_ctrl_high_cnt);
    bk_printf("    flow_ctrl_en:       %x\n", hw->flow_ctrl_config.flow_ctrl_en);
    bk_printf("    rts_polarity_sel:   %x\n", hw->flow_ctrl_config.rts_polarity_sel);
    bk_printf("    cts_polarity_sel:   %x\n", hw->flow_ctrl_config.cts_polarity_sel);

    bk_printf("\n");
    bk_printf("  wake_config=0x%x value=0x%x\n", &hw->wake_config, hw->wake_config.v);
    bk_printf("    wake_cnt:             %x\n", hw->wake_config.wake_cnt);
    bk_printf("    txd_wait_cnt:         %x\n", hw->wake_config.txd_wait_cnt);
    bk_printf("    rxd_wake_en:          %x\n", hw->wake_config.rxd_wake_en);
    bk_printf("    txd_wake_en:          %x\n", hw->wake_config.txd_wake_en);
    bk_printf("    rxd_neg_edge_wake_en: %x\n", hw->wake_config.rxd_neg_edge_wake_en);
}


