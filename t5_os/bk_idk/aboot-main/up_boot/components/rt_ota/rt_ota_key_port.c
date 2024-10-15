
#if 0
#include <stdio.h>
#include <string.h>
#include <stdint.h>

extern uint32_t iv_table_a;
extern uint32_t key_table_a;

static const uint8_t iv_table[16 + 1] = "0123456789ABCDEF";
static const uint8_t key_table[32 + 1] = "0123456789ABCDEF0123456789ABCDEF";

/**
 * Get the decryption key & iv
 *
 * @param iv_buf initialization vector
 * @param key_buf aes key
 */
void rt_ota_get_iv_key(uint8_t * iv_buf, uint8_t * key_buf)
{
    memcpy(iv_buf, iv_table, 17);
    memcpy(key_buf, key_table, 33);
}
#endif
