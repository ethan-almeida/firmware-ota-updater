#ifndef FLASH_OPS_H
#define FLASH_OPS_H

#include <stdint.h>

typedef enum {
    FLASH_OPS_OK = 0,
    FLASH_OPS_HALTED,
} flash_ops_result_t;

typedef struct {
    void *ctx;
    uint32_t (*read32)(void *ctx, uint32_t address);
    flash_ops_result_t (*program32)(void *ctx, uint32_t address, uint32_t word);
    flash_ops_result_t (*erase_sector)(void *ctx, uint32_t address);
    int (*sector_span)(void *ctx, uint32_t address, uint32_t *base, uint32_t *size);
} flash_ops_t;

#endif
