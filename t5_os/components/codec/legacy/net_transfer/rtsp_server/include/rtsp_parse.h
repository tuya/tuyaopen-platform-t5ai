/*
 * @Author: cijliu
 * @Date: 2021-02-05 14:32:30
 * @LastEditTime: 2021-02-26 16:38:36
 */
#ifndef __RTSP_PARSE_H__
#define __RTSP_PARSE_H__
#include "rtsp.h"
int rtsp_parse_msg(const char *msg, rtsp_msg_t *rtsp);
#endif
