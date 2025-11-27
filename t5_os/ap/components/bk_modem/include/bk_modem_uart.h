
#ifndef __BK_MODEM_UART_H_
#define __BK_MODEM_UART_H_

#define BK_MODEM_UART_ID    UART_ID_1
#define BK_MODEM_UART_BAUD   115200
#define BK_MODEM_UART_3M_BAUD 3000000
#define BK_MODEM_UART_6M_BAUD 6000000
#define BK_MODEM_UART_READ_BUFF_SIZE   2048

/// ec begin
#define MASTER_MRDY_GPIO           4 //gpio 4
#define MASTER_SRDY_GPIO            5//gpio 5
#define UART_NIC_HD_SIZE           sizeof(UART_NIC_HD_T)
#define UART_NIC_SLEEP_TIMER     15000  //15s
#define UART_NIC_RX_MTU             1600    // includes 4 bytes UART_NIC_HD_SIZE
typedef union
{
    uint8_t bytes[4];
    struct
    {
        uint32_t data_len:           12;
        uint32_t seq_no:             4; // for debug purpose
        uint32_t flow_control:      1; // flow control
        uint32_t is_tx_end:          1; // is this frame the last one?
        uint32_t type:                  2;
        uint32_t resv:                  4;
        uint32_t checksum:          8; // XOR of Frame head byte0-2
    } field;
} UART_NIC_HD_T;

/// ec endi
enum bk_modem_uart_msg_e
{
    MSG_MODEM_UART_TX,
    MSG_MODEM_UART_NIC_TX,
    MSG_MODEM_UART_RX,
    MSG_MODEM_UART_NIC_RX,
    MSG_MODEM_UART_NIC_SLEEP,
};

bk_err_t bk_modem_uart_init(uint32_t baud_rate);
bk_err_t bk_modem_uart_send_data(uint8_t *buff, uint32_t size);
bk_err_t bk_modem_uart_deinit(void);
void bk_modem_uart_data_send(uint32_t data_length, uint8_t *data, enum bk_modem_uart_trx_mode_e uart_trx_mode);
#endif