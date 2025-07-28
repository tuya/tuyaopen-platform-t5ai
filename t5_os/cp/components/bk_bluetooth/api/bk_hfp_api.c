
#include "bk_hfp_private.h"

bt_err_t bk_bt_hf_client_register_callback(bk_bt_hf_client_cb_t callback)
{
    return bt_hf_client_register_callback_internal(callback);
}

bt_err_t bk_bt_hf_client_init(uint8_t msbc_supported)
{
    return bt_hf_client_init_internal(msbc_supported);
}

bt_err_t bk_bt_hf_client_deinit(void)
{
    return bt_hf_client_deinit_internal();
}

bt_err_t bk_bt_hf_client_register_data_callback(bk_bt_hf_client_data_cb_t callback)
{
    return bt_hf_client_register_data_callback_internal(callback);
}

bt_err_t bk_bt_hf_client_voice_out_write(uint8_t *remote_bda, uint8_t *buf, uint16_t len)
{
    return bt_hf_client_voice_out_write_internal(remote_bda, buf, len);
}

bt_err_t bk_bt_hf_client_connect(uint8_t *remote_bda)
{
    return bt_hf_client_connect_internal(remote_bda);
}

bt_err_t bk_bt_hf_client_disconnect(uint8_t *remote_bda)
{
    return bt_hf_client_disconnect_internal(remote_bda);
}

bt_err_t bk_bt_hf_client_connect_audio(uint8_t *remote_bda)
{
    return bt_hf_client_connect_audio_internal(remote_bda);
}

bt_err_t bk_bt_hf_client_disconnect_audio(uint8_t *remote_bda)
{
    return bt_hf_client_disconnect_audio_internal(remote_bda);
}

bt_err_t bk_bt_hf_client_start_voice_recognition(uint8_t *remote_bda)
{
    return bt_hf_client_start_voice_recognition_internal(remote_bda);
}

bt_err_t bk_bt_hf_client_stop_voice_recognition(uint8_t *remote_bda)
{
    return bt_hf_client_stop_voice_recognition_internal(remote_bda);
}

bt_err_t bk_bt_hf_client_volume_update(uint8_t *remote_bda, bk_hf_volume_control_target_t type, uint8_t volume)
{
    return bt_hf_client_volume_update_internal(remote_bda, type, volume);
}

bt_err_t bk_bt_hf_client_dial(uint8_t *remote_bda, const char *number)
{
    return bt_hf_client_dial_internal(remote_bda, number);
}

bt_err_t bk_bt_hf_client_dial_memory(uint8_t *remote_bda, int location)
{
    return bt_hf_client_dial_memory_internal(remote_bda, location);
}

bt_err_t bk_bt_hf_client_send_chld_cmd(uint8_t *remote_bda, bk_hf_chld_type_t chld)
{
    return bt_hf_client_send_chld_cmd_internal(remote_bda, chld);
}

bt_err_t bk_bt_hf_client_send_btrh_cmd(uint8_t *remote_bda, bk_hf_btrh_cmd_t btrh)
{
    return bt_hf_client_send_btrh_cmd_internal(remote_bda, btrh);
}

bt_err_t bk_bt_hf_client_answer_call(uint8_t *remote_bda)
{
    return bt_hf_client_answer_call_internal(remote_bda);
}

bt_err_t bk_bt_hf_client_reject_call(uint8_t *remote_bda)
{
    return bt_hf_client_reject_call_internal(remote_bda);
}

bt_err_t bk_bt_hf_client_query_current_calls(uint8_t *remote_bda)
{
    return bt_hf_client_query_current_calls_internal(remote_bda);
}

bt_err_t bk_bt_hf_client_query_current_operator_name(uint8_t *remote_bda)
{
    return bt_hf_client_query_current_operator_name_internal(remote_bda);
}

bt_err_t bk_bt_hf_client_retrieve_subscriber_info(uint8_t *remote_bda)
{
    return bt_hf_client_retrieve_subscriber_info_internal(remote_bda);
}

bt_err_t bk_bt_hf_client_send_dtmf(uint8_t *remote_bda, const char *code)
{
    return bt_hf_client_send_dtmf_internal(remote_bda, code);
}

bt_err_t bk_bt_hf_client_request_last_voice_tag_number(uint8_t *remote_bda)
{
    return bt_hf_client_request_last_voice_tag_number_internal(remote_bda);
}

bt_err_t bk_bt_hf_client_send_nrec(uint8_t *remote_bda)
{
    return bt_hf_client_send_nrec_internal(remote_bda);
}

bt_err_t bk_bt_hf_client_redial(uint8_t *remote_bda)
{
    return bt_hf_client_redial_internal(remote_bda);
}

bt_err_t bk_bt_hf_client_send_custom_cmd(uint8_t *remote_bda, const char *atcmd)
{
    return bt_hf_client_send_custom_cmd_internal(remote_bda, atcmd);
}

