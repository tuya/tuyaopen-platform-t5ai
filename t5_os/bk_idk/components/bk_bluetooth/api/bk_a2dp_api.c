
#include "bk_a2dp_private.h"

bt_err_t bk_bt_a2dp_register_callback(bk_bt_a2dp_cb_t callback)
{
    return bt_a2dp_register_callback_internal(callback);
}

bt_err_t bk_bt_a2dp_sink_init(uint8_t aac_supported)
{
    return bt_a2dp_sink_init_internal(aac_supported);
}

bt_err_t bk_bt_a2dp_sink_deinit(void)
{
    return bt_a2dp_sink_deinit_internal();
}

bt_err_t bk_bt_a2dp_sink_register_data_callback(bk_bt_sink_data_cb_t callback)
{
    return bt_a2dp_sink_register_data_callback_internal(callback);
}

bt_err_t bk_bt_a2dp_sink_connect(uint8_t *remote_bda)
{
    return bt_a2dp_sink_connect_internal(remote_bda);
}

bt_err_t bk_bt_a2dp_sink_disconnect(uint8_t *remote_bda)
{
    return bt_a2dp_sink_disconnect_internal(remote_bda);
}

bt_err_t bk_bt_a2dp_sink_set_delay_value(uint16_t delay_value)
{
    return bt_a2dp_sink_set_delay_value_internal(delay_value);
}

bt_err_t bk_bt_a2dp_sink_get_delay_value(void)
{
    return bt_a2dp_sink_get_delay_value_internal();
}

