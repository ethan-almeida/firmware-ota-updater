#include "config_fake.h"

#include <string.h>

static int slot_for_address(uint32_t address, uint32_t *slot_index, uint32_t *offset)
{
    if (address >= CONFIG_FAKE_SLOT_A && address < CONFIG_FAKE_SLOT_A + CONFIG_FAKE_SLOT_SIZE) {
        *slot_index = 0;
    } else if (address >= CONFIG_FAKE_SLOT_B && address < CONFIG_FAKE_SLOT_B + CONFIG_FAKE_SLOT_SIZE) {
        *slot_index = 1;
    } else {
        return 0;
    }
    *offset = address - (*slot_index == 0u ? CONFIG_FAKE_SLOT_A : CONFIG_FAKE_SLOT_B);
    return 1;
}

static flash_ops_result_t consume_write(config_fake_t *f)
{
    if (f->writes_left == 0) {
        return FLASH_OPS_HALTED;
    }
    if (f->writes_left > 0) {
        f->writes_left--;
    }
    return FLASH_OPS_OK;
}

static uint32_t fake_read32(void *ctx, uint32_t address)
{
    config_fake_t *f = ctx;
    uint32_t slot_index;
    uint32_t offset;
    uint32_t value = 0xFFFFFFFFu;

    if (slot_for_address(address, &slot_index, &offset) && offset + 4u <= CONFIG_FAKE_SLOT_SIZE) {
        memcpy(&value, &f->slot[slot_index][offset], sizeof(value));
    }
    return value;
}

static flash_ops_result_t fake_program32(void *ctx, uint32_t address, uint32_t word)
{
    config_fake_t *f = ctx;
    uint32_t slot_index;
    uint32_t offset;
    uint32_t old = 0xFFFFFFFFu;

    if (!slot_for_address(address, &slot_index, &offset) || offset + 4u > CONFIG_FAKE_SLOT_SIZE) {
        return FLASH_OPS_HALTED;
    }
    if (consume_write(f) != FLASH_OPS_OK) {
        return FLASH_OPS_HALTED;
    }

    memcpy(&old, &f->slot[slot_index][offset], sizeof(old));
    old &= word;
    memcpy(&f->slot[slot_index][offset], &old, sizeof(old));
    return FLASH_OPS_OK;
}

static flash_ops_result_t fake_erase_sector(void *ctx, uint32_t address)
{
    config_fake_t *f = ctx;
    uint32_t slot_index;
    uint32_t offset;

    if (!slot_for_address(address, &slot_index, &offset)) {
        return FLASH_OPS_HALTED;
    }
    if (consume_write(f) != FLASH_OPS_OK) {
        return FLASH_OPS_HALTED;
    }

    memset(f->slot[slot_index], 0xFF, CONFIG_FAKE_SLOT_SIZE);
    return FLASH_OPS_OK;
}

static int fake_sector_span(void *ctx, uint32_t address, uint32_t *base, uint32_t *size)
{
    uint32_t slot_index;
    uint32_t offset;

    (void)ctx;

    if (!slot_for_address(address, &slot_index, &offset)) {
        return 0;
    }

    *base = slot_index == 0u ? CONFIG_FAKE_SLOT_A : CONFIG_FAKE_SLOT_B;
    *size = CONFIG_FAKE_SLOT_SIZE;

    return 1;
}

void config_fake_init(config_fake_t *fake)
{
    memset(fake, 0xFF, sizeof(*fake));
    fake->writes_left = -1;
}

void config_fake_bind(config_fake_t *fake, config_flash_t *flash)
{
    flash->ops.ctx = fake;
    flash->ops.read32 = fake_read32;
    flash->ops.program32 = fake_program32;
    flash->ops.erase_sector = fake_erase_sector;
    flash->ops.sector_span = fake_sector_span;
    flash->slot_address[0] = CONFIG_FAKE_SLOT_A;
    flash->slot_address[1] = CONFIG_FAKE_SLOT_B;
}

uint32_t config_fake_slot_magic(const config_fake_t *fake, uint32_t slot)
{
    uint32_t value;
    memcpy(&value, &fake->slot[slot][0], sizeof(value));
    return value;
}

void config_fake_flip_byte(config_fake_t *fake, uint32_t slot, uint32_t byte_index)
{
    fake->slot[slot][byte_index] ^= 0x01u;
}
