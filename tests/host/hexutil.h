#ifndef HEXUTIL_H
#define HEXUTIL_H

#include <stddef.h>
#include <stdint.h>

static inline int hex_nibble(char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return -1;
}

static inline size_t hex_decode(const char *hex, uint8_t *out, size_t out_cap)
{
    size_t len = 0;

    while (hex[0] != '\0' && hex[1] != '\0' && len < out_cap) {
        int hi = hex_nibble(hex[0]);
        int lo = hex_nibble(hex[1]);
        if (hi < 0 || lo < 0) {
            break;
        }
        out[len++] = (uint8_t)((hi << 4) | lo);
        hex += 2;
    }

    return len;
}

#endif
