#include "config_flash_stm32f4.h"

#include "flash_map.h"

#include <stddef.h>

static const struct {
    uint32_t base;
    uint32_t size;
} sector_table[FLASH_SECTOR_COUNT] = {
    { 0x08000000u, 16u * 1024u },
    { 0x08004000u, 16u * 1024u },
    { 0x08008000u, 16u * 1024u },
    { 0x0800C000u, 16u * 1024u },
    { 0x08010000u, 64u * 1024u },
    { 0x08020000u, 128u * 1024u },
    { 0x08040000u, 128u * 1024u },
    { 0x08060000u, 128u * 1024u },
    { 0x08080000u, 128u * 1024u },
    { 0x080A0000u, 128u * 1024u },
    { 0x080C0000u, 128u * 1024u },
    { 0x080E0000u, 128u * 1024u },
};

static int address_to_sector(uint32_t address, uint32_t *sector)
{
    for (uint32_t i = 0; i < FLASH_SECTOR_COUNT; i++) {
        if (address >= sector_table[i].base && address < sector_table[i].base + sector_table[i].size) {
            *sector = i;
            return 1;
        }
    }

    return 0;
}

static uint32_t backend_read32(void *ctx, uint32_t address)
{
    flash_t *flash = ctx;
    uint32_t value = 0;

    flash_read_word(flash, address, &value);

    return value;
}

static flash_ops_result_t backend_program32(void *ctx, uint32_t address, uint32_t word)
{
    flash_t *flash = ctx;

    return flash_program_word(flash, address, word) == FLASH_OK ? FLASH_OPS_OK : FLASH_OPS_HALTED;
}

static flash_ops_result_t backend_erase_sector(void *ctx, uint32_t address)
{
    flash_t *flash = ctx;
    uint32_t sector;

    if (!address_to_sector(address, &sector)) {
        return FLASH_OPS_HALTED;
    }

    return flash_erase_sector(flash, sector) == FLASH_OK ? FLASH_OPS_OK : FLASH_OPS_HALTED;
}

static int backend_sector_span(void *ctx, uint32_t address, uint32_t *base, uint32_t *size)
{
    uint32_t sector;

    (void)ctx;

    if (!address_to_sector(address, &sector)) {
        return 0;
    }

    *base = sector_table[sector].base;
    *size = sector_table[sector].size;

    return 1;
}

void config_flash_stm32f4_init(config_flash_t *config, flash_t *flash)
{
    config->ops.ctx = flash;
    config->ops.read32 = backend_read32;
    config->ops.program32 = backend_program32;
    config->ops.erase_sector = backend_erase_sector;
    config->ops.sector_span = backend_sector_span;
    config->slot_address[0] = FLASH_MAP_CONFIG_A_BASE;
    config->slot_address[1] = FLASH_MAP_CONFIG_B_BASE;
}
