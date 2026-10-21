#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

#define CONFIG_MAGIC 0x31474643u

#define CONFIG_SLOT_COUNT 2u
#define CONFIG_RECORD_SIZE 28u
#define CONFIG_RECORD_CRC_BYTES (CONFIG_RECORD_SIZE - 4u)

#define CONFIG_SLOT_NONE 0xFFFFFFFFu

typedef struct {
    uint32_t magic;
    uint32_t sequence;
    uint32_t active_slot;
    uint32_t pending_slot;
    uint32_t trial_count;
    uint32_t min_version;
    uint32_t crc;
} config_record_t;

typedef enum {
    CONFIG_IO_OK = 0,
    CONFIG_IO_HALTED,
} config_io_result_t;

typedef struct {
    void *ctx;
    uint32_t slot_address[CONFIG_SLOT_COUNT];
    uint32_t (*read32)(void *ctx, uint32_t address);
    config_io_result_t (*program32)(void *ctx, uint32_t address, uint32_t word);
    config_io_result_t (*erase_sector)(void *ctx, uint32_t address);
} config_flash_t;

typedef enum {
    CONFIG_OK = 0,
    CONFIG_ERR_EMPTY,
    CONFIG_ERR_PARAM,
    CONFIG_ERR_IO,
} config_status_t;

config_status_t config_load(const config_flash_t *flash, config_record_t *out);
config_status_t config_store(const config_flash_t *flash, const config_record_t *record);

#endif
