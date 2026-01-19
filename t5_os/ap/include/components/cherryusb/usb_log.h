/*
 * Copyright (c) 2022, sakumisu
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef USB_LOG_H
#define USB_LOG_H

#include <stdio.h>
#include <components/log.h>

#define CHERRY_USB_TAG "CHERRY_USB"
#define USB_LOG_INFO(...) BK_LOGI(CHERRY_USB_TAG, ##__VA_ARGS__)
#define USB_LOG_WRN(...) BK_LOGW(CHERRY_USB_TAG, ##__VA_ARGS__)
#define USB_LOG_RAW(...) BK_LOGW(CHERRY_USB_TAG, ##__VA_ARGS__)
#define USB_LOG_ERR(...) BK_LOGE(CHERRY_USB_TAG, ##__VA_ARGS__)
#define USB_LOG_DBG(...) BK_LOGD(CHERRY_USB_TAG, ##__VA_ARGS__)
#define USB_LOG_VBS(...) BK_LOGV(CHERRY_USB_TAG, ##__VA_ARGS__)

#endif /* USB_LOG_H */