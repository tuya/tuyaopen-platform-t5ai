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

#ifndef __DEVICE_H__
#define __DEVICE_H__

#include <stdint.h>
#include <stdbool.h>

#define DEVICE_LEVEL0       0
#define DEVICE_LEVEL1       1
#define DEVICE_LEVEL2       2
#define DEVICE_LEVEL3       3
#define DEVICE_MAX_LEVEL    3

#ifndef _CONCAT
#define _DO_CONCAT(x, y) x##y
#define _CONCAT(x, y) _DO_CONCAT(x, y)
#endif

#ifndef STRINGIFY
#define _STRINGIFY(x) #x
#define STRINGIFY(s) _STRINGIFY(s)
#endif

struct device;
/**
 * @brief Static device information (In ROM) Per driver instance
 *
 * @param name name of the device
 * @param init init function for the driver
 * @param config_info address of driver instance config information
 */
struct device_config {
    const char *name;
    int32_t (*init)(struct device *device);
#ifdef CONFIG_DEVICE_POWER_MANAGEMENT
    int (*device_pm_control)(struct device *device, uint32_t command,
                 void *context);
#endif
    const void *config_info;
};

/**
 * @brief Runtime device structure (In memory) Per driver instance
 * @param device_config Build time config information
 * @param driver_api pointer to structure containing the API functions for
 * the device type. This pointer is filled in by the driver at init time.
 * @param driver_data driver instance data. For driver use only
 */
struct device {
    const char *name;
    uint32_t level;
    struct device_config *config;
    const void *driver_api;
    void *driver_data;
};

/**
 * @def DEVICE_INIT
 *
 * @brief Create device object and set it up for boot time initialization.
 *
 * @details This macro defines a device object that is automatically
 * configured by the kernel during system initialization.
 *
 * @param dev_name Device name.
 *
 * @param drv_name The name this instance of the driver exposes to
 * the system.
 *
 * @param init_fn Address to the init function of the driver.
 *
 * @param data Pointer to the device's configuration data.
 *
 * @param cfg_info The address to the structure containing the
 * configuration information for this instance of the driver.
 *
 * @param level The initialization level at which configuration occurs.
 * Must be one of the following symbols, which are listed in the order
 * they are performed by the kernel:
 * \n
 * \li DEVICE_PRE: Used for devices that have no dependencies, such as those
 * that rely solely on hardware present in the processor/SOC. These devices
 * cannot use any kernel services during configuration, since they are not
 * yet available.
 * \n
 * \li DEVICE_POST: Used for devices that rely on the initialization of devices
 * initialized as part of the PRIMARY level. These devices cannot use any
 * kernel services during configuration, since they are not yet available.
 * \n
 * @param prio The initialization priority of the device, relative to
 * other devices of the same initialization level. Specified as an integer
 * value in the range 0 to 99; lower values indicate earlier initialization.
 * Must be a decimal integer literal without leading zeroes or sign (e.g. 32),
 * or an equivalent symbolic name (e.g. \#define MY_INIT_PRIO 32); symbolic
 * expressions are *not* permitted
 * (e.g. CONFIG_KERNEL_INIT_PRIORITY_DEFAULT + 5).
 */
#ifndef CONFIG_DEVICE_POWER_MANAGEMENT
#define DEVICE_DEFINE(dev_name, drv_name, init_fn,                                          \
              data, cfg_info, _level, prio, api)                                            \
    static struct device_config _CONCAT(__config_, dev_name) __attribute__((__used__))      \
    __attribute__((__section__(".devconfig.init"))) = {                                     \
        .name = drv_name, .init = (init_fn),                                                \
        .config_info = (cfg_info)                                                           \
    };                                                                                      \
    static struct device _CONCAT(__device_, dev_name) __attribute__((__used__))             \
    __attribute__((__section__(".init_level_" STRINGIFY(_level)"_" STRINGIFY(prio)))) = {   \
        .name = STRINGIFY(dev_name),                                                        \
        .level = _level,                                                                    \
        .config = &_CONCAT(__config_, dev_name),                                            \
        .driver_api = api,                                                                  \
        .driver_data = data                                                                 \
    }
#else
#define DEVICE_DEFINE(dev_name, drv_name, init_fn, pm_control_fn,                           \
              data, cfg_info, _level, prio, api)                                            \
    static struct device_config _CONCAT(__config_, dev_name) __attribute__((__used__))      \
    __attribute__((__section__(".devconfig.init"))) = {                                     \
        .name = drv_name, .init = (init_fn),                                                \
        .device_pm_control = (pm_control_fn);                                               \
        .config_info = (cfg_info)                                                           \
    };                                                                                      \
    static struct device _CONCAT(__device_, dev_name) __attribute__((__used__))             \
    __attribute__((__section__(".init_level_" STRINGIFY(_level)"_" STRINGIFY(prio)))) = {   \
        .name = STRINGIFY(dev_name),                                                        \
        .level = _level,                                                                    \
        .config = &_CONCAT(__config_, dev_name),                                            \
        .driver_api = api,                                                                  \
        .driver_data = data                                                                 \
    }
#endif

/**
 * @def DEVICE_NAME_GET
 *
 * @brief Expands to the full name of a global device object
 *
 * @details Return the full name of a device object symbol created by
 * DEVICE_INIT(), using the dev_name provided to DEVICE_INIT().
 *
 * It is meant to be used for declaring extern symbols pointing on device
 * objects before using the DEVICE_GET macro to get the device object.
 *
 * @param name The same as dev_name provided to DEVICE_INIT()
 *
 * @return The expanded name of the device object created by DEVICE_INIT()
 */
#define DEVICE_NAME_GET(name) (_CONCAT(__device_, name))

/**
 * @def DEVICE_GET
 *
 * @brief Obtain a pointer to a device object by name
 *
 * @details Return the address of a device object created by
 * DEVICE_INIT(), using the dev_name provided to DEVICE_INIT().
 *
 * @param name The same as dev_name provided to DEVICE_INIT()
 *
 * @return A pointer to the device object created by DEVICE_INIT()
 */
#define DEVICE_GET(name) (&DEVICE_NAME_GET(name))

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
struct device *device_get(const char *name);

int32_t sys_device_do_config_level(int32_t level);

#endif /* __DEVICE_H__ */

