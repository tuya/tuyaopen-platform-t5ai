#ifndef _DIFF2YA_H_
#define _DIFF2YA_H_

#include <stdint.h>
#include <stdlib.h>
#include "bsdiff_flash.h"

#define DIFF2YA_MODULE_VER_NAME     "V1.1.0"
#define DIFF2YA_MODULE_VER_CODE     2

typedef enum {
	DIFF2YA_OK = 0,
	DIFF2YA_ERROR_FLASH_READ = 1,
	DIFF2YA_ERROR_CRC32_OLD = 2,
	DIFF2YA_ERROR_CRC32_NEW = 3,
	DIFF2YA_ERROR_CRC32_PATCH = 4,
	DIFF2YA_ERROR_PATCH_PACKAGE = 5,
	DIFF2YA_ERROR_NEW_FLASH_DATA = 6,
	DIFF2YA_ERROR_DIFF_VER_NO_SUPP = 7,
    DIFF2YA_ERROR_BUF_SIZE = 8,
    DIFF2YA_ERROR_STEP_SIZE = 9,
	DIFF2YA_ERROR_PATCH_TYPE = 10,
}diff2ya_status_t;

diff2ya_status_t diff2ya_header_read(diff2ya_context_t* ctx, diff2ya_header_t *header);
diff2ya_status_t diff2ya_run(bsdiff_mag_t *mag, diff2ya_header_t* diff2ya_header, void* arg);
int ty_bsdiff_entry(void* arg);

#endif