#ifndef _SARADC_IPC_H_
#define _SARADC_IPC_H_

#include <common/bk_include.h>
#include <driver/adc.h>

enum
{
	SARADC_CMD_ACQUIRE = 0,
	SARADC_CMD_INIT,
	SARADC_CMD_ENABLE_BYPASS_CALIBRATION,
	SARADC_CMD_START,
	SARADC_CMD_READ_RAW,
	SARADC_CMD_READ_RAW_DONE,
	SARADC_CMD_STOP,
	SARADC_CMD_DEINIT,
	SARADC_CMD_RELEASE,
	SARADC_CMD_SET_CONFIG,
	SARADC_CMD_GET_CONFIG,
	SARADC_CMD_REGISTER_ISR_CALLBACK,
	SARADC_CMD_UNREGISTER_ISR_CALLBACK,
	SARADC_CMD_EN,
	SARADC_CMD_READ,
	SARADC_CMD_READ_DONE,
	SARADC_CMD_SINGLE_READ,
	SARADC_CMD_SET_CHANNEL,
	SARADC_CMD_SET_MODE,
	SARADC_CMD_GET_MODE,
	SARADC_CMD_SET_CLK,
	SARADC_CMD_SET_SAMPLE_RATE,
	SARADC_CMD_SET_FILTER,
	SARADC_CMD_SET_STEADY_TIME,
	SARADC_CMD_SET_SAMPLE_CNT,
	SARADC_CMD_SET_SATURATE_MODE,
	SARADC_CMD_REGISTER_ISR,
	SARADC_CMD_DATA_CALCULATE,
	SARADC_CMD_CALCULATE,
	SARADC_CMD_INIT_GPIO,
	SARADC_CMD_DEINIT_GPIO,
};

typedef struct
{
    u32                          param;
    u16                          buff[32];
    u32                          size;
    u32                          timeout;
    int16                        ret_status;
    u8                           sample_cnt;
    u16                          adc_cali_data;
    u32                          crc;
    adc_config_t                 config;
    void                         *callback;
    void                         *context;
} saradc_cmd_t;

#define SARADC_IPC_READ_SIZE     0x400
#define SARADC_IPC_WRITE_SIZE    0x400

#endif //_SARADC_IPC_H_
// eof