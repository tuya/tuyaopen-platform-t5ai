#ifndef _TYPE_H_
#define _TYPE_H_

#include <stdint.h>
#include "types.h"

typedef enum {
    FLASH_LINE_1=0,
    FLASH_LINE_2,
    FLASH_LINE_4
} FLASH_LINE_MODE;

enum
{	
	STATUS_OK = 0,
	FLASH_STATUS_BUSY = 1,
	SPI_OP_T_OUT = 2,
	FLASH_OP_T_OUT = 3,
	PACK_LEN_ERROR = 4,

	PACK_PAYLOAD_LACK = 5,

	PARAM_ERROR = 6,

	UNKNOW_CMD = 7,
};


#endif
// EOF
