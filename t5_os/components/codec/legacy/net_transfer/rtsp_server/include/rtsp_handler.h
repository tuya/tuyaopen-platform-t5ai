/*
 * @Author: cijliu
 * @Date: 2021-02-05 15:12:06
 * @LastEditTime: 2021-02-20 14:28:43
 */
#ifndef __RTSP_HANDLER_H__
#define __RTSP_HANDLER_H__
#include "rtsp.h"
void rtsp_options_handler(rtsp_msg_t rtsp);
void rtsp_describe_handler(rtsp_msg_t rtsp);
void rtsp_setup_handler(rtsp_msg_t rtsp);
void rtsp_play_handler(rtsp_msg_t rtsp);
void rtsp_record_handler(rtsp_msg_t rtsp);
void rtsp_pause_handler(rtsp_msg_t rtsp);
void rtsp_teardown_handler(rtsp_msg_t rtsp);
void rtsp_announce_handler(rtsp_msg_t rtsp);
void rtsp_set_parameter_handler(rtsp_msg_t rtsp);
void rtsp_get_parameter_handler(rtsp_msg_t rtsp);
void rtsp_redirect_handler(rtsp_msg_t rtsp);

rtsp_msg_t rtsp_msg_load(const char *msg);
rtsp_rely_t get_rely(rtsp_msg_t rtsp);
int rtsp_rely_dumps(rtsp_rely_t rely, char *msg, uint32_t len);
#endif