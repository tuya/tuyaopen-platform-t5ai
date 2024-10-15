/*
 * Copyright (C), 2018-2019, Arm Technology (China) Co., Ltd.
 * All rights reserved
 *
 * The content of this file or document is CONFIDENTIAL and PROPRIETARY
 * to Arm Technology (China) Co., Ltd. It is subject to the terms of a
 * License Agreement between Licensee and Arm Technology (China) Co., Ltd
 * restricting among other things, the use, reproduction, distribution
 * and transfer.  Each of the embodiments, including this information and,,
 * any derivative work shall retain this copyright notice.
 */

#include "device.h"
#include <stdint.h>
#include <stddef.h>
#include "string.h"

#define DBG_PRE_PROMPT "DEV"
#include "dbg.h"

extern bool g_uart_is_init;
#ifndef __ARMCC_VERSION
extern struct device __device_init_start[];
extern struct device __device_init_end[];
#endif

/**
 * @brief Execute all the device initialization functions at a given level
 *
 * @details Invokes the initialization routine for each device object
 * created by the DEVICE_INIT() macro using the specified level.
 * The linker script places the device objects in memory in the order
 * they need to be invoked, with symbols indicating where one level leaves
 * off and the next one begins.
 *
 * @param level init level to run.
 */
int32_t sys_device_do_config_level(int32_t level)
{
    struct device *info;
    struct device_config *config;
    int32_t ret;
	
#ifdef __ARMCC_VERSION
	extern unsigned int Load$$DEV_INIT$$Base;
	extern unsigned int Load$$DEV_INIT$$Limit;

	void *__device_init_start = &Load$$DEV_INIT$$Base;
	void *__device_init_end = &Load$$DEV_INIT$$Limit;
#endif

    for (info = (struct device *)__device_init_start; 
			info != (struct device *)__device_init_end; 
			info++) {
        if (info->level > level) {
            break;
        }
        if (info->level != level) {
            continue;
        }
        config = info->config;
        if (NULL == config) {
            continue;
        }
        if (NULL == config->init) {
            continue;
        }
#ifndef CONFIG_BK_BOOT
        if (true == g_uart_is_init) {
            DBG("init device[%s] driver[%s]\n",info->name, config->name);
        }
#endif
        ret = config->init(info);
        if (0 != ret) {
            /* TODO, uninit the device that has been inited */
            return ret;
        }
    }

    return 0;
}

/**
 * @brief Retrieve the device structure for a driver by name
 *
 * @details Device objects are created via the DEVICE_INIT() macro and
 * placed in memory by the linker. If a driver needs to bind to another driver
 * it can use this function to retrieve the device structure of the lower level
 * driver by the name the driver exposes to the system.
 *
 * @param name device name to search for.
 *
 * @return pointer to device structure; NULL if not found or cannot be used.
 */
struct device *device_get(const char *name)
{
    struct device *info;
	
#ifdef __ARMCC_VERSION
	extern unsigned int Load$$DEV_INIT$$Base;
	extern unsigned int Load$$DEV_INIT$$Limit;

	void *__device_init_start = &Load$$DEV_INIT$$Base;
	void *__device_init_end = &Load$$DEV_INIT$$Limit;
#endif

    /* Split the search into two loops: in the common scenario, where
     * device names are stored in ROM (and are referenced by the user
     * with CONFIG_* macros), only cheap pointer comparisons will be
     * performed.  Reserve string comparisons for a fallback.
     */
    for (info = __device_init_start; info != __device_init_end; info++) {
        if (info->driver_api != NULL && info->config->name == name) {
            return info;
        }
    }

    for (info = __device_init_start; info != __device_init_end; info++) {
        if (!info->driver_api) {
            continue;
        }

        if (!strcmp(name, info->config->name)) {
            return info;
        }
    }

    return NULL;
}

