/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <errno.h>

#include <bluetooth/mesh.h>

#define LOG_MODULE_NAME bt_mesh_test
#include "common/log.h"

#include "mesh.h"
#include "test.h"

#ifdef CONFIG_BT_MESH_SELF_TEST
int bt_mesh_test(void)
{
	return 0;
}
#endif
