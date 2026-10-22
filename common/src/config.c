#include "config.h"

#include "crc32.h"

#include <string.h>

_Static_assert(sizeof(config_record_t) == CONFIG_RECORD_SIZE, "config record layout must be stable");

typedef struct {
    config_record_t record;
    uint32_t slot;
    int valid;
} slot_state_t;

static void read_record(const config_flash_t *flash, uint32_t address, config_record_t *record)
{
    uint8_t bytes[CONFIG_RECORD_SIZE];

    for (uint32_t offset = 0; offset < CONFIG_RECORD_SIZE; offset += 4u) {
        uint32_t word = flash->ops.read32(flash->ops.ctx, address + offset);
        memcpy(bytes + offset, &word, sizeof(word));
    }

    memcpy(record, bytes, sizeof(*record));
}

static int record_valid(const config_record_t *record)
{
    if (record->magic != CONFIG_MAGIC) {
        return 0;
    }

    return crc32_calc((const uint8_t *)record, CONFIG_RECORD_CRC_BYTES) == record->crc;
}

static void load_slots(const config_flash_t *flash, slot_state_t slots[CONFIG_SLOT_COUNT])
{
    for (uint32_t slot = 0; slot < CONFIG_SLOT_COUNT; slot++) {
        slots[slot].slot = slot;
        slots[slot].valid = 0;
        read_record(flash, flash->slot_address[slot], &slots[slot].record);
        if (record_valid(&slots[slot].record)) {
            slots[slot].valid = 1;
        }
    }
}

static int newest_slot(const slot_state_t slots[CONFIG_SLOT_COUNT])
{
    int newest = -1;

    for (uint32_t slot = 0; slot < CONFIG_SLOT_COUNT; slot++) {
        if (!slots[slot].valid) {
            continue;
        }
        if (newest < 0 || slots[slot].record.sequence >= slots[newest].record.sequence) {
            newest = (int)slot;
        }
    }

    return newest;
}

config_status_t config_load(const config_flash_t *flash, config_record_t *out)
{
    if (flash == NULL || out == NULL) {
        return CONFIG_ERR_PARAM;
    }

    slot_state_t slots[CONFIG_SLOT_COUNT];
    load_slots(flash, slots);

    int newest = newest_slot(slots);
    if (newest < 0) {
        return CONFIG_ERR_EMPTY;
    }

    *out = slots[newest].record;

    return CONFIG_OK;
}

static flash_ops_result_t program_record(const config_flash_t *flash, uint32_t address,
                                         const config_record_t *record)
{
    const uint8_t *bytes = (const uint8_t *)record;

    for (uint32_t offset = 4u; offset < CONFIG_RECORD_SIZE; offset += 4u) {
        uint32_t word;
        memcpy(&word, bytes + offset, sizeof(word));

        if (flash->ops.program32(flash->ops.ctx, address + offset, word) != FLASH_OPS_OK) {
            return FLASH_OPS_HALTED;
        }
    }

    uint32_t magic;
    memcpy(&magic, bytes, sizeof(magic));

    if (flash->ops.program32(flash->ops.ctx, address, magic) != FLASH_OPS_OK) {
        return FLASH_OPS_HALTED;
    }

    return FLASH_OPS_OK;
}

config_status_t config_store(const config_flash_t *flash, const config_record_t *record)
{
    if (flash == NULL || record == NULL) {
        return CONFIG_ERR_PARAM;
    }

    slot_state_t slots[CONFIG_SLOT_COUNT];
    load_slots(flash, slots);

    int newest = newest_slot(slots);
    uint32_t target = newest < 0 ? 0u : 1u - slots[newest].slot;

    config_record_t next = *record;
    next.magic = CONFIG_MAGIC;
    next.sequence = newest < 0 ? 0u : slots[newest].record.sequence + 1u;
    next.crc = crc32_calc((const uint8_t *)&next, CONFIG_RECORD_CRC_BYTES);

    if (flash->ops.erase_sector(flash->ops.ctx, flash->slot_address[target]) != FLASH_OPS_OK) {
        return CONFIG_ERR_IO;
    }

    if (program_record(flash, flash->slot_address[target], &next) != FLASH_OPS_OK) {
        return CONFIG_ERR_IO;
    }

    return CONFIG_OK;
}
