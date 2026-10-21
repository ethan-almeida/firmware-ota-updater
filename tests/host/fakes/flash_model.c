#include "flash_model.h"

#include <string.h>

#define REG_INDEX(offset) ((offset) >> 2)

static const struct {
    uint32_t base;
    uint32_t size;
} sector_table[FLASH_MODEL_SECTORS] = {
    { FLASH_MODEL_BASE + 0x0000u, 16u * 1024u },
    { FLASH_MODEL_BASE + 0x4000u, 16u * 1024u },
    { FLASH_MODEL_BASE + 0x8000u, 16u * 1024u },
    { FLASH_MODEL_BASE + 0xC000u, 16u * 1024u },
};

static int address_to_word(uint32_t address, uint32_t *index)
{
    if (address < FLASH_MODEL_BASE || address >= FLASH_MODEL_BASE + FLASH_MODEL_SIZE) {
        return 0;
    }
    if ((address & 3u) != 0u) {
        return 0;
    }
    *index = (address - FLASH_MODEL_BASE) / 4u;
    return 1;
}

static void perform_erase(flash_model_t *model, uint32_t sector)
{
    model->erase_count++;
    if (sector >= FLASH_MODEL_SECTORS) {
        model->regs[REG_INDEX(FLASH_REG_SR)] |= FLASH_SR_PGSERR | FLASH_SR_EOP;
        return;
    }
    uint32_t first = (sector_table[sector].base - FLASH_MODEL_BASE) / 4u;
    uint32_t count = sector_table[sector].size / 4u;
    for (uint32_t i = 0; i < count; i++) {
        model->words[first + i] = 0xFFFFFFFFu;
    }
    model->bsy_polls_left = model->stuck_bsy ? 0xFFFFFFFFu : 1u;
    model->regs[REG_INDEX(FLASH_REG_SR)] |= FLASH_SR_EOP | model->op_error_flags;
}

static void perform_program(flash_model_t *model, uint32_t address, uint32_t value)
{
    model->program_count++;
    model->bsy_polls_left = model->stuck_bsy ? 0xFFFFFFFFu : 1u;

    if (model->ignored_programs > 0u) {
        model->ignored_programs--;
        model->regs[REG_INDEX(FLASH_REG_SR)] |= FLASH_SR_EOP | model->op_error_flags;
        return;
    }

    uint32_t index;
    if (!address_to_word(address, &index)) {
        model->regs[REG_INDEX(FLASH_REG_SR)] |= FLASH_SR_PGSERR | FLASH_SR_EOP;
        return;
    }

    model->words[index] &= value;
    model->regs[REG_INDEX(FLASH_REG_SR)] |= FLASH_SR_EOP | model->op_error_flags;
}

static uint32_t model_read_reg(void *ctx, uint32_t offset)
{
    flash_model_t *model = ctx;

    if (offset == FLASH_REG_SR && model->bsy_polls_left > 0u) {
        model->bsy_polls_left--;
        return FLASH_SR_BSY;
    }

    return model->regs[REG_INDEX(offset)];
}

static void model_write_reg(void *ctx, uint32_t offset, uint32_t value)
{
    flash_model_t *model = ctx;
    uint32_t *reg = &model->regs[REG_INDEX(offset)];

    if (offset == FLASH_REG_KEYR) {
        model->key_writes++;
        if (value == FLASH_KEY1) {
            model->key_state = 1u;
        } else if (value == FLASH_KEY2 && model->key_state == 1u) {
            if (!model->ignore_unlock) {
                *reg = 0u;
                model->regs[REG_INDEX(FLASH_REG_CR)] &= ~FLASH_CR_LOCK;
            }
            model->key_state = 0u;
        } else {
            model->key_state = 0u;
        }
        return;
    }

    if (offset == FLASH_REG_SR) {
        *reg &= ~value;
        return;
    }

    if (offset == FLASH_REG_CR) {
        *reg = (value & ~(FLASH_CR_LOCK | FLASH_CR_STRT)) | (*reg & FLASH_CR_LOCK);
        if ((value & FLASH_CR_LOCK) != 0u) {
            *reg |= FLASH_CR_LOCK;
        }
        if ((value & FLASH_CR_STRT) != 0u && (value & FLASH_CR_SER) != 0u) {
            if ((*reg & FLASH_CR_LOCK) != 0u) {
                return;
            }
            perform_erase(model, (value & FLASH_CR_SNB_MASK) >> 3);
        }
        return;
    }

    *reg = value;
}

static void model_write_mem(void *ctx, uint32_t address, uint32_t value)
{
    flash_model_t *model = ctx;
    uint32_t cr = model->regs[REG_INDEX(FLASH_REG_CR)];

    if ((cr & FLASH_CR_LOCK) != 0u || (cr & FLASH_CR_PG) == 0u) {
        return;
    }

    perform_program(model, address, value);
}

static uint32_t model_read_mem(void *ctx, uint32_t address)
{
    flash_model_t *model = ctx;
    uint32_t index;

    if (!address_to_word(address, &index)) {
        return 0xFFFFFFFFu;
    }

    return model->words[index];
}

void flash_model_init(flash_model_t *model)
{
    memset(model, 0, sizeof(*model));

    model->hw.ctx = model;
    model->hw.read_reg = model_read_reg;
    model->hw.write_reg = model_write_reg;
    model->hw.write_mem = model_write_mem;
    model->hw.read_mem = model_read_mem;

    model->regs[REG_INDEX(FLASH_REG_CR)] = FLASH_CR_LOCK;
    for (uint32_t i = 0; i < FLASH_MODEL_SIZE / 4u; i++) {
        model->words[i] = 0xFFFFFFFFu;
    }
}

const flash_hw_t *flash_model_hw(flash_model_t *model)
{
    return &model->hw;
}

uint32_t flash_model_cr(const flash_model_t *model)
{
    return model->regs[REG_INDEX(FLASH_REG_CR)];
}

uint32_t flash_model_sr(const flash_model_t *model)
{
    return model->regs[REG_INDEX(FLASH_REG_SR)];
}

uint32_t flash_model_read(const flash_model_t *model, uint32_t address)
{
    uint32_t index;

    if (!address_to_word(address, &index)) {
        return 0xFFFFFFFFu;
    }

    return model->words[index];
}
