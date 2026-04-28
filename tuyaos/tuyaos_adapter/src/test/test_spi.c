/*
 * test_spi.c
 * Copyright (C) 2025 cc <cc@tuya>
 *
 * Distributed under terms of the MIT license.
 */

#include "cli.h"
#include "cli_tuya_test.h"

#include "tuya_cloud_types.h"
#include "tkl_system.h"
#include "tkl_memory.h"
#include "tkl_spi.h"
#include "tkl_system.h"
#include "tkl_semaphore.h"

#define SPI_TEST_MASTER_PORT    0
#define SPI_TEST_SLAVE_PORT     1

// #define SPI_TEST_SEND_RECV_LEN   5120
#define SPI_TEST_SEND_RECV_LEN   1024

static uint8_t  *spi_data_buffer = NULL;

static TKL_SEM_HANDLE __test_spi_complete_sem = NULL;
#define TEST_SPI_RECV   0
#define TEST_SPI_SEND   1


void __spi_event_cb(TUYA_SPI_NUM_E port, TUYA_SPI_IRQ_EVT_E event)
{
    if ((event == TUYA_SPI_EVENT_TX_COMPLETE) || (event == TUYA_SPI_EVENT_RX_COMPLETE)) {
        bk_printf("post sem %d\r\n", event);
        tkl_semaphore_post(__test_spi_complete_sem);
    }
}

static int __test_spi_init(int port, TUYA_SPI_ROLE_E role, int with_dma, uint32_t len)
{
    int ret = 0;

    TUYA_SPI_BASE_CFG_T spi_cfg;

    spi_cfg.role = role;
    spi_cfg.mode = TUYA_SPI_MODE0;
    spi_cfg.type = TUYA_SPI_AUTO_TYPE;
    spi_cfg.databits = TUYA_SPI_DATA_BIT8;
    spi_cfg.bitorder = TUYA_SPI_ORDER_MSB2LSB;
    spi_cfg.freq_hz = 10000000;
    spi_cfg.spi_dma_flags = with_dma;

    ret = tkl_spi_init(port,  &spi_cfg);
    if(OPRT_OK != ret) {
        bk_printf("spi_init fail %d\r\n", __LINE__);
        return -1;
    }

    tkl_spi_irq_init(port, __spi_event_cb);
    tkl_spi_irq_enable(port);

    spi_data_buffer = (uint8_t *)tkl_system_malloc(len);
    if (spi_data_buffer == NULL) {
        bk_printf("spi_init fail %d\r\n", __LINE__);
        return -1;
    }

    memset(spi_data_buffer, 0x5a, len);

    tkl_semaphore_create_init(&__test_spi_complete_sem, 0, 1);

    bk_printf("spi test buffer: %p\r\n", spi_data_buffer);

    for (int i = 0; i < len; i ++) {
        spi_data_buffer[i] = i % 256;
    }

    return 0;
}

static int __test_spi_txrx(int port, int direct, uint32_t len)
{
    int ret = 0;
    uint32_t sum = 0;

    bk_printf("spi tx/rx test %d\r\n", __LINE__);

    if (direct == TEST_SPI_RECV) {
        ret = tkl_spi_recv(port, spi_data_buffer, len);
        if(OPRT_OK != ret) {
            bk_printf("spi_send fail %d\r\n", __LINE__);
            return -1;
        }
        tkl_semaphore_wait(__test_spi_complete_sem, TKL_SEM_WAIT_FOREVER);

        // data check
        for (int i = 0; i < len; i ++) {
            bk_printf("spi recv data:%x\r\n", spi_data_buffer[i]);
            sum += spi_data_buffer[i];
        }

    } else if (direct == TEST_SPI_SEND) {
        ret = tkl_spi_send(port, spi_data_buffer, len);
        if(OPRT_OK != ret) {
            bk_printf("spi_send fail %d\r\n", __LINE__);
            return -1;
        }
        tkl_semaphore_wait(__test_spi_complete_sem, TKL_SEM_WAIT_FOREVER);

        // data check
        for (int i = 0; i < len; i ++) {
            bk_printf("spi send data:%x\r\n", spi_data_buffer[i]);
            sum += spi_data_buffer[i];
        }
    } else {
        return -1;
    }

    bk_printf("spi master send && slave recv test complete %d\r\n", sum);

    return 0;
}

static int __test_spi_xfer(int port, uint32_t len)
{
    int ret = 0;

    uint8_t *spi_exchange_buffer = (uint8_t *)tkl_system_malloc(len);
    if (spi_exchange_buffer == NULL) {
        bk_printf("spi_init fail %d\r\n", __LINE__);
        return -1;
    }

    memset(spi_exchange_buffer, 0x11, len);
    memset(spi_data_buffer, 0x77, len);

    ret = tkl_spi_transfer(port, spi_data_buffer, spi_exchange_buffer, len);
    if(OPRT_OK != ret) {
        bk_printf("spi_send fail %d, %x\r\n", __LINE__, ret);
        return -1;
    }

    bk_printf("=================== len: %d, %x\r\n", len, spi_exchange_buffer[0]);
    for (int i = 0; i < len; i ++) {
        if (spi_exchange_buffer[i] != 0x77) {
            bk_printf("spi_send fail %d %d\r\n", __LINE__, spi_exchange_buffer[i]);
        }
        if (i < 10) {
            bk_printf("recv %x\r\n", spi_exchange_buffer[i]);
        }
    }

    tkl_system_free(spi_exchange_buffer);
    spi_exchange_buffer = NULL;

    bk_printf("spi xfer test ok \r\n");
    return 0;
}

void __test_spi_usage(void)
{
    bk_printf("Usage: xspi [0|1] [master/slave] [send/recv] [no_dma|dma] [half|full] [10000]\r\n");
}

void cli_spi_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    int ret = 0;

    uint32_t port = 0;
    TUYA_SPI_ROLE_E role = TUYA_SPI_ROLE_MASTER;
    uint8_t direct = 0, with_dma = 0, mode = 0;
    uint32_t test_len = SPI_TEST_SEND_RECV_LEN;

    bk_printf("argc: %d\r\n cmd: ", argc);
    for (int i = 0; i < argc; i++) {
        bk_printf("%s ", argv[i]);
    }
    bk_printf("\r\n");

    if (argc < 6) {
        bk_printf("error %d\r\n", __LINE__);
        __test_spi_usage();
        return;
    }

    if (!os_strcmp(argv[1], "0")) {
        port = 0;
    } else if (!os_strcmp(argv[1], "1")) {
        port = 1;
    } else {
        bk_printf("error %d, %s\r\n", __LINE__, argv[1]);
        __test_spi_usage();
        return;
    }

    if (!os_strcmp(argv[2], "master")) {
        role = TUYA_SPI_ROLE_MASTER;
    } else if (!os_strcmp(argv[2], "slave")) {
        role = TUYA_SPI_ROLE_SLAVE;
    } else {
        bk_printf("error %d, %s\r\n", __LINE__, argv[2]);
        __test_spi_usage();
        return;
    }

    if (!os_strcmp(argv[3], "send")) {
        direct = TEST_SPI_SEND;
    } else if (!os_strcmp(argv[3], "recv")) {
        direct = TEST_SPI_RECV;
    } else {
        bk_printf("error %d, %s\r\n", __LINE__, argv[3]);
        __test_spi_usage();
        return;
    }

    if (!os_strcmp(argv[4], "dma")) {
        with_dma = 1;
    } else if (!os_strcmp(argv[4], "no_dma")) {
        with_dma = 0;
    } else {
        bk_printf("error %d, %s\r\n", __LINE__, argv[4]);
        __test_spi_usage();
        return;
    }

    if (!os_strcmp(argv[5], "half")) {
        mode = 1;
    } else if (!os_strcmp(argv[5], "full")) {
        mode = 2;
    } else {
        bk_printf("error %d, %s\r\n", __LINE__, argv[5]);
        __test_spi_usage();
        return;
    }

    if (argv[6] != NULL) {
        test_len = os_strtoul(argv[6], NULL, 10);
    }

    bk_printf("spi test len: %d\r\n", test_len);

    // --- step 1, init
    ret = __test_spi_init(port, role, with_dma, test_len);
    if (ret) {
        goto __spi_error_exit;
    }

    if (mode == 1) {
        ret = __test_spi_txrx(port, direct, test_len);
    } else if (mode == 2) {
        ret = __test_spi_xfer(port, test_len);
    }

    if (ret) {
        goto __spi_error_exit;
    }

    bk_printf("spi send / recv test ok \r\n");

__spi_error_exit:
    if (NULL != spi_data_buffer) {
        tkl_system_free(spi_data_buffer);
        spi_data_buffer = NULL;
    }

    if (__test_spi_complete_sem != NULL) {
        tkl_semaphore_release(__test_spi_complete_sem);
        __test_spi_complete_sem = NULL;
    }

    bk_printf("spi%d deinit\r\n", port);
    tkl_spi_deinit(port);
    return;
}

void spi_tx_done_cb(TUYA_QSPI_NUM_E port, TUYA_QSPI_IRQ_EVT_E event)
{
    bk_printf("----------qspi_tx_done_cb:%x,%x \r\n", port, event);
}

#define BSP_LCD_H_RES              (128)
#define BSP_LCD_V_RES              (128)

#define LCD_SPI_BD_PIN      GPIO_25

#define LCD_SPI_RESET_PIN      GPIO_6
bk_err_t lcd_spi_hardware_reset(void)
{
    gpio_dev_unmap(LCD_SPI_BD_PIN);
    gpio_dev_map(LCD_SPI_BD_PIN, 0);
    bk_gpio_enable_pull(LCD_SPI_BD_PIN);
    bk_gpio_pull_up(LCD_SPI_BD_PIN);
    rtos_delay_milliseconds(20);
    bk_gpio_pull_down(LCD_SPI_BD_PIN);
    rtos_delay_milliseconds(200);
    bk_gpio_pull_up(LCD_SPI_BD_PIN);
    rtos_delay_milliseconds(120);


    gpio_dev_unmap(LCD_SPI_RESET_PIN);
    gpio_dev_map(LCD_SPI_RESET_PIN, 0);
    bk_gpio_enable_pull(LCD_SPI_RESET_PIN);
    bk_gpio_pull_up(LCD_SPI_RESET_PIN);
    rtos_delay_milliseconds(20);
    bk_gpio_pull_down(LCD_SPI_RESET_PIN);
    rtos_delay_milliseconds(200);
    bk_gpio_pull_up(LCD_SPI_RESET_PIN);
    rtos_delay_milliseconds(120);

    return BK_OK;
}


void cli_spi_2_3_cmd(char *pcWriteBuffer, int xWriteBufferLen, int argc, char **argv)
{
    int i,j = 0;
    int port = 2;
    TUYA_SPI_BASE_CFG_T cfg;

    bk_printf("argc: %d\r\n cmd: ", argc);
    for (int i = 0; i < argc; i++) {
        bk_printf("%s ", argv[i]);
    }
    bk_printf("\r\n");

    memset(&cfg, 0, sizeof(TUYA_SPI_BASE_CFG_T));
    cfg.freq_hz = 10000000;
    cfg.role = TUYA_SPI_ROLE_MASTER;
    cfg.mode = TUYA_SPI_MODE0;

    lcd_spi_hardware_reset();

    if (argc == 3) {
        if (!os_strcmp(argv[2], "2")) {
            port = 2;
        } else if (!os_strcmp(argv[2], "3")) {
            port = 3;
        }
    }
    tkl_spi_init(port, &cfg);

    uint8_t *data = (uint8_t *)psram_malloc(BSP_LCD_H_RES * BSP_LCD_V_RES * 2);
    for(i = 0; i < BSP_LCD_H_RES * BSP_LCD_V_RES; i ++) {
        data[2 * i] = 0xf8;
        data[2*i + 1] = 0x00;
    }

    // bk_printf("%x, %x, %x, %x, %x, %x, %x, %x \r\n", data[1016], data[1017],data[1018],data[1019],data[1020],data[1021],data[1022],data[1023]);
    if (!os_strcmp(argv[1], "test")) {
        tkl_spi_irq_init(port, spi_tx_done_cb);
        tkl_spi_irq_enable(port);
        tkl_spi_send(port, data, 256);
    } else if (!os_strcmp(argv[1], "irq_send")) {

        tkl_spi_irq_init(port, spi_tx_done_cb);
        tkl_spi_irq_enable(port);
        tkl_spi_send(port, data, 2);

        tkl_spi_send(port, data, BSP_LCD_H_RES * BSP_LCD_V_RES * 2);

    } else if (!os_strcmp(argv[1], "send")) {
        tkl_spi_send(port, data, 256);
    } else if (!os_strcmp(argv[1], "recv")) {
        memset(data, 0, 1024);
        tkl_spi_recv(port, data, 1024);
        for (int i = 0; i < 1024; i ++) {
            bk_printf(" %x", data[i]);
        }
    } else {
    }
    tkl_system_sleep(2000);
    tkl_spi_irq_disable(port);
    tkl_spi_deinit(port);

    psram_free(data);
    return;

}

