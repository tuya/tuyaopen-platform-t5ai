#include "tkl_adc.h"
#include "tkl_memory.h"
#include "tuya_error_code.h"
#include "tkl_system.h"

// #include "gpio_pub.h"
// #include "saradc_pub.h"
// #include "BkDriverGpio.h"
#include "FreeRTOS.h"
#include "task.h"
#include "driver/adc.h"
#include "common/bk_err.h"
//#include "middleware/driver/sys_ctrl/sys_driver.h"
#include "sys_driver.h"
#include "gpio_driver.h"

/*============================ MACROS ========================================*/
#define ADC_DEV_NUM         1
#define ADC_DEV_CHANNEL_SUM 16
#define ADC_BUF_SIZE 32

#define ADC_REF_VALUE 3300
#define SET_CONFIG_DIV  200

static WORD_T adc_buf[ADC_BUF_SIZE];

static UINT8_T g_adc_init[ADC_DEV_CHANNEL_SUM] = {0};
static UINT8_T g_channel_id_div[ADC_MAX] = {
    0, 0, 0, 0, 1, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0
};   // 0: no div, 1: div, chan4 default div
static UINT8_T g_adc_current_ch_num = 0;
static adc_config_t g_config[ADC_DEV_CHANNEL_SUM] = {0};
static TUYA_ADC_NUM_E adc[ADC_DEV_NUM] = {TUYA_ADC_NUM_MAX};

extern void bk_printf(const char *fmt, ...);

/**
 * @brief tuya kernel platform_adc_init,must be called before adc function enable
 *
 * @param[in] null
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */

int platform_adc_init(void)
{
    int  ret;

    ret = bk_adc_driver_init();
    if(BK_OK != ret) {
        ret = OPRT_COM_ERROR;
    } else {
        ret = OPRT_OK;
    }

    return ret;
}

gpio_id_t tkl_adc_to_pin_func(adc_chan_t adc)
{
    gpio_id_t gpio_pin = GPIO_25;

    switch (adc) {
        case ADC_1:  gpio_pin = GPIO_25; break;
        case ADC_2:  gpio_pin = GPIO_24; break;
        case ADC_3:  gpio_pin = GPIO_23; break;
        case ADC_4:  gpio_pin = GPIO_28; break;
        case ADC_5:  gpio_pin = GPIO_22; break;
        case ADC_6:  gpio_pin = GPIO_21; break;
        case ADC_10: gpio_pin = GPIO_8;  break;
        case ADC_12: gpio_pin = GPIO_0;  break;
        case ADC_13: gpio_pin = GPIO_1;  break;
        case ADC_14: gpio_pin = GPIO_12; break;
        case ADC_15: gpio_pin = GPIO_13; break;
        default:
            break;
    }
    // bk_printf("tkl_adc_to_pin_func adc:%d,%d\r\n",adc,gpio_pin);
    return gpio_pin;
}

adc_mode_t ty_to_bk_adc(TUYA_ADC_MODE_E adc_mode)
{
    adc_mode_t bk_adc_mode = ADC_CONTINUOUS_MODE;
    switch(adc_mode) {
        case TUYA_ADC_SINGLE:
            bk_adc_mode = ADC_SINGLE_STEP_MODE;
            break;
        case TUYA_ADC_CONTINUOUS:
        case TUYA_ADC_SCAN:
            bk_adc_mode = ADC_CONTINUOUS_MODE;
            break;

        default:
            break;
    }

    return bk_adc_mode;
}


/**
 * @brief tuya kernel adc init
 *
 * @param[in] unit_num: adc unit number
 * @param[in] cfg: adc config
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_adc_init(TUYA_ADC_NUM_E unit_num, TUYA_ADC_BASE_CFG_T *cfg)
{
    UINT8_T cnt = 0;
    static uint8_t is_init = 0;

    if ((unit_num >= ADC_DEV_NUM) || (cfg->ch_nums > ADC_DEV_CHANNEL_SUM)) {
        return OPRT_INVALID_PARM;
    }

    adc[unit_num] = unit_num;

    //TODO: cfg->ch_list

    for(int i = 0; i < 16; i++) {
        if(cfg->ch_list.data & (1 << i)) {
            //bk_printf("cfg->ch_list.data:%d, %d\r\n",cfg->ch_list.data, cfg->ch_list.data & (1 << i));
            g_config[cnt].chan = i;
            g_config[cnt].adc_mode = ty_to_bk_adc(cfg->mode);
            g_config[cnt].src_clk = ADC_SCLK_XTAL_26M;
            g_config[cnt].clk = 0x30A0C5;
            g_config[cnt].saturate_mode = ADC_SATURATE_MODE_3;
            g_config[cnt].steady_ctrl = 7;
            g_config[cnt].adc_filter = 0;
            g_config[cnt].sample_rate = 0x20;
            if(g_config[cnt].adc_mode == ADC_CONTINUOUS_MODE) {
                g_config[cnt].sample_rate = 0;
            }

            if(g_channel_id_div[i] == 1) {
                g_config[cnt].vol_div = ADC_VOL_DIV_1;
            }

            g_config[cnt].is_open = FALSE;
            g_config[cnt].output_buf = &adc_buf[0];
            g_config[cnt].output_buf_len = cfg->conv_cnt;
            g_adc_init[cnt] = TRUE;
            cnt++;
        }
    }

    g_adc_current_ch_num = cfg->ch_nums;

    if (!is_init) {
        BK_LOG_ON_ERR(bk_saradc_driver_init());
        is_init = 1;
    }

    return OPRT_OK;
}

/**
 * @brief adc deinit
 *
 * @param[in] unit_num: adc unit number

 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_adc_deinit(TUYA_ADC_NUM_E unit_num)
{
    for(int i = 0; i < g_adc_current_ch_num; i++) {
        g_adc_init[i] = FALSE;
    }

    memset(g_channel_id_div, 0, sizeof(g_channel_id_div));
    g_channel_id_div[ADC_4] = 1; // adc4 default div

    adc[unit_num] = TUYA_ADC_NUM_MAX;

    return OPRT_OK;
}

/**
 * @brief get adc width
 *
 * @param[in] unit_num: adc unit number

 *
 * @return adc width
 */
UINT8_T tkl_adc_width_get(TUYA_ADC_NUM_E unit_num)
{
    return 12;
}


/**
 * @brief get adc reference voltage
 *
 * @param[in] NULL

 *
 * @return adc reference voltage(bat: mv)
 */
UINT32_T tkl_adc_ref_voltage_get(TUYA_ADC_NUM_E port_num)
{
    return ADC_REF_VALUE;

}

/**
 * @brief adc read
 *
 * @param[in] unit_num: adc unit number
 * @param[out] buff: points to the list of data read from the ADC register
 * @param[out] len:  buff len
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 */
OPERATE_RET tkl_adc_read_data(TUYA_ADC_NUM_E unit_num, INT32_T *buff, UINT16_T len)
{
    OPERATE_RET ret = OPRT_OK;
    UINT16_T buff_len = 0;
    unsigned char i = 0;

    if (unit_num > ADC_DEV_NUM-1 && adc[unit_num] != unit_num) {
        bk_printf("error port num: %d:%d\r\n", unit_num, __LINE__);
        return OPRT_INVALID_PARM;
    }

    for(int i = 0; i < g_adc_current_ch_num; i++) {
        if(g_adc_init[i] == TRUE) {
            buff_len += g_config[i].output_buf_len;
        }
    }

    if (buff_len > len) {
        bk_printf("param len err:%d !!!\r\n",len);
        ret = OPRT_COM_ERROR;
    }

    buff_len = 0;
    for (i = 0; i < g_adc_current_ch_num; i++) {
        if (g_adc_init[i]) {
            ret = tkl_adc_read_single_channel(unit_num, g_config[i].chan, &buff[buff_len]);
            buff_len += g_config[i].output_buf_len;
        }
    }

    return ret;
}

OPERATE_RET tkl_adc_read_single_channel(TUYA_ADC_NUM_E unit_num, UINT8_T ch_id, INT32_T *data)
{
    int curr_ch_index =  0;
    int time_out = BEKEN_WAIT_FOREVER;
    if(adc[unit_num] != unit_num && unit_num > ADC_DEV_NUM-1) {
        bk_printf("error port num: %d:%d\r\n", unit_num, __LINE__);
        return OPRT_INVALID_PARM;
    }
    BK_LOG_ON_ERR(bk_adc_acquire());
    sys_drv_set_ana_pwd_gadc_buf(1);

    for(int i = 0; i < g_adc_current_ch_num; i++) {
        if(ch_id == g_config[i].chan) {
            curr_ch_index = i;
            break;
        }
    }

    if(g_adc_init[curr_ch_index] == TRUE) {
        gpio_dev_unmap(tkl_adc_to_pin_func(g_config[curr_ch_index].chan));
        BK_LOG_ON_ERR(bk_adc_init(g_config[curr_ch_index].chan));
        BK_LOG_ON_ERR(bk_adc_set_config(&g_config[curr_ch_index]));
        BK_LOG_ON_ERR(bk_adc_enable_bypass_clalibration());
        BK_LOG_ON_ERR(bk_adc_start());
        bk_adc_set_channel(g_config[curr_ch_index].chan);
        bk_adc_read_raw(g_config[curr_ch_index].output_buf, g_config[curr_ch_index].output_buf_len, time_out);

        for (int i = 0; i < g_config[curr_ch_index].output_buf_len; i++) {
            data[i] = (INT32_T)g_config[curr_ch_index].output_buf[i];
            //bk_printf("%d ", g_config[curr_ch_index].output_buf[i]);
        }
        //bk_printf("\r\n");
        bk_adc_stop();
        bk_adc_deinit(g_config[curr_ch_index].chan);
    }

    sys_drv_set_ana_pwd_gadc_buf(0);
    bk_adc_release();

    return 0;
}


/**
 * @brief adc get temperature
 *
 * @return temperature(bat: 'C)
 */
INT32_T tkl_adc_temperature_get(VOID_T)
{
    return OPRT_NOT_SUPPORTED;
}

/**
 * @brief read voltage
 *
 * @param[in] port_num: adc port number
 * @param[out] data: convert voltage, voltage range to -vref - +vref
 * @param[in] len: channel number
 *
 * @return OPRT_OK on success. Others on error, please refer to tuya_error_code.h
 *
 */
extern UINT16 bk_adc_data_calculate(uint16_t adc_val, uint8_t adc_chan);
OPERATE_RET tkl_adc_read_voltage(TUYA_ADC_NUM_E port_num, INT32_T *buff, UINT16_T len)
{
    UINT16_T value   = 0;
    float cali_value = 0;
    UINT8_T read_cnt = 0;
    UINT16_T buff_len = 0;

    if(adc[port_num] != port_num && port_num > ADC_DEV_NUM-1) {
        bk_printf("error port num: %d:%d\r\n", port_num, __LINE__);
        return OPRT_INVALID_PARM;
    }

    for(int i = 0; i < g_adc_current_ch_num; i++) {
        if(g_adc_init[i] == TRUE) {
            buff_len += g_config[i].output_buf_len;
        }
    }

    if(buff_len > len) {
        bk_printf("param len err !!!\r\n");
        return OPRT_INVALID_PARM;
    }

    BK_LOG_ON_ERR(bk_adc_acquire());
    sys_drv_set_ana_pwd_gadc_buf(1);

    for(int i = 0; i < g_adc_current_ch_num; i++) {
        if(g_adc_init[i] == TRUE) {
            gpio_dev_unmap(tkl_adc_to_pin_func(g_config[i].chan));
            BK_LOG_ON_ERR(bk_adc_init(g_config[i].chan));
#if 0
            bk_printf("adc_mode:%d\r\n", g_config[i].adc_mode);
            bk_printf("src_clk:%d\r\n", g_config[i].src_clk);
            bk_printf("clk:%x\r\n", g_config[i].clk);
            bk_printf("saturate_mode:%d\r\n", g_config[i].saturate_mode);
            bk_printf("steady_ctrl:%d\r\n", g_config[i].steady_ctrl);
            bk_printf("adc_filter:%d\r\n", g_config[i].adc_filter);
            bk_printf("sample_rate:%d\r\n", g_config[i].sample_rate);
#endif
            BK_LOG_ON_ERR(bk_adc_set_config(&g_config[i]));
            BK_LOG_ON_ERR(bk_adc_enable_bypass_clalibration());
            BK_LOG_ON_ERR(bk_adc_start());
            for(int j = 0; j < g_config[i].output_buf_len; j++) {
                BK_LOG_ON_ERR(bk_adc_read(&value, 1000));

                extern UINT16 bk_adc_data_calculate(UINT16 adc_val, UINT8 adc_chan);
                cali_value = bk_adc_data_calculate(value, g_config[i].chan);

                buff[read_cnt] = (INT32_T) (cali_value * 1000);
                //bk_printf("buff[%d]:%dmv, %f\r\n", read_cnt, buff[read_cnt], cali_value);
                read_cnt++;
            }
            bk_adc_stop();
            bk_adc_deinit(g_config[i].chan);
        }
    }
    sys_drv_set_ana_pwd_gadc_buf(0);
    bk_adc_release();

    return 0;
}

OPERATE_RET tkl_adc_ioctl(ADC_IOCTL_CMD_E cmd,  VOID *args)
{
    UINT8_T ch_id = ADC_MAX;

    if(NULL != args) {
        ch_id = (UINT8_T)*(UINT8_T *)args;
    }

    if(ch_id >= ADC_MAX) {
        return OPRT_INVALID_PARM;
    }


    switch (cmd)
    {
    case ADC_DIV_RESIS:
        //bk_printf("tkl_adc_ioctl :%d\r\n", ch_id);
        g_channel_id_div[ch_id] = 1;
        break;

    default:
        break;
    }

    return OPRT_OK;
}

