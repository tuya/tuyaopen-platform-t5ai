#pragma once

#include <os/os.h>
#include "ff.h"
#include "diskio.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef struct
{
    char file_name[50];
    FIL fd;
} tfcard_util_t;

bk_err_t tfcard_util_create(tfcard_util_t *tfcard_util, char *name);
bk_err_t tfcard_util_destroy(tfcard_util_t *tfcard_util);
bk_err_t tfcard_util_tx_data(tfcard_util_t *tfcard_util, void *data_buf, uint32_t len);

#ifdef __cplusplus
}
#endif

