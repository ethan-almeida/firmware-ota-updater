/**
    crc32.c

    Calculates CRC-32 checksums. Used to check for accidental data corruption
    In this case that would be a corrupted firmware file.

*/

#include "crc32.h"


uint32_t crc32_update(uint32_t crc, const uint8_t *data, size_t len)
{
    if (data == NULL) {
        return crc;
    }

    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int bit = 0; bit < 8; bit++) {
            const uint32_t mask = (uint32_t)-(int32_t)(crc & 1u);
            crc = (crc >> 1) ^ (0xEDB88320u & mask);
        }
    }

    return crc;
}

uint32_t crc32_finalize(uint32_t crc)
{
    return crc ^ 0xFFFFFFFFu;
}

uint32_t crc32_calc(const uint8_t *data, size_t len)
{
    return crc32_finalize(crc32_update(CRC32_SEED, data, len));
}
