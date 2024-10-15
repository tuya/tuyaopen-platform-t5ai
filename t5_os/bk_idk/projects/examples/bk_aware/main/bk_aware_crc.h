#include <stdint.h>

uint16_t crc16_le(uint16_t crc, const uint8_t *buf, uint32_t len);
uint32_t crc32_le(uint32_t crc, const uint8_t *buf, uint32_t len);

