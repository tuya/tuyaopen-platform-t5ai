#include "stdint.h"
#include "string.h"
#include "bluetooth/bluetooth.h"
#include "bluetooth/uuid.h"
#include "bluetooth/hci.h"
#include "zephyr/zephyr_log.h"

static inline char *log_strdup(const char *str)
{
	return (char *)str;
}

const char *bt_hex_real(const void *buf, size_t len);
const char *bt_addr_str_real(const bt_addr_t *addr);
const char *bt_addr_le_str_real(const bt_addr_le_t *addr);
const char *bt_uuid_str_real(const struct bt_uuid *uuid);
uint8_t bt_hex_to_buff(char *out_buf, size_t *out_len, uint8 *in_buf, size_t len);

#define bt_hex(buf, len) log_strdup(bt_hex_real(buf, len))
#define bt_addr_str(addr) log_strdup(bt_addr_str_real(addr))
#define bt_addr_le_str(addr) log_strdup(bt_addr_le_str_real(addr))
#define bt_uuid_str(uuid) log_strdup(bt_uuid_str_real(uuid))

