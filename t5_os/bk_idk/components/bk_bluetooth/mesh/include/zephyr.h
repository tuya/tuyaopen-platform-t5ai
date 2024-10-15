/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_ZEPHYR_H_
#define ZEPHYR_INCLUDE_ZEPHYR_H_

/*
 * Applications can identify whether they are built for Zephyr by
 * macro below. (It may be already defined by a makefile or toolchain.)
 */
#ifndef __ZEPHYR__
#define __ZEPHYR__
#endif

#include "zephyr/zp_config.h"

struct device{
	char ___a;
};

#define __fallthrough
#define	PRIu8			"u"		/* uint8_t */

#include "zephyr/zephyr_rtos.h"
#include "zephyr/zephyr_log.h"
#include "zephyr/z_errno.h"
#include "zephyr/z_gcc.h"
#include "zephyr/z_ffs.h"
#include "sys/z_queue.h"
#include "sys/z_work.h"
#include "sys/z_poll.h"
#include "sys/z_mem_slab.h"

#endif /* ZEPHYR_INCLUDE_ZEPHYR_H_ */

