#pragma once
//#include "tinycrypt/ecc.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

uint8_t zephyr_is_scan_enable(void);
uint8_t zephyr_get_adv_handle(void);
bool zephyr_is_stack_inited(void);
