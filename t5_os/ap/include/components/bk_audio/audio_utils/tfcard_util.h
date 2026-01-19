#pragma once

#include <os/os.h>
#include "ff.h"
#include "diskio.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct tfcard_util *tfcard_util_handle_t;

tfcard_util_handle_t tfcard_util_create(char *name);
bk_err_t tfcard_util_destroy(tfcard_util_handle_t tfcard_util);
bk_err_t tfcard_util_tx_data(tfcard_util_handle_t tfcard_util, void *data_buf, uint32_t len);

#ifdef __cplusplus
}
#endif

