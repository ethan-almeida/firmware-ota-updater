#ifndef CRC32_H
#define CRC32_H

#include <stddef.h>
#include <stdint.h>


#define CRC32_SEED 0xFFFFFFFFu

uint32_t crc32_update(uint32_t crc, const uint8_t *data, size_t len);
uint32_t crc32_finalize(uint32_t crc);
uint32_t crc32_calc(const uint8_t *data, size_t len);

#endif 
